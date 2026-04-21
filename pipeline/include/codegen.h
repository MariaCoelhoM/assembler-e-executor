#ifndef CODEGEN_H
#define CODEGEN_H

/*
 * ══════════════════════════════════════════════════════════════
 *  GERADOR DE CÓDIGO  –  Percorre a AST e emite código Neander
 *
 *  Estratégia:
 *    - Percurso pós-ordem na AST
 *    - Cada número vira uma variável temporária (T0, T1, ...)
 *    - Cada operação vira LDA + ADD/NOT/AND/OR + STA em temp
 *    - O resultado final é armazenado na variável do .ndr
 * ══════════════════════════════════════════════════════════════
 */

#include "tree.h"

#define MAX_LINES  512
#define MAX_LINE   80
#define MAX_TEMPS  64

typedef struct codegen {
    char   lines[MAX_LINES][MAX_LINE]; /* linhas do .asm gerado  */
    int    line_count;
    char   data[MAX_TEMPS][MAX_LINE];  /* secção DATA            */
    int    data_count;
    int    temp_count;                 /* contador de temporários */
    int    valid;
    /* MÉTODOS */
    int  (*generate)(struct codegen*, node_t *root, const char *varname);
    void (*save)    (struct codegen*, const char *filename, const char *varname);
    void (*print)   (struct codegen*);
} codegen_t;

void init_codegen(codegen_t *cg);

#endif
