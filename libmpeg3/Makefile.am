INCLUDES = -Ivideo -Iaudio 
lib_LTLIBRARIES =  libmpeg3.la
AM_CFLAGS = $(CPU_CFLAGS) $(LARGEFILE_CFLAGS) $(CSS_CFLAGS)
libmpeg3_la_LDFLAGS = -version-info 1:0:0 -release 1.5.0 -Wl,--no-undefined
libmpeg3_la_LIBADD = -la52 video/libmpeg3_video.la audio/libmpeg3_audio.la -lm \
		     -lpthread
libmpeg3_la_SOURCES = bitstream.c libmpeg3.c mpeg3atrack.c mpeg3css.c \
											mpeg3demux.c mpeg3ifo.c mpeg3io.c mpeg3title.c \
											mpeg3vtrack.c workarounds.c 
SUBDIRS = audio video 

pkgincludedir=$(includedir)/mpeg3
pkginclude_HEADERS=libmpeg3.h mpeg3private.h
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTC_FLAGS)
