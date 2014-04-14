noinst_LTLIBRARIES = libmpeg3_audio.la
libmpeg3_audio_la_SOURCES = ac3.c dct.c huffman.c layer2.c layer3.c mpeg3audio.c pcm.c synthesizers.c tables.c 
libmpeg3_audio_la_LDFLAGS =

noinst_HEADERS = ac3.h huffman.h mpeg3audio.h tables.h

AM_CFLAGS = $(CPU_CFLAGS) $(LARGEFILE_CFLAGS)
AM_CXXFLAGS = $(CPU_CFLAGS) $(LARGEFILE_CFLAGS)
AM_CPPFLAGS = -I$(srcdir)/..

LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTC_FLAGS)

