#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([physengine], [m4_esyscmd([git rev-parse HEAD | cut -c 1-10 | xargs printf "git-%s"])], [atomnuker@gmail.com])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_SRCDIR([src/msg_phys.h])
AM_INIT_AUTOMAKE
# Checks for programs.
AC_PROG_CC([cc])
AM_SILENT_RULES([yes])

CFLAGS+=" -Wall -pedantic"
CFLAGS+=" -std=gnu11 -march=native -g -O3"

# Checks for libraries.
AC_CHECK_LIB([m], [main], [LDFLAGS+=" -lm"],[
         echo "The standard C math libary is required to run this program. You have one, right?"
         exit -1])
AC_CHECK_LIB([pthread], [main], [LDFLAGS+=" -pthread" CFLAGS+=" -pthread"],[
         echo "The standard C math libary is required to run this program. You have one, right?"
         exit -1])
AC_CHECK_LIB([SDL2], [main], [LDFLAGS+=" `sdl2-config --libs`" CFLAGS+=" `sdl2-config --cflags`"],[
         echo "SDL2 library is required to build this program."
         exit -1])
AC_CHECK_LIB([GLESv2], [main], [LDFLAGS+=" -lGLESv2"],[
         echo "OpenGL ES 2.0 library is required to build this program."
         exit -1])
AC_CHECK_LIB([lua5.2], [main], [LDFLAGS+=" -llua5.2"],[
         echo "The Lua 5.2 library is required to compile this program."
         exit -1])
AC_CHECK_LIB([freetype], [main], [LDFLAGS+=" `freetype-config --libs`" CFLAGS+=" `freetype-config --cflags`"],[
         echo "Freetype 2 libary is required to build this program."
         exit -1])
AC_CHECK_LIB([fontconfig], [main], [LDFLAGS+=" -lfontconfig"],[
         echo "Fontconfig dev libary is required to build this program."
         exit -1])

AC_ARG_ENABLE([lto], AS_HELP_STRING([--enable-lto],
		[enable link time optimizations @<:@default=no@:>@]), [LDFLAGS+=" -flto" CFLAGS+=" -flto"], [])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

AC_LANG([C])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_LDFLAGS])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([clock_gettime gettimeofday memset pow sqrt strstr])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])

AC_OUTPUT

echo "
 $PACKAGE_NAME $VERSION

    prefix:		${prefix}
    compiler:		${CC}
    cflags:		${CFLAGS}
    ldflags:		${LDFLAGS}
"
