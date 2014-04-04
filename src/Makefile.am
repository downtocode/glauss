AM_CFLAGS=-Wall -pthread -pedantic -std=gnu99 -march=native -I/usr/include/freetype2 -I/usr/include/SDL2 -D_REENTRANT -O2 -g -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\""
AM_LDFLAGS=-lm -pthread -lGLESv2 -lfreetype -lz -lpng12 -lSDL2

bin_PROGRAMS = physengine
physengine_SOURCES = elements.c glfun.c molreader.h msg_phys.h parser.c physengine.c physics.h toxyz.h\
			elements.h glfun.h molreader.c msg_phys.c options.h parser.h physics.c toxyz.c

