#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "common.h"

#define MAX_SYMBOLS 128
#define MAX_LABEL   64

typedef enum { ADDR_IMPLICIT, ADDR_DIRECT } addr_mode_t;

typedef struct {
    const char *mnemonic;
    uint8_t     opcode;
    addr_mode_t mode;
} instr_info_t;

extern const instr_info_t INSTR_TABLE[];

/* ── Tabela de Símbolos ─────────────────────────────────────── */
typedef struct {
    char    name[MAX_LABEL];
    uint8_t address;
} symbol_t;

typedef struct symbol_table {
    symbol_t entries[MAX_SYMBOLS];
    int      count;
    int  (*add)   (struct symbol_table*, const char*, uint8_t);
    int  (*lookup)(struct symbol_table*, const char*, uint8_t*);
    void (*print) (struct symbol_table*);
} symbol_table_t;

void init_symbol_table(symbol_table_t *st);

/* ── Assembler ──────────────────────────────────────────────── */
typedef struct assembler {
    symbol_table_t sym;
    uint8_t        mem[MEM_SIZE];
    int            errors;
    int  (*assemble)(struct assembler*, const char *filename);
    void (*dump_mem)(struct assembler*);
    void (*save_mem)(struct assembler*, const char *filename);
} assembler_t;

void init_assembler(assembler_t *as);

#endif
