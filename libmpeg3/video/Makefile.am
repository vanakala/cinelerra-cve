noinst_LTLIBRARIES = libmpeg3_video.la
libmpeg3_video_la_SOURCES = getpicture.c headers.c idct.c macroblocks.c mmxtest.c motion.c \
														mpeg3video.c output.c reconstruct.c seek.c slice.c vlc.c 
if USEMMX
libmpeg3_video_la_SOURCES += mmxidct.S reconmmx.s
else
libmpeg3_video_la_SOURCES +=
endif

libmpeg3_video_la_LDFLAGS=-static
AM_CFLAGS = $(CPU_CFLAGS) $(LARGEFILE_CFLAGS)
INCLUDES=-I..


