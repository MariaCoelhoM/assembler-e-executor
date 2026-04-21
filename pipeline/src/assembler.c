#include "../include/assembler.h"
#include <stdarg.h>

const instr_info_t INSTR_TABLE[] = {
    { "NOP", OP_NOP, ADDR_IMPLICIT },
    { "STA", OP_STA, ADDR_DIRECT   },
    { "LDA", OP_LDA, ADDR_DIRECT   },
    { "ADD", OP_ADD, ADDR_DIRECT   },
    { "OR",  OP_OR,  ADDR_DIRECT   },
    { "AND", OP_AND, ADDR_DIRECT   },
    { "NOT", OP_NOT, ADDR_IMPLICIT },
    { "JMP", OP_JMP, ADDR_DIRECT   },
    { "JN",  OP_JN,  ADDR_DIRECT   },
    { "JZ",  OP_JZ,  ADDR_DIRECT   },
    { "HLT", OP_HLT, ADDR_IMPLICIT },
    { NULL,  0x00,   ADDR_IMPLICIT }
};

/* ── Tabela de Símbolos ─────────────────────────────────────── */
static int sym_add(symbol_table_t *st, const char *name, uint8_t addr) {
    uint8_t dummy;
    if (st->lookup(st, name, &dummy)) {
        fprintf(stderr, "[ASM] Rótulo duplicado: '%s'\n", name); return 0;
    }
    if (st->count >= MAX_SYMBOLS) {
        fprintf(stderr, "[ASM] Tabela de símbolos cheia.\n"); return 0;
    }
    strncpy(st->entries[st->count].name, name, MAX_LABEL - 1);
    st->entries[st->count].address = addr;
    st->count++;
    return 1;
}

static int sym_lookup(symbol_table_t *st, const char *name, uint8_t *addr) {
    for (int i = 0; i < st->count; i++)
        if (strcmp(st->entries[i].name, name) == 0) {
            *addr = st->entries[i].address; return 1;
        }
    return 0;
}

static void sym_print(symbol_table_t *st) {
    printf("\n=== Tabela de Símbolos ===\n");
    printf("%-16s %s\n", "Rótulo", "Endereço");
    printf("%-16s %s\n", "------", "--------");
    for (int i = 0; i < st->count; i++)
        printf("%-16s 0x%02X (%d)\n",
               st->entries[i].name,
               st->entries[i].address,
               st->entries[i].address);
    printf("\n");
}

void init_symbol_table(symbol_table_t *st) {
    st->count  = 0;
    st->add    = sym_add;
    st->lookup = sym_lookup;
    st->print  = sym_print;
}

/* ── Helpers ────────────────────────────────────────────────── */
static const instr_info_t *find_instr(const char *m) {
    for (int i = 0; INSTR_TABLE[i].mnemonic; i++)
        if (strcmp(INSTR_TABLE[i].mnemonic, m) == 0)
            return &INSTR_TABLE[i];
    return NULL;
}

static void strip_comment(char *line) {
    char *p = strchr(line, ';');
    if (p) *p = '\0';
}

static int parse_line(const char *raw,
                      char *label, char *mnem, char *operand) {
    char line[512];
    strncpy(line, raw, 511); line[511] = '\0';
    strip_comment(line);
    trim(line);
    label[0] = mnem[0] = operand[0] = '\0';
    if (str_is_empty(line)) return 0;

    char *p = line;
    char tok1[64] = {0};
    int i = 0;
    while (*p && !isspace((unsigned char)*p) && *p != ':')
        tok1[i++] = *p++;
    tok1[i] = '\0';

    char up1[64];
    str_upper(up1, tok1);

    if (*p == ':') {
        strcpy(label, up1); p++;
        ltrim(p);
        i = 0; char tok2[64] = {0};
        while (*p && !isspace((unsigned char)*p)) tok2[i++] = *p++;
        tok2[i] = '\0';
        str_upper(mnem, tok2);
    } else {
        if (find_instr(up1) ||
            strcmp(up1,"DATA")==0 || strcmp(up1,"SPACE")==0 || strcmp(up1,"ORG")==0) {
            strcpy(mnem, up1);
        } else {
            strcpy(label, up1);
            ltrim(p);
            i = 0; char tok2[64] = {0};
            while (*p && !isspace((unsigned char)*p)) tok2[i++] = *p++;
            tok2[i] = '\0';
            str_upper(mnem, tok2);
        }
    }
    ltrim(p); rtrim(p);
    str_upper(operand, p);
    return 1;
}

static int parse_number(const char *s, int *out) {
    char *end;
    long v = (s[0]=='0'&&(s[1]=='x'||s[1]=='X'))
             ? strtol(s+2, &end, 16)
             : strtol(s,   &end, 10);
    if (*end && !isspace((unsigned char)*end)) return 0;
    *out = (int)v;
    return 1;
}

