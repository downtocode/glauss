#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([physengine], [0.1pre], [atomnuker@gmail.com])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_SRCDIR([src/msg_phys.h])
AM_INIT_AUTOMAKE
# Checks for programs.
AC_PROG_CC([cc gcc clang])

# Checks for libraries.
AC_CHECK_LIB([m], [main], [],[
         echo "The standard C math libary is required to run this program. You have one, right?"
         exit -1])
AC_CHECK_LIB([SDL2], [main], [],[
         echo "SDL2 library is required to build this program."
         exit -1])
AC_CHECK_LIB([GLESv2], [main], [],[
         echo "OpenGL ES 2.0 library is required to build this program."
         exit -1])
AC_CHECK_LIB([freetype], [main], [],[
         echo "Freetype 2 libary is required to build this program."
         exit -1])
AC_CHECK_LIB([png12], [main], [],[
         echo "The PNG 1.2 library is required(by Freetype) to build this program."
         exit -1])
AC_CHECK_LIB([z], [main], [],[
         echo "The standard ZIP libary is required(by Freetype) to build this program."
         exit -1])

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
