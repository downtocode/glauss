SRCDIR=src
SOURCES=$(shell find $(SRCDIR) -type f -iname '*.c' | sed 's/^\.\.\/src\///')
OBJS=$(subst .c,.o,$(SOURCES))

CC=cc
PROGRAM=physengine
LDFLAGS=-lm -pthread -lGLESv2 `freetype-config --libs` `sdl2-config --libs`
CFLAGS=-Wall -pthread -pedantic -std=gnu99 -march=native `freetype-config --cflags` `sdl2-config --cflags` -O2 -g

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
ifneq (,$(wildcard $(PROGRAM)))
	rm $(PROGRAM)
	@${DELETE_PROG}
endif
ifneq (,$(wildcard $(OBJS)))
	rm -rf $(OBJS)
endif
ifneq (,$(wildcard *.xyz))
	rm *.xyz
	@${DELETE_XYZ}
endif
ifneq (,$(wildcard *.log))
	rm *.log
	@${DELETE_LOG}
endif

.PHONY: all rem

rem:
ifneq (,$(wildcard *.xyz))
	rm *.xyz
	@${DELETE_XYZ}
endif
ifneq (,$(wildcard *.log))
	rm *.log
	@${DELETE_LOG}
endif

COMPILE_STATUS = printf "[K[33mCompiling [1m$<(B[m[33m...(B[m\r"
COMPILE_OK = printf "[K[32mSuccessfully compiled [1m$<(B[m[32m.(B[m\n"
LINK_STATUS = printf "[K[33mLinking [1m$@(B[m[33m...(B[m\r"
LINK_OK = printf "[K[32mSuccessfully linked [1m$@(B[m[32m.(B[m\n"
DELETE_XYZ = printf "[K[31mDeleted *.xyz files.(B[m\n"
DELETE_LOG = printf "[K[31mDeleted *.log files.(B[m\n"
DELETE_PROG = printf "[K[33mCleaning up...(B[m\n"
