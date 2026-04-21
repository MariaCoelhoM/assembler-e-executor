#include "../include/parser.h"

static node_t *parse_fn   (parser_t *p, tokenizer_t *t, char *str);
static void    validate_fn(parser_t *p);

static node_t *create_node(token_t *tok);
static node_t *match(parser_t *p, tokenizer_t *t, type_t type);
static node_t *numeric_literal(parser_t *p, tokenizer_t *t);
static node_t *binary_expression(parser_t *p, tokenizer_t *t, func_ptr f, type_t type);
static node_t *parenthesis_expression(parser_t *p, tokenizer_t *t);
static node_t *primary_expression(parser_t *p, tokenizer_t *t);
static node_t *unary_expression(parser_t *p, tokenizer_t *t);
static node_t *multiplicative_expression(parser_t *p, tokenizer_t *t);
static node_t *additive_expression(parser_t *p, tokenizer_t *t);
static node_t *expression(parser_t *p, tokenizer_t *t);
static node_t *program(parser_t *p, tokenizer_t *t);

void init_parser(parser_t *p) {
    p->parse    = parse_fn;
    p->validate = validate_fn;
    p->valid    = 1;
}

static node_t *parse_fn(parser_t *p, tokenizer_t *t, char *str) {
    t->load(t, str);
    if (!t->str) {
        printf("[PARSER] Erro de sintaxe: token inválido\n");
        p->valid = 0;
        p->lookahead = NULL;
        return NULL;
    }
    p->lookahead = t->get_next_token(t);
    return program(p, t);
}

static void validate_fn(parser_t *p) {
    if (p->lookahead) {
        printf("[PARSER] Erro de sintaxe: token inesperado\n");
        p->valid = 0;
        free(p->lookahead);
    }
}

static node_t *create_node(token_t *tok) {
    node_t *n = (node_t*)malloc(sizeof(node_t));
    n->value = tok->value;
    n->left  = n->right = NULL;
    n->type  = tok->type;
    free(tok);
    return n;
}

static node_t *match(parser_t *p, tokenizer_t *t, type_t type) {
    token_t *tok = p->lookahead;
    if (!tok) { printf("[PARSER] Erro: final inesperado\n"); p->valid = 0; return NULL; }
    if (tok->type != type) { printf("[PARSER] Erro: token inesperado\n"); p->valid = 0; return NULL; }
    p->lookahead = t->get_next_token(t);
    if (type == PARENTHESIS) { free(tok); return NULL; }
    return create_node(tok);
}

static node_t *numeric_literal(parser_t *p, tokenizer_t *t) {
    return match(p, t, NUMBER);
}

static node_t *parenthesis_expression(parser_t *p, tokenizer_t *t) {
    match(p, t, PARENTHESIS);
    node_t *exp = NULL;
    if (p->lookahead && p->lookahead->value != ')')
        exp = expression(p, t);
    else
        p->valid = 0;
    match(p, t, PARENTHESIS);
    return exp;
}

static node_t *primary_expression(parser_t *p, tokenizer_t *t) {
    if (p->lookahead->type == PARENTHESIS)
        return parenthesis_expression(p, t);
    return numeric_literal(p, t);
}

static node_t *unary_expression(parser_t *p, tokenizer_t *t) {
    node_t *op = NULL;
    if (!p->lookahead) { printf("[PARSER] Erro: final inesperado\n"); p->valid = 0; return NULL; }
    if (p->lookahead->type == ADDITIVE_OPERATOR)
        op = match(p, t, ADDITIVE_OPERATOR);
    if (op) {
        op->type  = UNARY_OPERATOR;
        op->left  = unary_expression(p, t);
        return op;
    }
    return primary_expression(p, t);
}

static node_t *binary_expression(parser_t *p, tokenizer_t *t, func_ptr f, type_t type) {
    node_t *left = f(p, t);
    while (p->lookahead && p->lookahead->type == type) {
        node_t *op    = match(p, t, type);
        node_t *right = f(p, t);
        op->left  = left;
        op->right = right;
        left = op;
    }
    return left;
}

static node_t *multiplicative_expression(parser_t *p, tokenizer_t *t) {
    return binary_expression(p, t, unary_expression, MULTIPLICATION_OPERATOR);
}

static node_t *additive_expression(parser_t *p, tokenizer_t *t) {
    return binary_expression(p, t, multiplicative_expression, ADDITIVE_OPERATOR);
}

static node_t *expression(parser_t *p, tokenizer_t *t) {
    return additive_expression(p, t);
}

static node_t *program(parser_t *p, tokenizer_t *t) {
    return expression(p, t);
}
