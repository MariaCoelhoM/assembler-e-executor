# Relatório — Pipeline NDR → Neander

---

## 1. Visão Geral do Pipeline

O projeto implementa, em C, um pipeline completo de 5 etapas que transforma um arquivo fonte `.ndr` (Neander Description Language) até a execução simulada na CPU Neander:

```
arquivo.ndr
    │
    ▼  Etapa 1 — Frontend     lê o .ndr e extrai a expressão e a variável de destino
    ▼  Etapa 2 — Parser       tokeniza a expressão e constrói a AST (Árvore Sintática)
    ▼  Etapa 3 — Codegen      percorre a AST e emite assembly Neander (.asm)
    ▼  Etapa 4 — Assembler    duas passagens: tabela de símbolos + código de máquina (.mem)
    ▼  Etapa 5 — Executor     simula a CPU Neander e exibe o resultado final
```

---

## 2. Como Compilar e Executar

### Pré-requisitos

- GCC (ou outro compilador C compatível com C99)
- `make`
- Sistema Unix/Linux (ou WSL no Windows)

### Compilação

```bash
# Dentro do diretório pipeline/
make
```

Isso compila todos os arquivos em `src/` e gera o executável `ndr`.

### Execução básica

```bash
./ndr <arquivo.ndr>
```

Exemplo:

```bash
./ndr tests/soma_simples.ndr
./ndr tests/expressao_complexa.ndr
```

### Execução detalhada (verbose)

A flag `--verbose` exibe a AST, o código assembly gerado e o dump completo da memória:

```bash
./ndr tests/soma_simples.ndr --verbose
./ndr tests/expressao_complexa.ndr --verbose
```

### Atalhos via Makefile

```bash
make test          # executa os dois testes em modo padrão
make test_verbose  # executa os dois testes com saída detalhada
make clean         # remove objetos, executável e arquivos intermediários
```

### Arquivos gerados

Após cada execução são gerados dois arquivos intermediários com o mesmo nome base do `.ndr`:

| Arquivo | Conteúdo |
|---|---|
| `<nome>.asm` | Código assembly Neander gerado pelo Codegen |
| `<nome>.mem` | Imagem binária de 256 bytes (imagem de memória) |

---

## 3. Estrutura de Diretórios

```
pipeline/
├── Makefile
├── ndr               (executável pré-compilado)
├── include/
│   ├── common.h      (opcodes, tamanho de memória, utilitários de string)
│   ├── assembler.h   (tipos: symbol_table_t, assembler_t)
│   ├── executor.h    (tipos: cpu_state_t, executor_t)
│   ├── frontend.h
│   ├── tokenizer.h
│   ├── parser.h
│   ├── tree.h
│   └── codegen.h
├── src/
│   ├── main.c        (orquestração das 5 etapas)
│   ├── common.c      (utilitários de string)
│   ├── frontend.c    (leitura do .ndr)
│   ├── tokenizer.c   (análise léxica)
│   ├── parser.c      (análise sintática / AST)
│   ├── tree.c        (operações na AST)
│   ├── codegen.c     (geração de assembly)
│   ├── assembler.c   (montador em duas passagens)
│   └── executor.c    (simulador da CPU Neander)
└── tests/
    ├── soma_simples.ndr          calcular (3 + 5) * 2 -> resultado
    └── expressao_complexa.ndr   calcular 10 + 4 * 3 - 2 -> resultado
```

---

## 4. O Assembler — Estrutura e Passagens

O assembler está implementado em `src/assembler.c` e `include/assembler.h`. Ele recebe um arquivo `.asm` (produzido pelo Codegen) e gera uma imagem de memória de 256 bytes.

### 4.1 Tabela de Símbolos

A tabela de símbolos mapeia **rótulos** (labels) a **endereços de memória** (1 byte, 0x00–0xFF).

```c
typedef struct {
    char    name[MAX_LABEL];  // nome do rótulo (até 64 caracteres)
    uint8_t address;          // endereço correspondente na memória
} symbol_t;

typedef struct symbol_table {
    symbol_t entries[MAX_SYMBOLS];  // até 128 entradas
    int      count;
    int  (*add)   (struct symbol_table*, const char*, uint8_t);
    int  (*lookup)(struct symbol_table*, const char*, uint8_t*);
    void (*print) (struct symbol_table*);
} symbol_table_t;
```

A tabela oferece três operações:

