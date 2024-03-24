TARGET = testafila
LIBRARY = libqueue.a

CC = gcc
CFLAGS = -Wall -std=c99 -g

SOURCES = testafila.c
LIB_SOURCES = queue.c

OBJECTS = $(SOURCES:.c=.o)
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)

$(TARGET): $(OBJECTS) $(LIBRARY)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) -L. -lqueue

$(LIBRARY): $(LIB_OBJECTS)
	ar rcs $(LIBRARY) $(LIB_OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJECTS) $(LIB_OBJECTS) $(TARGET) $(LIBRARY)
