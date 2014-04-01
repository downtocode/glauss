SRCDIR=src
SOURCES=$(shell find $(SRCDIR) -type f -iname '*.c' | sed 's/^\.\.\/src\///')
OBJS=$(subst .c,.o,$(SOURCES))

CC=cc
PROGRAM=physengine
LDFLAGS=-lm -pthread -lGLESv2 `freetype-config --libs` `sdl2-config --libs`
CFLAGS=-Wall -pthread -pedantic -std=gnu99 -march=native `freetype-config --cflags` `sdl2-config --cflags` -O3 -g

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
	rm *.log
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
	rm *.log
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
