#ifndef PARSER_H
#define PARSER_H

#include "tree.h"
#include "tokenizer.h"

typedef struct parser {
    token_t *lookahead;
    int      valid;
    /* MÉTODOS */
    node_t *(*parse)   (struct parser*, tokenizer_t*, char*);
    void    (*validate)(struct parser*);
} parser_t;

typedef node_t *(func_ptr)(parser_t*, tokenizer_t*);

void init_parser(parser_t *parser);

#endif
