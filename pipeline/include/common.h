#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

/* ── Tamanho da memória Neander ─────────────────────────────── */
#define MEM_SIZE 256

/* ── Opcodes Neander ────────────────────────────────────────── */
#define OP_NOP  0x00
#define OP_STA  0x10
#define OP_LDA  0x20
#define OP_ADD  0x30
#define OP_OR   0x50
#define OP_AND  0x60
#define OP_NOT  0x70
#define OP_JMP  0x80
#define OP_JN   0x90
#define OP_JZ   0xA0
#define OP_HLT  0xF0

/* ── Utilitários de string ──────────────────────────────────── */
void str_upper(char *dst, const char *src);
int  str_is_empty(const char *s);
void trim(char *s);
void ltrim(char *s);
void rtrim(char *s);

#endif
