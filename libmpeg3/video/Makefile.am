noinst_LTLIBRARIES = libmpeg3_video.la
libmpeg3_video_la_SOURCES = getpicture.c headers.c idct.c macroblocks.c mmxtest.c motion.c \
	mpeg3cache.c \
	mpeg3video.c \
	output.c \
	reconstruct.c \
	seek.c \
	slice.c \
	subtitle.c \
	vlc.c

if USEMMX
libmpeg3_video_la_SOURCES += mmxidct.S reconmmx.s
else
libmpeg3_video_la_SOURCES +=
endif

noinst_HEADERS = idct.h \
	layerdata.h \
	mpeg3video.h \
	mpeg3videoprotos.h \
	slice.h \
	vlc.h

libmpeg3_video_la_LDFLAGS=
AM_CFLAGS = $(CPU_CFLAGS) $(LARGEFILE_CFLAGS)
INCLUDES=-I$(srcdir)/..

LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTC_FLAGS)
