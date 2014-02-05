
SRCDIR=src
SOURCES=$(shell find $(SRCDIR) -type f -iname '*.c' | sed 's/^\.\.\/src\///')
OBJS=$(subst .c,.o,$(SOURCES))

CC=cc
PROGRAM=physengine
LDFLAGS=-lm -lGL -lGLU -lSDL2
CFLAGS=-Wall -pedantic -std=c99 -O3 -msse -g


vpath %.c ./src/

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $< -c -o $@ $(CFLAGS)

.PHONY: all clean

clean:
	rm -rf $(OBJS) $(PROGRAM)

.PHONY: all rem

rem:
	rm *.xyz
