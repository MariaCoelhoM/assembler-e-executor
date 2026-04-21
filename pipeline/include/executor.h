#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "common.h"

typedef struct cpu_state {
    uint8_t  AC;
    uint8_t  PC;
    uint8_t  IR;
    uint8_t  MAR;
    uint8_t  MDR;
    int      flag_N;
    int      flag_Z;
    uint64_t cycles;
    int      halted;
} cpu_state_t;

typedef struct executor {
    cpu_state_t cpu;
    uint8_t     mem[MEM_SIZE];
    int  (*load_from_array)(struct executor*, uint8_t *src);
    void (*reset)          (struct executor*);
    int  (*step)           (struct executor*);
    void (*run)            (struct executor*);
    void (*print_state)    (struct executor*);
    void (*dump_mem)       (struct executor*);
} executor_t;

void init_executor(executor_t *ex);

#endif
