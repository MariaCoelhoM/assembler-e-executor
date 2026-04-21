#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "common.h"

typedef enum type {
    NUMBER,
    PARENTHESIS,
    ADDITIVE_OPERATOR,
    MULTIPLICATION_OPERATOR,
    UNARY_OPERATOR,
} type_t;

typedef struct token {
    type_t   type;
    long int value;
} token_t;

typedef struct tokenizer {
    char    *str;
    int      cursor;
    /* MÉTODOS */
    void     (*load)          (struct tokenizer*, char*);
    void     (*free_str)      (struct tokenizer*);
    token_t *(*get_next_token)(struct tokenizer*);
} tokenizer_t;

void init_tokenizer(tokenizer_t *tokenizer);

#endif
