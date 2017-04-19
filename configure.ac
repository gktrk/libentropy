#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([libentropy], [0.0.3], [gokturk@binghamton.edu])
AC_CONFIG_SRCDIR([src/entropy.c])
AC_CONFIG_HEADERS([include/config.h])
AM_INIT_AUTOMAKE([foreign])

AC_ARG_ENABLE([e2ntropy],
	[AS_HELP_STRING([--enable-e2ntropy],
		"Enable e2ntropy library and tool for ext4")],
		[], [enable_e2ntropy=no])
AM_CONDITIONAL([ENABLE_E2NTROPY], [test x$enable_e2ntropy != xno])


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
AC_CHECK_FUNCS([posix_fadvise lseek64])
AC_CHECK_FUNCS([getopt_long])

AS_IF([test "x$enable_e2ntropy" != "xno"], [
	PKG_CHECK_MODULES([EXT2FS], [ext2fs >= 1.43.3])
])


AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 include/Makefile
                 src/Makefile])
AC_OUTPUT
