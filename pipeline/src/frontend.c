#include "../include/frontend.h"

/* ══════════════════════════════════════════════════════════════
 *  Lê um arquivo .ndr e extrai a expressão e o nome da variável
 *
 *  Regras da linguagem NDR:
 *    - Linhas começando com '--' são comentários (ignoradas)
 *    - Linhas em branco são ignoradas
 *    - Instrução válida: calcular <expr> -> <var>
 * ══════════════════════════════════════════════════════════════ */

static int do_load(frontend_t *fe, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "[FRONTEND] Erro: não foi possível abrir '%s'\n", filename);
        fe->prog.valid = 0;
        return 0;
    }

    char line[1024];
    int  found = 0;
    int  lineno = 0;

    while (fgets(line, sizeof(line), fp)) {
        lineno++;
        /* Remove newline */
        line[strcspn(line, "\n")] = '\0';
        trim(line);

        /* Ignora linhas vazias e comentários */
        if (str_is_empty(line))       continue;
        if (strncmp(line, "--", 2) == 0) continue;

        /* Espera: calcular <expr> -> <var> */
        if (strncmp(line, "calcular", 8) != 0) {
            fprintf(stderr, "[FRONTEND] Linha %d: palavra-chave 'calcular' esperada, encontrado: '%s'\n",
                    lineno, line);
            fclose(fp);
            fe->prog.valid = 0;
            return 0;
        }

        /* Avança além de 'calcular' */
        char *rest = line + 8;
        ltrim(rest);

        /* Divide em <expr> e <var> pelo '->' */
        char *arrow = strstr(rest, "->");
        if (!arrow) {
            fprintf(stderr, "[FRONTEND] Linha %d: '->' não encontrado. Sintaxe: calcular <expr> -> <var>\n", lineno);
            fclose(fp);
            fe->prog.valid = 0;
            return 0;
        }

        /* Extrai expressão (antes do '->') */
        int expr_len = (int)(arrow - rest);
        if (expr_len <= 0 || expr_len >= MAX_EXPR) {
            fprintf(stderr, "[FRONTEND] Linha %d: expressão inválida ou muito longa\n", lineno);
            fclose(fp);
            fe->prog.valid = 0;
            return 0;
        }
        strncpy(fe->prog.expr, rest, expr_len);
        fe->prog.expr[expr_len] = '\0';
        rtrim(fe->prog.expr);

        /* Extrai nome da variável (depois do '->') */
        char *varpart = arrow + 2;
        ltrim(varpart);
        rtrim(varpart);
        if (str_is_empty(varpart)) {
            fprintf(stderr, "[FRONTEND] Linha %d: nome de variável ausente após '->'\n", lineno);
            fclose(fp);
            fe->prog.valid = 0;
            return 0;
        }
        strncpy(fe->prog.varname, varpart, MAX_VAR - 1);
        fe->prog.varname[MAX_VAR - 1] = '\0';

        found = 1;
        break; /* apenas uma instrução por arquivo por enquanto */
    }

    fclose(fp);

    if (!found) {
        fprintf(stderr, "[FRONTEND] Nenhuma instrução 'calcular' encontrada em '%s'\n", filename);
        fe->prog.valid = 0;
        return 0;
    }

    fe->prog.valid = 1;
    return 1;
}

static void do_print(frontend_t *fe) {
    printf("\n=== Frontend NDR ===\n");
    printf("Expressão : %s\n", fe->prog.expr);
    printf("Variável  : %s\n", fe->prog.varname);
    printf("Válido    : %s\n\n", fe->prog.valid ? "sim" : "não");
}

void init_frontend(frontend_t *fe) {
    fe->prog.valid    = 0;
    fe->prog.expr[0]  = '\0';
    fe->prog.varname[0] = '\0';
    fe->load  = do_load;
    fe->print = do_print;
}
