AM_LDFLAGS = -Wl,--as-needed
bin_PROGRAMS = entropy
entropy_SOURCES = entropy.c
entropy_LDADD = ../lib/libentropy.la @LIBS@

if ENABLE_E2NTROPY
bin_PROGRAMS += e2ntropy
e2ntropy_SOURCES = e2ntropy.c
e2ntropy_CFLAGS = @EXT2FS_CFLAGS@
e2ntropy_LDADD = ../lib/libentropy.la @EXT2FS_LIBS@
endif
