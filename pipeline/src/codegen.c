#include "../include/codegen.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/*
 * ══════════════════════════════════════════════════════════════
 *  GERADOR DE CÓDIGO  –  AST → Assembly Neander
 *
 *  Estratégia de geração (pós-ordem na AST):
 *
 *  Para cada nó:
 *    NUMBER       → apenas registra DATA
 *    UNARY (-)    → gera NOT + ADD 1 (complemento de 2)
 *    ADD/SUB/MUL/DIV → gera sequência LDA/ADD/STA em temporários
 *
 *  Como a Neander tem apenas AC e memória, toda operação binária
 *  usa o padrão:
 *    LDA  <esquerda>
 *    ADD  <direita>    (ou NOT + ajuste para sub/mul)
 *    STA  <temp_novo>
 *
 *  Multiplicação: loop de soma repetida (A * B = somar A, B vezes)
 *  Subtração    : A - B = A + (~B + 1)  via complemento de 2
 * ══════════════════════════════════════════════════════════════ */

/* ── Auxiliares internos ──────────────────────────────────────*/

static void emit(codegen_t *cg, const char *fmt, ...) {
    if (cg->line_count >= MAX_LINES) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(cg->lines[cg->line_count++], MAX_LINE, fmt, ap);
    va_end(ap);
}

static void emit_data(codegen_t *cg, const char *fmt, ...) {
    if (cg->data_count >= MAX_TEMPS) return;
    va_list ap2;
    va_start(ap2, fmt);
    vsnprintf(cg->data[cg->data_count++], MAX_LINE, fmt, ap2);
    va_end(ap2);
}

/* Cria novo temporário e retorna seu nome */
static void new_temp(codegen_t *cg, char *out) {
    snprintf(out, 16, "T%d", cg->temp_count++);
}

/*
 * Percurso recursivo pós-ordem.
 * Retorna o nome da variável temporária que contém o resultado do
 * sub-nó atual.
 */
