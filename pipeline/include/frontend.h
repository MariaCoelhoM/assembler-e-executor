#ifndef FRONTEND_H
#define FRONTEND_H

/*
 * ══════════════════════════════════════════════════════════════
 *  FRONTEND  –  Lê arquivo .ndr e extrai a expressão matemática
 *
 *  Sintaxe da linguagem NDR (Neander Description Language):
 *
 *    -- comentário
 *    calcular <expressão> -> <nome_variável>
 *
 *  Exemplo:
 *    calcular (3 + 5) * 2 -> resultado
 * ══════════════════════════════════════════════════════════════
 */

#include "common.h"

#define MAX_EXPR  512
#define MAX_VAR   64

typedef struct ndr_program {
    char expr[MAX_EXPR];   /* expressão matemática extraída   */
    char varname[MAX_VAR]; /* nome da variável de destino      */
    int  valid;
} ndr_program_t;

typedef struct frontend {
    ndr_program_t prog;
    /* MÉTODOS */
    int  (*load)(struct frontend*, const char *filename);
    void (*print)(struct frontend*);
} frontend_t;

void init_frontend(frontend_t *fe);

#endif
