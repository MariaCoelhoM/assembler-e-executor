#include "../include/frontend.h"
#include "../include/tokenizer.h"
#include "../include/parser.h"
#include "../include/tree.h"
#include "../include/codegen.h"
#include "../include/assembler.h"
#include "../include/executor.h"

/*
 * ══════════════════════════════════════════════════════════════
 *  Pipeline completo:
 *
 *   arquivo.ndr
 *       │
 *       ▼
 *   [FRONTEND]  lê o .ndr, extrai expressão e variável destino
 *       │
 *       ▼
 *   [TOKENIZER] converte expressão em tokens
 *       │
 *       ▼
 *   [PARSER]    constrói a AST (árvore sintática)
 *       │
 *       ▼
 *   [CODEGEN]   percorre a AST e emite assembly Neander (.asm)
 *       │
 *       ▼
 *   [ASSEMBLER] duas passagens: tabela de símbolos + código (.mem)
 *       │
 *       ▼
 *   [EXECUTOR]  simula a CPU Neander e exibe o resultado
 * ══════════════════════════════════════════════════════════════ */

static void print_banner(void) {
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║   Pipeline NDR → Neander  (UFRGS / CompArq)     ║\n");
    printf("╚══════════════════════════════════════════════════╝\n\n");
}

static void print_usage(const char *prog) {
    printf("Uso: %s <arquivo.ndr> [--verbose]\n", prog);
    printf("  --verbose   mostra AST, código gerado e dump de memória\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) { print_usage(argv[0]); return 1; }

    const char *ndr_file = argv[1];
    int verbose = (argc >= 3 && strcmp(argv[2], "--verbose") == 0);

    /* Deriva nomes de arquivos intermediários do nome base */
    char asm_file[256], mem_file[256], base[256];
    strncpy(base, ndr_file, 255);
    /* Remove extensão */
    char *dot = strrchr(base, '.');
    if (dot) *dot = '\0';
    snprintf(asm_file, 255, "%s.asm", base);
    snprintf(mem_file, 255, "%s.mem", base);

    print_banner();

    /* ══════════════════════════════════════════════════════════
     *  ETAPA 1 — Frontend: lê o arquivo .ndr
     * ══════════════════════════════════════════════════════════ */
    printf("━━━ Etapa 1 / 5 ─ Frontend (.ndr) ━━━━━━━━━━━━━━━━━━\n");
    frontend_t fe;
    init_frontend(&fe);
    if (!fe.load(&fe, ndr_file)) {
        fprintf(stderr, "Pipeline abortado: erro no frontend.\n");
        return 1;
    }
    fe.print(&fe);

    /* ══════════════════════════════════════════════════════════
     *  ETAPA 2 — Parser: tokeniza e constrói a AST
     * ══════════════════════════════════════════════════════════ */
    printf("━━━ Etapa 2 / 5 ─ Parser (AST) ━━━━━━━━━━━━━━━━━━━━━\n");
    tokenizer_t tokenizer;
    parser_t    parser;
    tree_t      ast;

    init_tokenizer(&tokenizer);
    init_parser(&parser);
    init_tree(&ast);

    ast.root = parser.parse(&parser, &tokenizer, fe.prog.expr);
    parser.validate(&parser);

    if (!parser.valid || !ast.root) {
        fprintf(stderr, "Pipeline abortado: erro no parser.\n");
        return 1;
    }
    printf("AST construída com sucesso para: %s\n", fe.prog.expr);

    if (verbose) {
        printf("\n--- AST (pré-ordem) ---\n");
        ast.print(&ast);
    }

    /* Avalia para exibir resultado esperado */
    {
        /* Faz uma cópia da árvore só para avaliação (eval modifica valores) */
        printf("\nResultado esperado (avaliação direta): ");
        ast.eval(&ast);
        /* Re-parse para gerar código com a árvore original */
        init_parser(&parser);
        ast.root = parser.parse(&parser, &tokenizer, fe.prog.expr);
        parser.validate(&parser);
    }

    /* ══════════════════════════════════════════════════════════
     *  ETAPA 3 — Gerador de Código: AST → Assembly Neander
     * ══════════════════════════════════════════════════════════ */
    printf("\n━━━ Etapa 3 / 5 ─ Gerador de Código (.asm) ━━━━━━━━━\n");
    codegen_t cg;
    init_codegen(&cg);

    if (!cg.generate(&cg, ast.root, fe.prog.varname)) {
        fprintf(stderr, "Pipeline abortado: erro no gerador de código.\n");
        ast.free_all(&ast);
        tokenizer.free_str(&tokenizer);
        return 1;
    }

    cg.save(&cg, asm_file, fe.prog.varname);
    if (verbose) cg.print(&cg);

    ast.free_all(&ast);
    tokenizer.free_str(&tokenizer);

    /* ══════════════════════════════════════════════════════════
     *  ETAPA 4 — Assembler: .asm → .mem (duas passagens)
     * ══════════════════════════════════════════════════════════ */
    printf("\n━━━ Etapa 4 / 5 ─ Assembler (.mem) ━━━━━━━━━━━━━━━━━\n");
    assembler_t as;
    init_assembler(&as);

    if (!as.assemble(&as, asm_file)) {
        fprintf(stderr, "Pipeline abortado: erro na montagem.\n");
        return 1;
    }
    as.save_mem(&as, mem_file);
    if (verbose) as.dump_mem(&as);

    /* ══════════════════════════════════════════════════════════
     *  ETAPA 5 — Executor: simula a CPU Neander
     * ══════════════════════════════════════════════════════════ */
    printf("\n━━━ Etapa 5 / 5 ─ Executor (CPU Neander) ━━━━━━━━━━━\n");
    executor_t ex;
    init_executor(&ex);
    ex.load_from_array(&ex, as.mem);
    ex.run(&ex);
    ex.print_state(&ex);

    if (verbose) ex.dump_mem(&ex);

    /* Resultado final: lê o endereço da variável de saída */
    uint8_t result_addr;
    char varname_upper[MAX_VAR];
    str_upper(varname_upper, fe.prog.varname);
    if (as.sym.lookup(&as.sym, varname_upper, &result_addr)) {
        int8_t result_signed = (int8_t)ex.mem[result_addr];
        printf("\n┌────────────────────────────────────────────┐\n");
        printf("│  Expressão : %-30s│\n", fe.prog.expr);
        printf("│  Variável  : %-30s│\n", fe.prog.varname);
        printf("│  Resultado : %-30d│\n", (int)result_signed);
        printf("│  Endereço  : 0x%02X  (valor bruto: 0x%02X)     │\n",
               result_addr, ex.mem[result_addr]);
        printf("└────────────────────────────────────────────┘\n");
    }

    printf("\nArquivos gerados:\n");
    printf("  %-20s  (código assembly Neander)\n", asm_file);
    printf("  %-20s  (imagem binária de memória)\n", mem_file);

    return 0;
}