static void gen_node(codegen_t *cg, node_t *node, char *result_var) {

    if (!node) { result_var[0] = '\0'; return; }

    /* ── Folha: número ──────────────────────────────────────── */
    if (node->type == NUMBER) {
        new_temp(cg, result_var);
        /* Valores > 127 são armazenados como unsigned 8-bit */
        int val = (int)(node->value & 0xFF);
        emit_data(cg, "%-12s DATA %d", result_var, val);
        return;
    }

    /* ── Operador unário: - ─────────────────────────────────── */
    if (node->type == UNARY_OPERATOR) {
        char left_var[16];
        gen_node(cg, node->left, left_var);

        if (node->value == '-') {
            /* Complemento de 2: NOT + ADD 1 */
            char neg1[16], tmp[16];
            new_temp(cg, neg1);
            new_temp(cg, tmp);

            /* constante 1 para o complemento */
            emit_data(cg, "%-12s DATA 1", neg1);
            emit(cg,  "        ; -- negacao de %s --", left_var);
            emit(cg,  "        LDA  %s", left_var);
            emit(cg,  "        NOT");
            emit(cg,  "        ADD  %s", neg1);    /* ~x + 1 = -x */
            emit(cg,  "        STA  %s", tmp);
            strcpy(result_var, tmp);
        } else {
            /* unário + : identidade */
            strcpy(result_var, left_var);
        }
        return;
    }

    /* ── Operador binário ───────────────────────────────────── */
    char left_var[16], right_var[16];
    gen_node(cg, node->left,  left_var);
    gen_node(cg, node->right, right_var);

    char tmp[16];
    new_temp(cg, tmp);

    switch ((char)node->value) {

        case '+':
            emit(cg, "        ; -- %s + %s --", left_var, right_var);
            emit(cg, "        LDA  %s", left_var);
            emit(cg, "        ADD  %s", right_var);
            emit(cg, "        STA  %s", tmp);
            emit_data(cg, "%-12s DATA 0", tmp);
            break;

        case '-': {
            /*  A - B  =  A + (~B + 1)  */
            char neg1[16], negB[16];
            new_temp(cg, neg1);
            new_temp(cg, negB);
            emit_data(cg, "%-12s DATA 1",  neg1);
            emit_data(cg, "%-12s DATA 0",  negB);
            emit(cg, "        ; -- %s - %s --", left_var, right_var);
            emit(cg, "        LDA  %s",  right_var);
            emit(cg, "        NOT");
            emit(cg, "        ADD  %s",  neg1);   /* ~B + 1 = -B */
            emit(cg, "        STA  %s",  negB);
            emit(cg, "        LDA  %s",  left_var);
            emit(cg, "        ADD  %s",  negB);
            emit(cg, "        STA  %s",  tmp);
            emit_data(cg, "%-12s DATA 0", tmp);
            break;
        }

        case '*': {
            /*
             *  Multiplicação por soma repetida:
             *
             *  RESULTADO = 0
             *  CONTADOR  = B
             *  LOOP:
             *    JZ   FIM        ; se CONTADOR == 0, termina
             *    LDA  RESULTADO
             *    ADD  A
             *    STA  RESULTADO
             *    LDA  CONTADOR
             *    ADD  NEG1       ; CONTADOR = CONTADOR - 1
             *    STA  CONTADOR
             *    JMP  LOOP
             *  FIM:
             */
            char res[16], cnt[16], neg1[16], loop_lbl[16], fim_lbl[16];
            new_temp(cg, res);
            new_temp(cg, cnt);
            new_temp(cg, neg1);
            /* rótulos únicos para o loop */
            snprintf(loop_lbl, 16, "MLOOP%d", cg->temp_count);
            snprintf(fim_lbl,  16, "MFIM%d",  cg->temp_count);

            emit_data(cg, "%-12s DATA 0",   res);
            emit_data(cg, "%-12s DATA 0",   cnt);
            emit_data(cg, "%-12s DATA 255", neg1); /* -1 em 8 bits */

            emit(cg, "        ; -- %s * %s --", left_var, right_var);
            /* inicializa resultado = 0 e contador = B */
            emit(cg, "        LDA  %s",      right_var);
            emit(cg, "        STA  %s",      cnt);
            /* zera resultado */
            emit(cg, "        LDA  %s",      res);   /* já é 0 */

            emit(cg, "%s:", loop_lbl);
            emit(cg, "        LDA  %s",      cnt);
            emit(cg, "        JZ   %s",      fim_lbl);
            emit(cg, "        LDA  %s",      res);
            emit(cg, "        ADD  %s",      left_var);
            emit(cg, "        STA  %s",      res);
            emit(cg, "        LDA  %s",      cnt);
            emit(cg, "        ADD  %s",      neg1);
            emit(cg, "        STA  %s",      cnt);
            emit(cg, "        JMP  %s",      loop_lbl);
            emit(cg, "%s:",   fim_lbl);
            /* copia resultado para temporário de saída */
            emit(cg, "        LDA  %s",      res);
            emit(cg, "        STA  %s",      tmp);
            emit_data(cg, "%-12s DATA 0", tmp);
            break;
        }

        case '/': {
            /*
             *  Divisão inteira por subtração repetida:
             *  QUOCIENTE = 0
             *  DIVIDENDO = A  (cópia)
             *  LOOP:
             *    LDA  DIVIDENDO - DIVISOR
             *    JN   FIM           ; se < 0, termina
             *    STA  DIVIDENDO
             *    LDA  QUOCIENTE
             *    ADD  UM
             *    STA  QUOCIENTE
             *    JMP  LOOP
             *  FIM:
             */
            char quot[16], divid[16], neg1[16], um[16];
            char negB[16], loop_lbl[16], fim_lbl[16];
            new_temp(cg, quot);
            new_temp(cg, divid);
            new_temp(cg, neg1);
            new_temp(cg, um);
            new_temp(cg, negB);
            snprintf(loop_lbl, 16, "DLOOP%d", cg->temp_count);
            snprintf(fim_lbl,  16, "DFIM%d",  cg->temp_count);

            emit_data(cg, "%-12s DATA 0",   quot);
            emit_data(cg, "%-12s DATA 0",   divid);
            emit_data(cg, "%-12s DATA 255", neg1);
            emit_data(cg, "%-12s DATA 1",   um);
            emit_data(cg, "%-12s DATA 0",   negB);

            emit(cg, "        ; -- %s / %s --", left_var, right_var);
            /* -B (complemento de 2) */
            emit(cg, "        LDA  %s",  right_var);
            emit(cg, "        NOT");
            emit(cg, "        ADD  %s",  um);
            emit(cg, "        STA  %s",  negB);
            /* dividendo = A */
            emit(cg, "        LDA  %s",  left_var);
            emit(cg, "        STA  %s",  divid);

            emit(cg, "%s:", loop_lbl);
            emit(cg, "        LDA  %s",  divid);
            emit(cg, "        ADD  %s",  negB);   /* dividendo - divisor */
            emit(cg, "        JN   %s",  fim_lbl);
            emit(cg, "        STA  %s",  divid);
            emit(cg, "        LDA  %s",  quot);
            emit(cg, "        ADD  %s",  um);
            emit(cg, "        STA  %s",  quot);
            emit(cg, "        JMP  %s",  loop_lbl);
            emit(cg, "%s:",  fim_lbl);
            emit(cg, "        LDA  %s",  quot);
            emit(cg, "        STA  %s",  tmp);
            emit_data(cg, "%-12s DATA 0", tmp);
            break;
        }

        default:
            fprintf(stderr, "[CODEGEN] Operador desconhecido: '%c'\n",
                    (char)node->value);
            cg->valid = 0;
    }

    strcpy(result_var, tmp);
}

