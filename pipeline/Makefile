CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c99 -g
TARGET  = ndr
SRCDIR  = src
INCDIR  = include

SRCS = $(SRCDIR)/main.c     \
       $(SRCDIR)/common.c   \
       $(SRCDIR)/frontend.c \
       $(SRCDIR)/tokenizer.c\
       $(SRCDIR)/parser.c   \
       $(SRCDIR)/tree.c     \
       $(SRCDIR)/codegen.c  \
       $(SRCDIR)/assembler.c\
       $(SRCDIR)/executor.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean test test_verbose

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o $(TARGET) tests/*.asm tests/*.mem

# Testes básicos
test: all
	@echo ""
	@echo "╔══════════════════════════════════════╗"
	@echo "║  Teste 1: soma simples               ║"
	@echo "╚══════════════════════════════════════╝"
	./$(TARGET) tests/soma_simples.ndr
	@echo ""
	@echo "╔══════════════════════════════════════╗"
	@echo "║  Teste 2: expressão complexa         ║"
	@echo "╚══════════════════════════════════════╝"
	./$(TARGET) tests/expressao_complexa.ndr

# Testes com saída detalhada
test_verbose: all
	@echo ""
	@echo "╔══════════════════════════════════════╗"
	@echo "║  Teste 1 (verbose)                   ║"
	@echo "╚══════════════════════════════════════╝"
	./$(TARGET) tests/soma_simples.ndr --verbose
	@echo ""
	@echo "╔══════════════════════════════════════╗"
	@echo "║  Teste 2 (verbose)                   ║"
	@echo "╚══════════════════════════════════════╝"
	./$(TARGET) tests/expressao_complexa.ndr --verbose
