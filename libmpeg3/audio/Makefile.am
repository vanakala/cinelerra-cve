noinst_LTLIBRARIES = libmpeg3_audio.la
libmpeg3_audio_la_SOURCES = ac3.c dct.c huffman.c layer2.c layer3.c mpeg3audio.c pcm.c synthesizers.c tables.c 
libmpeg3_audio_la_LDFLAGS = -static
AM_CFLAGS = $(CPU_CFLAGS) $(LARGEFILE_CFLAGS)
AM_CXXFLAGS = $(CPU_CFLAGS) $(LARGEFILE_CFLAGS)
INCLUDES=-I.. -I/usr/include/a52dec

LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTC_FLAGS)