/* ── Métodos públicos ─────────────────────────────────────────*/

static int do_generate(codegen_t *cg, node_t *root, const char *varname) {
    cg->line_count = 0;
    cg->data_count = 0;
    cg->temp_count = 0;
    cg->valid      = 1;

    emit(cg, "        ORG  0");
    emit(cg, "");

    char result[16];
    gen_node(cg, root, result);

    if (!cg->valid) return 0;

    /* Armazena resultado na variável do .ndr */
    emit(cg, "        ; -- armazena resultado em %s --", varname);
    emit(cg, "        LDA  %s", result);
    emit(cg, "        STA  %s", varname);
    emit(cg, "        HLT");
    emit(cg, "");

    /* Seção de dados */
    emit(cg, "; ── Dados ───────────────────────────────────────");
    for (int i = 0; i < cg->data_count; i++)
        emit(cg, "%s", cg->data[i]);
    emit(cg, "%-12s DATA 0", varname);

    return 1;
}

static void do_save(codegen_t *cg, const char *filename,
                    const char *varname) {
    (void)varname;
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "[CODEGEN] Erro: não foi possível criar '%s'\n", filename);
        return;
    }
    fprintf(fp, "; Gerado automaticamente pelo compilador NDR\n");
    fprintf(fp, "; Pipeline: .ndr -> parser -> AST -> codegen -> .asm\n\n");
    for (int i = 0; i < cg->line_count; i++)
        fprintf(fp, "%s\n", cg->lines[i]);
    fclose(fp);
    printf("[CODEGEN] Arquivo '%s' gerado (%d linhas)\n",
           filename, cg->line_count);
}

static void do_print(codegen_t *cg) {
    printf("\n=== Código Assembly Gerado ===\n");
    for (int i = 0; i < cg->line_count; i++)
        printf("%s\n", cg->lines[i]);
}

void init_codegen(codegen_t *cg) {
    cg->line_count  = 0;
    cg->data_count  = 0;
    cg->temp_count  = 0;
    cg->valid       = 1;
    cg->generate    = do_generate;
    cg->save        = do_save;
    cg->print       = do_print;
}
