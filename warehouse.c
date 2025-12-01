// Header file
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

typedef struct {
    int id;
    char name[50];
    int aisle;
    int shelf;
    float weight;
} Product;

typedef struct {
    int product_id;
    int qty;
} OrderItem;

typedef struct {
    int id;
    int item_count;
    OrderItem *items;
} Order;

typedef struct {
    int order_count;
    Order **orders;
    float total_weight;
    int total_items;
} Batch;

typedef struct {
    int product_id;
    int aisle;
    int shelf;
    int qty;
} PickPoint;


float manhattan(int x1, int y1, int x2, int y2) {
    return fabs(x1 - x2) + fabs(y1 - y2);
}

Product* find_product(Product *p, int n, int pid) {
    for (int i = 0; i < n; i++)
        if (p[i].id == pid)
            return &p[i];
    return NULL;
}

Batch* create_batch() {
    Batch *b = malloc(sizeof(Batch));
    b->order_count = 0;
    b->orders = malloc(sizeof(Order*) * 20);
    b->total_weight = 0;
    b->total_items = 0;
    return b;
}

void add_to_batch(Batch *b, Order *o, Product *products, int product_count) {
    b->orders[b->order_count++] = o;

    for (int i = 0; i < o->item_count; i++) {
        Product *p = find_product(products, product_count, o->items[i].product_id);
        b->total_weight += p->weight * o->items[i].qty;
        b->total_items += o->items[i].qty;
    }
}

Batch** batch_orders(Order *orders, int order_count, Product *products, int product_count,
                     int *batch_count, float max_weight, int max_items) {

    Batch **batches = malloc(sizeof(Batch*) * order_count);
    *batch_count = 0;

    for (int i = 0; i < order_count; i++) {

        float ow = 0;
        int oi = 0;

        for (int j = 0; j < orders[i].item_count; j++) {
            Product *p = find_product(products, product_count, orders[i].items[j].product_id);
            ow += p->weight * orders[i].items[j].qty;
            oi += orders[i].items[j].qty;
        }

        int placed = 0;
        for (int b = 0; b < *batch_count; b++) {
            if (batches[b]->total_weight + ow <= max_weight &&
                batches[b]->total_items + oi <= max_items) {
                add_to_batch(batches[b], &orders[i], products, product_count);
                placed = 1;
                break;
            }
        }

        if (!placed) {
            Batch *newb = create_batch();
            add_to_batch(newb, &orders[i], products, product_count);
            batches[(*batch_count)++] = newb;
        }
    }

    return batches;
}


PickPoint* build_pickpoints(Batch *b, int *pp_count, Product *products, int pcount) {
    PickPoint *pp = malloc(sizeof(PickPoint) * 100);
    *pp_count = 0;

    for (int i = 0; i < b->order_count; i++) {
        Order *O = b->orders[i];

        for (int j = 0; j < O->item_count; j++) {
            int pid = O->items[j].product_id;
            int qty = O->items[j].qty;

            int found = -1;
            for (int k = 0; k < *pp_count; k++) {
                if (pp[k].product_id == pid)
                    found = k;
            }

            Product *p = find_product(products, pcount, pid);

            if (found == -1) {
                pp[*pp_count].product_id = pid;
                pp[*pp_count].aisle = p->aisle;
                pp[*pp_count].shelf = p->shelf;
                pp[*pp_count].qty = qty;
                (*pp_count)++;
            } else {
                pp[found].qty += qty;
            }
        }
    }

    return pp;
}


float route_batch(PickPoint *pp, int n, int *seq) {
    int visited[100] = {0};
    int cx = 0, cy = 0;
    float cost = 0;

    for (int s = 0; s < n; s++) {
        float best = INT_MAX;
        int best_idx = -1;

        for (int i = 0; i < n; i++) {
            if (visited[i]) continue;
            float d = manhattan(cx, cy, pp[i].aisle, pp[i].shelf);
            if (d < best) {
                best = d;
                best_idx = i;
            }
        }

        visited[best_idx] = 1;
        seq[s] = best_idx;

        cost += best;
        cx = pp[best_idx].aisle;
        cy = pp[best_idx].shelf;
    }

    cost += manhattan(cx, cy, 0, 0);  
    return cost;
}

int main() {

    Product products[6] = {
        {1, "Soap",       1, 1, 0.5},
        {2, "Shampoo",    1, 4, 1.0},
        {3, "Oil 1L",     2, 3, 1.5},
        {4, "Detergent",  2, 6, 3.0},
        {5, "Toothpaste", 3, 1, 0.2},
        {6, "Flour 5kg",  3, 5, 5.0}
    };

    int product_count = 6;

    Order orders[4];

    orders[0].id = 101;
    orders[0].item_count = 2;
    orders[0].items = (OrderItem[]) {{1,2},{2,1}};

    orders[1].id = 102;
    orders[1].item_count = 2;
    orders[1].items = (OrderItem[]) {{3,1},{4,1}};

    orders[2].id = 103;
    orders[2].item_count = 2;
    orders[2].items = (OrderItem[]) {{2,2},{5,3}};

    orders[3].id = 104;
    orders[3].item_count = 1;
    orders[3].items = (OrderItem[]) {{6,1}};

    int order_count = 4;

    int batch_count = 0;
    Batch **batches = batch_orders(orders, order_count, products, product_count,
                                   &batch_count, 6.0, 8);

    printf("\n======================\n");
    printf("WAREHOUSE OPTIMIZATION\n");
    printf("======================\n");

    float total_batched = 0;

    for (int b = 0; b < batch_count; b++) {
        printf("\n--- Batch %d ---\n", b+1);

        printf("Orders: ");
        for (int i = 0; i < batches[b]->order_count; i++)
            printf("%d ", batches[b]->orders[i]->id);
        printf("\n");

        int pp_count;
        PickPoint *pp = build_pickpoints(batches[b], &pp_count, products, product_count);

        int seq[100];
        float cost = route_batch(pp, pp_count, seq);

        printf("Pick Sequence:\n");
        for (int i = 0; i < pp_count; i++) {
            Product *p = find_product(products, product_count, pp[seq[i]].product_id);
            printf(" -> %s at (%d,%d) qty=%d\n",
                   p->name, p->aisle, p->shelf, pp[seq[i]].qty);
        }

        printf("Return to (0,0)\n");
        printf("Total distance for batch: %.2f\n", cost);

        total_batched += cost;

        free(pp);
    }

    printf("\nTotal batched distance = %.2f\n", total_batched);

    printf("\nDone.\n");
    return 0;
}