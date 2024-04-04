CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Lista de arquivos fonte
SRCS = $(wildcard *.c)

# Lista de arquivos objeto
OBJS = $(SRCS:.c=.o)

# Nome do executável
EXEC = teste

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)

debug: CFLAGS += -DDEBUG
debug: $(EXEC)
