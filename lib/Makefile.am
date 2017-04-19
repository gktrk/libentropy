lib_LTLIBRARIES = libentropy.la
libentropy_la_SOURCES = libentropy.c
libentropy_la_LIBADD = @LIBS@

if ENABLE_E2NTROPY
lib_LTLIBRARIES += libe2ntropy.la
libe2ntropy_la_SOURCES = libe2ntropy.c
libe2ntropy_CFLAGS = @EXT2FS_CFLAGS@
libe2ntropy_la_LIBADD = @EXT2FS_LIBS@ @LIBS@
endif
