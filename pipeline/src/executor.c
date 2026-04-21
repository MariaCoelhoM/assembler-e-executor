#include "../include/executor.h"

static void update_flags(cpu_state_t *cpu) {
    cpu->flag_N = (cpu->AC & 0x80) ? 1 : 0;
    cpu->flag_Z = (cpu->AC == 0)   ? 1 : 0;
}

static int do_load_from_array(executor_t *ex, uint8_t *src) {
    memcpy(ex->mem, src, MEM_SIZE);
    return 1;
}

static void do_reset(executor_t *ex) {
    ex->cpu.AC     = 0;
    ex->cpu.PC     = 0;
    ex->cpu.IR     = 0;
    ex->cpu.MAR    = 0;
    ex->cpu.MDR    = 0;
    ex->cpu.flag_N = 0;
    ex->cpu.flag_Z = 1;
    ex->cpu.cycles = 0;
    ex->cpu.halted = 0;
}

static int do_step(executor_t *ex) {
    cpu_state_t *cpu = &ex->cpu;
    if (cpu->halted) return 0;

    /* FETCH */
    cpu->MAR = cpu->PC;
    cpu->MDR = ex->mem[cpu->MAR];
    cpu->IR  = cpu->MDR;
    cpu->PC++;
    cpu->cycles++;

    /* DECODE / EXECUTE */
    switch (cpu->IR) {
        case OP_NOP: break;
        case OP_STA:
            cpu->MAR = ex->mem[cpu->PC++];
            ex->mem[cpu->MAR] = cpu->AC;
            break;
        case OP_LDA:
            cpu->MAR = ex->mem[cpu->PC++];
            cpu->MDR = ex->mem[cpu->MAR];
            cpu->AC  = cpu->MDR;
            update_flags(cpu);
            break;
        case OP_ADD:
            cpu->MAR = ex->mem[cpu->PC++];
            cpu->MDR = ex->mem[cpu->MAR];
            cpu->AC  = (uint8_t)(cpu->AC + cpu->MDR);
            update_flags(cpu);
            break;
        case OP_OR:
            cpu->MAR = ex->mem[cpu->PC++];
            cpu->MDR = ex->mem[cpu->MAR];
            cpu->AC  = cpu->AC | cpu->MDR;
            update_flags(cpu);
            break;
        case OP_AND:
            cpu->MAR = ex->mem[cpu->PC++];
            cpu->MDR = ex->mem[cpu->MAR];
            cpu->AC  = cpu->AC & cpu->MDR;
            update_flags(cpu);
            break;
        case OP_NOT:
            cpu->AC = (uint8_t)(~cpu->AC);
            update_flags(cpu);
            break;
        case OP_JMP:
            cpu->MAR = ex->mem[cpu->PC];
            cpu->PC  = cpu->MAR;
            break;
        case OP_JN:
            cpu->MAR = ex->mem[cpu->PC++];
            if (cpu->flag_N) cpu->PC = cpu->MAR;
            break;
        case OP_JZ:
            cpu->MAR = ex->mem[cpu->PC++];
            if (cpu->flag_Z) cpu->PC = cpu->MAR;
            break;
        case OP_HLT:
            cpu->halted = 1;
            printf("[HLT] Execução encerrada após %llu ciclos.\n",
                   (unsigned long long)cpu->cycles);
            return 0;
        default:
            fprintf(stderr, "[CPU] Opcode desconhecido: 0x%02X no PC=0x%02X\n",
                    cpu->IR, (uint8_t)(cpu->PC - 1));
            cpu->halted = 1;
            return 0;
    }
    return 1;
}

static void do_run(executor_t *ex) {
    uint64_t limit = (uint64_t)MEM_SIZE * MEM_SIZE * 32;
    while (!ex->cpu.halted && ex->cpu.cycles < limit)
        do_step(ex);
    if (!ex->cpu.halted)
        printf("[AVISO] Limite de ciclos atingido sem HLT.\n");
}

static void do_print_state(executor_t *ex) {
    cpu_state_t *c = &ex->cpu;
    printf("\n┌──────────────────────────────────────────┐\n");
    printf("│         Estado Final da CPU Neander      │\n");
    printf("├─────────────────────┬────────────────────┤\n");
    printf("│ AC  = 0x%02X  (%4d)  │ bin: %c%c%c%c%c%c%c%c       │\n",
           c->AC, (int8_t)c->AC,
           (c->AC>>7)&1?'1':'0', (c->AC>>6)&1?'1':'0',
           (c->AC>>5)&1?'1':'0', (c->AC>>4)&1?'1':'0',
           (c->AC>>3)&1?'1':'0', (c->AC>>2)&1?'1':'0',
           (c->AC>>1)&1?'1':'0', (c->AC>>0)&1?'1':'0');
    printf("│ PC  = 0x%02X          │ IR  = 0x%02X         │\n", c->PC, c->IR);
    printf("│ MAR = 0x%02X          │ MDR = 0x%02X         │\n", c->MAR, c->MDR);
    printf("│ N=%d  Z=%d            │ Ciclos: %-10llu │\n",
           c->flag_N, c->flag_Z, (unsigned long long)c->cycles);
    printf("└─────────────────────┴────────────────────┘\n");
}

static void do_dump_mem(executor_t *ex) {
    printf("\n=== Dump de Memória (Executor) ===\n");
    printf("     "); for(int c=0;c<16;c++) printf(" %02X",c);
    printf("\n     "); for(int c=0;c<16;c++) printf(" --"); printf("\n");
    for(int r=0;r<16;r++) {
        printf("%02X | ", r*16);
        for(int c=0;c<16;c++) printf(" %02X", ex->mem[r*16+c]);
        printf("\n");
    }
}

void init_executor(executor_t *ex) {
    memset(ex->mem, 0, MEM_SIZE);
    do_reset(ex);
    ex->load_from_array = do_load_from_array;
    ex->reset           = do_reset;
    ex->step            = do_step;
    ex->run             = do_run;
    ex->print_state     = do_print_state;
    ex->dump_mem        = do_dump_mem;
}
