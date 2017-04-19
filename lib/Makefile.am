lib_LTLIBRARIES = libentropy.la
libentropy_la_SOURCES = libentropy.c libentropy.pc.in
libentropy_la_LIBADD = @LIBS@
pkgconfig_DATA = libentropy.pc

if ENABLE_E2NTROPY
lib_LTLIBRARIES += libe2ntropy.la
libe2ntropy_la_SOURCES = libe2ntropy.c libe2ntropy.pc.in
libe2ntropy_CFLAGS = @EXT2FS_CFLAGS@
libe2ntropy_la_LIBADD = @EXT2FS_LIBS@ @LIBS@
pkgconfig_DATA += libe2ntropy.pc
endif
