# Makefile para compilar os arquivos fornecidos

CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS =

# Lista de arquivos fonte
SRCS = ppos_core.c queue.c teste1.c

# Lista de arquivos objeto
OBJS = $(SRCS:.c=.o)

# Nome do executável
EXEC = teste

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(OBJS) $(EXEC)

debug: CFLAGS += -DDEBUG
debug: $(EXEC)