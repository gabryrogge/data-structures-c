#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 *  PART 1 – AVL TREE
 *  Self-balancing BST: every insert/delete keeps |bf| <= 1
 * ================================================================ */

typedef struct AVLNode {
    int key;
    int height;
    struct AVLNode *left, *right;
} AVLNode;

static int avl_height(AVLNode *n) { return n ? n->height : 0; }
static int avl_max(int a, int b)  { return a > b ? a : b; }

static AVLNode *avl_new(int key) {
    AVLNode *n = malloc(sizeof *n);
    n->key = key; n->height = 1; n->left = n->right = NULL;
    return n;
}

static void avl_update_height(AVLNode *n) {
    n->height = 1 + avl_max(avl_height(n->left), avl_height(n->right));
}

static int avl_bf(AVLNode *n) {
    return n ? avl_height(n->left) - avl_height(n->right) : 0;
}

static AVLNode *avl_rotate_right(AVLNode *y) {
    AVLNode *x = y->left, *T2 = x->right;
    x->right = y; y->left = T2;
    avl_update_height(y); avl_update_height(x);
    return x;
}

static AVLNode *avl_rotate_left(AVLNode *x) {
    AVLNode *y = x->right, *T2 = y->left;
    y->left = x; x->right = T2;
    avl_update_height(x); avl_update_height(y);
    return y;
}

static AVLNode *avl_balance(AVLNode *n) {
    avl_update_height(n);
    int bf = avl_bf(n);
    if (bf > 1) {
        if (avl_bf(n->left) < 0) n->left = avl_rotate_left(n->left);
        return avl_rotate_right(n);
    }
    if (bf < -1) {
        if (avl_bf(n->right) > 0) n->right = avl_rotate_right(n->right);
        return avl_rotate_left(n);
    }
    return n;
}

AVLNode *avl_insert(AVLNode *node, int key) {
    if (!node) return avl_new(key);
    if (key < node->key)       node->left  = avl_insert(node->left,  key);
    else if (key > node->key)  node->right = avl_insert(node->right, key);
    else return node; /* duplicates ignored */
    return avl_balance(node);
}

static AVLNode *avl_min_node(AVLNode *n) {
    while (n->left) n = n->left;
    return n;
}

AVLNode *avl_delete(AVLNode *root, int key) {
    if (!root) return NULL;
    if      (key < root->key) root->left  = avl_delete(root->left,  key);
    else if (key > root->key) root->right = avl_delete(root->right, key);
    else {
        if (!root->left || !root->right) {
            AVLNode *tmp = root->left ? root->left : root->right;
            free(root); return tmp;
        }
        AVLNode *succ = avl_min_node(root->right);
        root->key   = succ->key;
        root->right = avl_delete(root->right, succ->key);
    }
    return avl_balance(root);
}

int avl_search(AVLNode *root, int key) {
    if (!root) return 0;
    if (key == root->key) return 1;
    return key < root->key ? avl_search(root->left, key)
                           : avl_search(root->right, key);
}

void avl_inorder(AVLNode *root) {
    if (!root) return;
    avl_inorder(root->left);
    printf("%d(h%d) ", root->key, root->height);
    avl_inorder(root->right);
}

void avl_free(AVLNode *root) {
    if (!root) return;
    avl_free(root->left); avl_free(root->right); free(root);
}

/* ================================================================
 *  PART 2 – HASH TABLE  (open addressing, double hashing)
 *  Supports insert, search, delete, dynamic resize at load > 0.7
 * ================================================================ */

#define HT_INIT_CAP   16
#define HT_LOAD_MAX   0.7

typedef enum { EMPTY, OCCUPIED, DELETED } Slot;

typedef struct {
    int   key;
    int   value;
    Slot  state;
} HEntry;

typedef struct {
    HEntry *table;
    int     capacity;
    int     count;
} HashTable;

static int ht_h1(int key, int cap) { return ((unsigned)key * 2654435761u) % cap; }
static int ht_h2(int key, int cap) {
    int h = 1 + ((unsigned)key * 40503u) % (cap - 1);
    return h % 2 == 0 ? h + 1 : h; /* keep h2 odd so gcd(h2,cap)==1 */
}

HashTable *ht_create(int capacity) {
    HashTable *ht = malloc(sizeof *ht);
    ht->capacity = capacity;
    ht->count    = 0;
    ht->table    = calloc(capacity, sizeof(HEntry));
    return ht;
}

static void ht_insert_raw(HashTable *ht, int key, int value);

static void ht_resize(HashTable *ht) {
    int old_cap = ht->capacity;
    HEntry *old = ht->table;
    ht->capacity *= 2;
    ht->table = calloc(ht->capacity, sizeof(HEntry));
    ht->count = 0;
    for (int i = 0; i < old_cap; i++)
        if (old[i].state == OCCUPIED)
            ht_insert_raw(ht, old[i].key, old[i].value);
    free(old);
}

static void ht_insert_raw(HashTable *ht, int key, int value) {
    if ((double)ht->count / ht->capacity >= HT_LOAD_MAX) ht_resize(ht);
    int h1 = ht_h1(key, ht->capacity);
    int h2 = ht_h2(key, ht->capacity);
    for (int i = 0; i < ht->capacity; i++) {
        int idx = (h1 + i * h2) % ht->capacity;
        if (ht->table[idx].state != OCCUPIED) {
            ht->table[idx] = (HEntry){key, value, OCCUPIED};
            ht->count++;
            return;
        }
        if (ht->table[idx].key == key) { ht->table[idx].value = value; return; }
    }
}

