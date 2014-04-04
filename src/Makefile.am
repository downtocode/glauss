AM_CFLAGS=-Wall -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" -pthread -pedantic -std=gnu99\
		-march=native `freetype-config --cflags` `sdl2-config --cflags` -O3 -g

AM_LDFLAGS=-lm -pthread -lGLESv2 `freetype-config --libs` `sdl2-config --libs`

bin_PROGRAMS = physengine
physengine_SOURCES = elements.c glfun.c molreader.h msg_phys.h parser.c physengine.c physics.h toxyz.h\
		elements.h glfun.h molreader.c msg_phys.c options.h parser.h physics.c toxyz.c

