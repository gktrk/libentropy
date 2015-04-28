#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([libentropy], [0.0.1], [gokturk@binghamton.edu])
AC_CONFIG_SRCDIR([src/entropy.c])
AC_CONFIG_HEADERS([include/config.h])
AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CC_C99
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h unistd.h math.h limits.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_SEARCH_LIBS([log2], [m])
AC_SEARCH_LIBS([isnormal], [m])
AC_CHECK_FUNCS([posix_fadvise])
AC_SEARCH_LIBS([ext2fs_open], [ext2fs])

AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 include/Makefile
                 src/Makefile])
AC_OUTPUT