void ht_insert(HashTable *ht, int key, int value) { ht_insert_raw(ht, key, value); }

int  ht_search(HashTable *ht, int key, int *out_value) {
    int h1 = ht_h1(key, ht->capacity);
    int h2 = ht_h2(key, ht->capacity);
    for (int i = 0; i < ht->capacity; i++) {
        int idx = (h1 + i * h2) % ht->capacity;
        if (ht->table[idx].state == EMPTY) return 0;
        if (ht->table[idx].state == OCCUPIED && ht->table[idx].key == key) {
            if (out_value) *out_value = ht->table[idx].value;
            return 1;
        }
    }
    return 0;
}

int  ht_delete(HashTable *ht, int key) {
    int h1 = ht_h1(key, ht->capacity);
    int h2 = ht_h2(key, ht->capacity);
    for (int i = 0; i < ht->capacity; i++) {
        int idx = (h1 + i * h2) % ht->capacity;
        if (ht->table[idx].state == EMPTY) return 0;
        if (ht->table[idx].state == OCCUPIED && ht->table[idx].key == key) {
            ht->table[idx].state = DELETED;
            ht->count--;
            return 1;
        }
    }
    return 0;
}

void ht_free(HashTable *ht) { free(ht->table); free(ht); }

/* ================================================================
 *  PART 3 – MIN-HEAP
 *  Array-based binary heap: O(log n) insert & extract-min
 *  Also supports decrease-key (useful for Dijkstra)
 * ================================================================ */

#define HEAP_MAX 256

typedef struct {
    int data[HEAP_MAX];
    int size;
} MinHeap;

static void heap_swap(MinHeap *h, int i, int j) {
    int t = h->data[i]; h->data[i] = h->data[j]; h->data[j] = t;
}

static void heap_sift_up(MinHeap *h, int i) {
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h->data[p] > h->data[i]) { heap_swap(h, p, i); i = p; }
        else break;
    }
}

static void heap_sift_down(MinHeap *h, int i) {
    while (1) {
        int l = 2*i+1, r = 2*i+2, smallest = i;
        if (l < h->size && h->data[l] < h->data[smallest]) smallest = l;
        if (r < h->size && h->data[r] < h->data[smallest]) smallest = r;
        if (smallest == i) break;
        heap_swap(h, i, smallest); i = smallest;
    }
}

void heap_insert(MinHeap *h, int val) {
    if (h->size >= HEAP_MAX) { fprintf(stderr, "heap full\n"); return; }
    h->data[h->size++] = val;
    heap_sift_up(h, h->size - 1);
}

int heap_extract_min(MinHeap *h) {
    if (h->size == 0) { fprintf(stderr, "heap empty\n"); return -1; }
    int min = h->data[0];
    h->data[0] = h->data[--h->size];
    heap_sift_down(h, 0);
    return min;
}

void heap_decrease_key(MinHeap *h, int i, int new_val) {
    if (new_val > h->data[i]) return; /* not a decrease */
    h->data[i] = new_val;
    heap_sift_up(h, i);
}

void MinHeap_build(MinHeap *h, int *arr, int n) {
    h->size = n;
    memcpy(h->data, arr, n * sizeof(int));
    for (int i = n/2 - 1; i >= 0; i--) heap_sift_down(h, i);
}

/* ================================================================
 *  MAIN – test suite for all three structures
 * ================================================================ */

int main(void) {
    /* --- AVL Tree --- */
    printf("\n=== AVL Tree ===\n");
    AVLNode *root = NULL;
    int keys[] = {30,20,40,10,25,35,50,5,15};
    for (int i = 0; i < 9; i++) root = avl_insert(root, keys[i]);
    printf("Inorder: "); avl_inorder(root); printf("\n");
    printf("Root: %d (height %d)\n", root->key, root->height);
    printf("Search 25: %s\n", avl_search(root,25) ? "found" : "not found");
    root = avl_delete(root, 20);
    printf("After delete 20 – inorder: "); avl_inorder(root); printf("\n");
    avl_free(root);

    /* --- Hash Table --- */
    printf("\n=== Hash Table (double hashing) ===\n");
    HashTable *ht = ht_create(HT_INIT_CAP);
    for (int i = 1; i <= 10; i++) ht_insert(ht, i*7, i*100);
    int v;
    printf("Search key=35: %s\n", ht_search(ht,35,&v) ? "found" : "not found");
    printf("Value of key=35: %d\n", v);
    ht_delete(ht, 35);
    printf("After delete key=35: %s\n", ht_search(ht,35,&v) ? "found" : "not found");
    printf("Load: %d/%d\n", ht->count, ht->capacity);
    ht_free(ht);

    /* --- Min-Heap --- */
    printf("\n=== Min-Heap ===\n");
    MinHeap heap = {.size = 0};
    int arr[] = {9,4,7,1,8,3,6,2,5};
    MinHeap_build(&heap, arr, 9);
    printf("Heap-sort output: ");
    while (heap.size > 0) printf("%d ", heap_extract_min(&heap));
    printf("\n");

    heap_insert(&heap, 10);
    heap_insert(&heap, 3);
    heap_insert(&heap, 7);
    heap_decrease_key(&heap, 2, 1);
    printf("After insertions + decrease-key: ");
    while (heap.size > 0) printf("%d ", heap_extract_min(&heap));
    printf("\n");

    return 0;
}
