
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
	@${LINK_STATUS}
	@$(CC) $^ -o $@ $(LDFLAGS)
	@${LINK_OK}

%.o: %.c
	@${COMPILE_STATUS}
	@$(CC) $< -c -o $@ $(CFLAGS)
	@${COMPILE_OK}

.PHONY: all clean

clean:
	@${DELETE_PROG}
	rm -rf $(OBJS) $(PROGRAM)
ifneq (,$(wildcard *.xyz))
	rm *.xyz
	@${DELETE_XYZ}
else 
	@${DELETE_NO}
endif

.PHONY: all rem

rem:
ifneq (,$(wildcard *.xyz))
	rm *.xyz
	@${DELETE_XYZ}
else 
	@${DELETE_NO}
endif

COMPILE_STATUS = printf "[K[33mCompiling [1m$<(B[m[33m...(B[m\r"
COMPILE_OK = printf "[K[32mSuccessfully compiled [1m$<(B[m[32m.(B[m\n"
LINK_STATUS = printf "[K[33mLinking [1m$@(B[m[33m...(B[m\r"
LINK_OK = printf "[K[32mSuccessfully linked [1m$@(B[m[32m.(B[m\n"
DELETE_XYZ = printf "[K[31mDeleted *.xyz files.(B[m\n"
DELETE_PROG = printf "[K[33mCleaning up...(B[m\n"
DELETE_NO = printf "[K[33mNo .xyz files to delete.(B[m\n"
