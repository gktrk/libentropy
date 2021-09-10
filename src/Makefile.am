AM_LDFLAGS = @LDFLAGS_AS_NEEDED@
bin_PROGRAMS = entropy
entropy_SOURCES = entropy.c
entropy_CPPFLAGS = -I$(top_srcdir)/include
entropy_LDADD = $(top_builddir)/lib/libentropy.la @LIBS@

if ENABLE_E2NTROPY
bin_PROGRAMS += e2ntropy
e2ntropy_SOURCES = e2ntropy.c
e2ntropy_LDADD = $(top_builddir)/lib/libentropy.la $(top_builddir)/lib/libe2ntropy.la
endif