/* ── Passagem 1 ─────────────────────────────────────────────── */
static int first_pass(assembler_t *as, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) { fprintf(stderr,"[ASM] Erro: '%s'\n", filename); return 0; }
    char raw[512]; int lc=0, line=0, ok=1;
    while (fgets(raw, sizeof(raw), fp)) {
        line++;
        char label[64], mnem[64], operand[64];
        if (!parse_line(raw, label, mnem, operand)) continue;
        if (strcmp(mnem,"ORG")==0) {
            int a; if (parse_number(operand,&a)) lc=a; continue;
        }
        if (label[0]) {
            if (!as->sym.add(&as->sym, label, (uint8_t)lc)) { ok=0; }
        }
        if (!mnem[0]) continue;
        const instr_info_t *info = find_instr(mnem);
        if (info) {
            lc += (info->mode == ADDR_DIRECT) ? 2 : 1;
        } else if (strcmp(mnem,"DATA")==0) {
            lc += 1;
        } else if (strcmp(mnem,"SPACE")==0) {
            int n; if (parse_number(operand,&n) && n>0) lc += n;
        } else {
            fprintf(stderr,"[ASM] Linha %d: mnemônico desconhecido '%s'\n",line,mnem); ok=0;
        }
        if (lc > MEM_SIZE) { fprintf(stderr,"[ASM] Linha %d: overflow de memória\n",line); ok=0; break; }
    }
    fclose(fp); return ok;
}

/* ── Passagem 2 ─────────────────────────────────────────────── */
static int second_pass(assembler_t *as, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    char raw[512]; int lc=0, line=0, ok=1;
    while (fgets(raw, sizeof(raw), fp)) {
        line++;
        char label[64], mnem[64], operand[64];
        if (!parse_line(raw, label, mnem, operand)) continue;
        if (strcmp(mnem,"ORG")==0) { int a; if (parse_number(operand,&a)) lc=a; continue; }
        if (!mnem[0]) continue;
        const instr_info_t *info = find_instr(mnem);
        if (info) {
            as->mem[lc++] = info->opcode;
            if (info->mode == ADDR_DIRECT) {
                uint8_t addr_byte = 0; int num;
                if (parse_number(operand, &num)) {
                    addr_byte = (uint8_t)num;
                } else if (operand[0]) {
                    if (!as->sym.lookup(&as->sym, operand, &addr_byte)) {
                        fprintf(stderr,"[ASM] Linha %d: símbolo indefinido '%s'\n",line,operand); ok=0;
                    }
                } else {
                    fprintf(stderr,"[ASM] Linha %d: '%s' requer operando\n",line,mnem); ok=0;
                }
                as->mem[lc++] = addr_byte;
            }
        } else if (strcmp(mnem,"DATA")==0) {
            int val=0;
            if (operand[0]) {
                if (!parse_number(operand, &val)) {
                    uint8_t sa; if (as->sym.lookup(&as->sym,operand,&sa)) val=sa;
                    else { fprintf(stderr,"[ASM] Linha %d: operando inválido '%s'\n",line,operand); ok=0; }
                }
            }
            as->mem[lc++] = (uint8_t)val;
        } else if (strcmp(mnem,"SPACE")==0) {
            int n; if (parse_number(operand,&n)) lc += n;
        }
    }
    fclose(fp); return ok;
}

/* ── Métodos públicos ───────────────────────────────────────── */
static int do_assemble(assembler_t *as, const char *filename) {
    memset(as->mem, 0, MEM_SIZE);
    as->errors = 0;
    printf("\n=== Assembler Neander ===\n");
    printf("[Passagem 1] Construindo tabela de símbolos...\n");
    if (!first_pass(as, filename)) { as->errors++; return 0; }
    as->sym.print(&as->sym);
    printf("[Passagem 2] Gerando código de máquina...\n");
    if (!second_pass(as, filename)) { as->errors++; return 0; }
    printf("Montagem concluída!\n");
    return 1;
}

static void do_dump_mem(assembler_t *as) {
    printf("\n=== Dump de Memória ===\n");
    printf("     "); for(int c=0;c<16;c++) printf(" %02X",c);
    printf("\n     "); for(int c=0;c<16;c++) printf(" --"); printf("\n");
    for(int r=0;r<16;r++) {
        printf("%02X | ",r*16);
        for(int c=0;c<16;c++) printf(" %02X",as->mem[r*16+c]);
        printf("\n");
    }
}

static void do_save_mem(assembler_t *as, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) { fprintf(stderr,"[ASM] Erro ao salvar '%s'\n",filename); return; }
    fwrite(as->mem, 1, MEM_SIZE, fp);
    fclose(fp);
    printf("[ASM] Imagem salva em '%s'\n", filename);
}

void init_assembler(assembler_t *as) {
    memset(as->mem, 0, MEM_SIZE);
    as->errors   = 0;
    init_symbol_table(&as->sym);
    as->assemble = do_assemble;
    as->dump_mem = do_dump_mem;
    as->save_mem = do_save_mem;
}
