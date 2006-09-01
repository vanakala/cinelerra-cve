lib_LTLIBRARIES = libmpeg3hv.la
AM_CFLAGS = $(CPU_CFLAGS) $(LARGEFILE_CFLAGS) $(CSS_CFLAGS)
libmpeg3hv_la_LDFLAGS = -version-info 1:0:0 -release 1.5.0 -Wl,--no-undefined
libmpeg3hv_la_LIBADD = -la52 video/libmpeg3_video.la audio/libmpeg3_audio.la -lm \
		     -lpthread
libmpeg3hv_la_SOURCES = bitstream.c libmpeg3.c mpeg3atrack.c mpeg3css.c \
	mpeg3demux.c \
	mpeg3ifo.c \
	mpeg3io.c \
	mpeg3strack.c \
	mpeg3title.c \
	mpeg3tocutil.c \
											mpeg3vtrack.c workarounds.c 
bin_PROGRAMS = mpeg3dump mpeg3toc mpeg3cat
mpeg3dump_SOURCES = mpeg3dump.c
mpeg3dump_LDADD = libmpeg3hv.la
mpeg3toc_SOURCES = mpeg3toc.c
mpeg3toc_LDADD = libmpeg3hv.la
mpeg3cat_SOURCES = mpeg3cat.c
mpeg3cat_LDADD = libmpeg3hv.la

SUBDIRS = audio video 

noinst_HEADERS = bitstream.h \
	ifo.h \
	libmpeg3.h \
	mpeg3atrack.h \
	mpeg3css.h \
	mpeg3css_fake.c \
	mpeg3demux.h \
	mpeg3io.h \
	mpeg3private.h \
	mpeg3private.inc \
	mpeg3protos.h \
	mpeg3title.h \
	mpeg3vtrack.h \
	timecode.h \
	workarounds.h
	

pkgincludedir=$(includedir)/mpeg3
pkginclude_HEADERS=libmpeg3.h mpeg3private.h
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTC_FLAGS)