- **`add`** — insere um novo par `(rótulo, endereço)`, detectando duplicatas.
- **`lookup`** — busca linearmente um rótulo e retorna o endereço correspondente.
- **`print`** — imprime a tabela formatada durante a montagem (útil para depuração).

### 4.2 Tabela de Instruções

O assembler possui uma tabela estática com os 11 mnemônicos do Neander:

| Mnemônico | Opcode | Modo de Endereçamento |
|-----------|--------|----------------------|
| NOP | 0x00 | Implícito (1 byte) |
| STA | 0x10 | Direto (2 bytes) |
| LDA | 0x20 | Direto (2 bytes) |
| ADD | 0x30 | Direto (2 bytes) |
| OR  | 0x50 | Direto (2 bytes) |
| AND | 0x60 | Direto (2 bytes) |
| NOT | 0x70 | Implícito (1 byte) |
| JMP | 0x80 | Direto (2 bytes) |
| JN  | 0x90 | Direto (2 bytes) |
| JZ  | 0xA0 | Direto (2 bytes) |
| HLT | 0xF0 | Implícito (1 byte) |

Instruções de modo **implícito** ocupam 1 byte; instruções de modo **direto** ocupam 2 bytes (opcode + endereço do operando).

### 4.3 Diretivas de Montagem

Além dos mnemônicos, o assembler suporta três diretivas:

- **`ORG <endereço>`** — define o Contador de Localização (LC) para um endereço específico.
- **`DATA [valor]`** — reserva 1 byte com um valor inicial.
- **`SPACE <n>`** — reserva `n` bytes sem inicialização.

### 4.4 Passagem 1 — Construção da Tabela de Símbolos

A primeira passagem percorre o arquivo `.asm` **sem gerar código**, com o único objetivo de calcular e registrar os endereços de todos os rótulos.

Algoritmo da Passagem 1:

1. Inicializa o Contador de Localização (LC) em 0.
2. Para cada linha do arquivo:
   - Remove comentários (tudo após `;`) e espaços extras.
   - Se a diretiva for `ORG`, atualiza o LC com o endereço fornecido.
   - Se houver um **rótulo** na linha, registra o par `(rótulo, LC)` na tabela de símbolos.
   - Avança o LC de acordo com o tamanho da instrução ou diretiva:
     - Instrução com modo direto → LC += 2
     - Instrução com modo implícito → LC += 1
     - `DATA` → LC += 1
     - `SPACE n` → LC += n
3. Detecta overflow (LC > 255) e rótulos duplicados como erros.

Ao final da Passagem 1, a tabela de símbolos está completa e é impressa na saída.

### 4.5 Passagem 2 — Geração de Código de Máquina

A segunda passagem percorre novamente o arquivo `.asm` e, desta vez, **escreve os bytes na imagem de memória** (`mem[256]`):

1. Para cada instrução encontrada, escreve `mem[LC] = opcode` e avança o LC.
2. Para instruções com modo direto, o operando é resolvido:
   - Se for um número literal, converte diretamente para byte.
   - Se for um **símbolo**, consulta a tabela construída na Passagem 1 para obter o endereço.
   - Se o símbolo não existir, reporta erro de referência indefinida.
3. Para `DATA`, escreve o valor inicial no byte corrente.
4. Para `SPACE`, apenas avança o LC sem escrever.

Ao final, a imagem de memória é salva em disco como arquivo `.mem` (256 bytes brutos).

---

## 5. O Executor — Ciclo de Máquina e Flags

O executor está implementado em `src/executor.c` e `include/executor.h`. Ele simula a CPU Neander, que é uma arquitetura acumuladora de 8 bits.

### 5.1 Estado da CPU

```c
typedef struct cpu_state {
    uint8_t  AC;      // Acumulador (registrador de 8 bits)
    uint8_t  PC;      // Program Counter
    uint8_t  IR;      // Instruction Register
    uint8_t  MAR;     // Memory Address Register
    uint8_t  MDR;     // Memory Data Register
    int      flag_N;  // Flag Negativo (bit 7 do AC = 1)
    int      flag_Z;  // Flag Zero    (AC == 0)
    uint64_t cycles;  // contador de ciclos executados
    int      halted;  // indica se a CPU parou
} cpu_state_t;
```

O estado inicial após `reset` é: AC=0, PC=0, IR=0, MAR=0, MDR=0, flag_N=0, **flag_Z=1** (pois AC começa em 0), cycles=0, halted=0.

