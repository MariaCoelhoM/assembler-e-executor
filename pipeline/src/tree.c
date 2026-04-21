#include "../include/tree.h"

static void   free_tree (node_t *root);
static long   evaluate  (node_t *root, int *ok);
static void   print_tree(node_t *root, int depth);

static long mult_div(node_t *n, int *ok) {
    if (n->value == '*') return n->left->value * n->right->value;
    if (n->value == '/') {
        if (n->right->value == 0) { *ok = 0; return 0; }
        return n->left->value / n->right->value;
    }
    long r = n->right->value;
    return (n->left->value % r + r) % r;
}

static long add_sub(node_t *n) {
    return (n->value == '+') ? n->left->value + n->right->value
                             : n->left->value - n->right->value;
}

static long unary_eval(node_t *n) {
    return (n->value == '-') ? -n->left->value : n->left->value;
}

static long evaluate(node_t *root, int *ok) {
    if (!root) return 0;
    evaluate(root->left,  ok);
    evaluate(root->right, ok);
    if (root->type == MULTIPLICATION_OPERATOR) root->value = mult_div(root, ok);
    if (root->type == ADDITIVE_OPERATOR)       root->value = add_sub(root);
    if (root->type == UNARY_OPERATOR)          root->value = unary_eval(root);
    return root->value;
}

static void eval_fn(tree_t *tree) {
    int ok = 1;
    long result = evaluate(tree->root, &ok);
    if (ok) printf("%ld\n", result);
    else    printf("Erro: divisão por zero.\n");
}

static void print_tree(node_t *root, int depth) {
    if (!root) return;
    for (int i = 0; i < depth * 2; i++) printf(" ");
    if (root->type == NUMBER)
        printf("NUM(%ld)\n", root->value);
    else if (root->type == UNARY_OPERATOR)
        printf("UNARY(%c)\n", (char)root->value);
    else
        printf("OP(%c)\n", (char)root->value);
    print_tree(root->left,  depth + 1);
    print_tree(root->right, depth + 1);
}

static void print_fn(tree_t *tree) { print_tree(tree->root, 0); }

static void free_tree(node_t *root) {
    if (!root) return;
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

static void free_all_fn(tree_t *tree) { free_tree(tree->root); }

void init_tree(tree_t *tree) {
    tree->root     = NULL;
    tree->free_all = free_all_fn;
    tree->print    = print_fn;
    tree->eval     = eval_fn;
}
