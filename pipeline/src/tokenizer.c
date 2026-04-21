#include "../include/tokenizer.h"

static void     load           (tokenizer_t *t, char *str);
static token_t *get_next_token (tokenizer_t *t);
static void     free_str       (tokenizer_t *t);
static int has_more_tokens(tokenizer_t *t);
static token_t *do_operator(tokenizer_t *t, type_t type);
static token_t *do_num(tokenizer_t *t);

static int is_whitespace(char c)    { return c == ' ' || c == '\t' || c == 10; }
static int is_additive(char c)      { return c == '+' || c == '-'; }
static int is_multiplication(char c){ return c == '/' || c == '*' || c == '%'; }
static int is_num(char c)           { return c >= '0' && c <= '9'; }
static int is_parenthesis(char c)   { return c == '(' || c == ')'; }

static int is_valid(char c) {
    return is_whitespace(c) || is_additive(c) || is_multiplication(c)
        || is_num(c) || is_parenthesis(c);
}

static int validate(char *str) {
    for (int i = 0; str[i]; i++)
        if (!is_valid(str[i])) return 0;
    return 1;
}

static void load(tokenizer_t *t, char *str) {
    size_t len = strlen(str);
    if (validate(str)) {
        t->str = (char*)malloc((len + 1) * sizeof(char));
        strcpy(t->str, str);
    } else {
        t->str = NULL;
    }
    t->cursor = 0;
}

static token_t *get_next_token(tokenizer_t *t) {
    if (!has_more_tokens(t)) return NULL;

    char c = t->str[t->cursor];
    if (is_num(c))            return do_num(t);
    if (is_additive(c))       return do_operator(t, ADDITIVE_OPERATOR);
    if (is_multiplication(c)) return do_operator(t, MULTIPLICATION_OPERATOR);
    if (is_parenthesis(c))    return do_operator(t, PARENTHESIS);
    if (is_whitespace(c))     { t->cursor++; return get_next_token(t); }
    return NULL;
}

static void free_str(tokenizer_t *t) { free(t->str); }

static int has_more_tokens(tokenizer_t *t) {
    return t->cursor < (int)strlen(t->str);
}

static token_t *do_operator(tokenizer_t *t, type_t type) {
    token_t *tok = (token_t*)malloc(sizeof(token_t));
    tok->type  = type;
    tok->value = (long int)t->str[t->cursor++];
    return tok;
}

static token_t *do_num(tokenizer_t *t) {
    token_t *tok = (token_t*)malloc(sizeof(token_t));
    tok->type = NUMBER;
    size_t len = strlen(t->str);
    char *buf = (char*)malloc((len + 1) * sizeof(char));
    int count = 0;
    while (is_num(t->str[t->cursor]))
        buf[count++] = t->str[t->cursor++];
    buf[count] = '\0';
    char *end;
    tok->value = strtol(buf, &end, 10);
    free(buf);
    return tok;
}

void init_tokenizer(tokenizer_t *t) {
    t->load           = load;
    t->get_next_token = get_next_token;
    t->free_str       = free_str;
}