### 5.2 Ciclo de Máquina

Cada chamada a `do_step()` executa um ciclo completo composto pelas fases **Fetch** e **Decode/Execute**:

#### Fase FETCH

```
MAR ← PC
MDR ← mem[MAR]
IR  ← MDR
PC  ← PC + 1
cycles++
```

O opcode da próxima instrução é carregado no registrador IR e o PC é incrementado.

#### Fase DECODE/EXECUTE

O opcode em IR é decodificado via `switch` e a instrução é executada. O comportamento de cada instrução:

| Instrução | Operação realizada |
|---|---|
| **NOP** | Nenhuma operação. |
| **STA end** | `MAR ← mem[PC++]` ; `mem[MAR] ← AC` — armazena o acumulador na memória. |
| **LDA end** | `MAR ← mem[PC++]` ; `AC ← mem[MAR]` — carrega da memória para o AC. Atualiza flags. |
| **ADD end** | `MAR ← mem[PC++]` ; `AC ← AC + mem[MAR]` — soma e atualiza flags. |
| **OR end**  | `MAR ← mem[PC++]` ; `AC ← AC | mem[MAR]` — OR bit a bit. Atualiza flags. |
| **AND end** | `MAR ← mem[PC++]` ; `AC ← AC & mem[MAR]` — AND bit a bit. Atualiza flags. |
| **NOT**     | `AC ← ~AC` — complemento de 1. Atualiza flags. |
| **JMP end** | `MAR ← mem[PC]` ; `PC ← MAR` — desvio incondicional. |
| **JN end**  | `MAR ← mem[PC++]` ; se `flag_N=1`, então `PC ← MAR` — desvio se negativo. |
| **JZ end**  | `MAR ← mem[PC++]` ; se `flag_Z=1`, então `PC ← MAR` — desvio se zero. |
| **HLT**     | Marca `halted=1` e encerra a execução, imprimindo o total de ciclos. |

Instruções com opcode desconhecido também marcam `halted=1` e reportam erro.

### 5.3 Manipulação de Flags

As flags são atualizadas pela função `update_flags()` após as instruções LDA, ADD, OR, AND e NOT:

```c
static void update_flags(cpu_state_t *cpu) {
    cpu->flag_N = (cpu->AC & 0x80) ? 1 : 0;  // bit 7 = sinal
    cpu->flag_Z = (cpu->AC == 0)   ? 1 : 0;  // resultado nulo
}
```

**Flag N (Negativo):** ativada quando o bit 7 do acumulador é 1, indicando que o valor, interpretado em complemento de 2, é negativo.

**Flag Z (Zero):** ativada quando o acumulador é exatamente 0x00.

As flags são usadas pelas instruções de desvio condicional JN (salta se N=1) e JZ (salta se Z=1), permitindo implementar estruturas de controle de fluxo.

### 5.4 Proteção contra Loop Infinito

O método `do_run()` impõe um limite de `MEM_SIZE² × 32 = 2.097.152` ciclos. Se a CPU não encontrar um HLT dentro desse limite, a execução é interrompida com aviso, evitando loops infinitos.

---

## 6. Exemplos de Entrada

### `tests/soma_simples.ndr`
```
calcular (3 + 5) * 2 -> resultado
```
Resultado esperado: **16**

### `tests/expressao_complexa.ndr`
```
calcular 10 + 4 * 3 - 2 -> resultado
```
Resultado esperado: **20** (respeitando precedência de operadores: `10 + (4*3) - 2`)

---

## 7. Resumo das Responsabilidades dos Módulos

| Módulo | Arquivo | Responsabilidade |
|---|---|---|
| Frontend | `frontend.c` | Lê o `.ndr`, extrai a expressão e o nome da variável de destino |
| Tokenizer | `tokenizer.c` | Análise léxica: divide a expressão em tokens |
| Parser | `parser.c` | Análise sintática: constrói a AST respeitando precedência |
| Tree | `tree.c` | Operações na AST (impressão, avaliação, liberação) |
| Codegen | `codegen.c` | Percorre a AST e emite assembly Neander |
| **Assembler** | `assembler.c` | Duas passagens: tabela de símbolos + código de máquina |
| **Executor** | `executor.c` | Simula o ciclo de máquina da CPU Neander, manipula flags |
