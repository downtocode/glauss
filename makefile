
SRCDIR=src
SOURCES=$(shell find $(SRCDIR) -type f -iname '*.c' | sed 's/^\.\.\/src\///')
OBJS=$(subst .c,.o,$(SOURCES))

CC=cc
PROGRAM=physengine
LDFLAGS=-lm -pthread -lEGL -lGLESv2 -lrt -lX11 `freetype-config --libs` `sdl2-config --libs`
CFLAGS=-Wall -pedantic -std=c99 `freetype-config --cflags` `sdl2-config --cflags` -O2 -msse -msse2 -msse3 -g


vpath %.c ./src/

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $< -c -o $@ $(CFLAGS)

.PHONY: all clean

clean:
	rm -rf $(OBJS) $(PROGRAM)
	rm *.xyz

.PHONY: all rem

rem:
	rm *.xyz
