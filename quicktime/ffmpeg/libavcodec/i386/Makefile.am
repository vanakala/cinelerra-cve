noinst_LTLIBRARIES = libavcodeci386.la

if TARGET_BUILTIN_VECTOR
SSE_FLAGS = -msse -DHAVE_BUILTIN_VECTOR
SSE_FILES = fft_sse.c
else
SSE_FLAGS =
endif

#Cinelerra arch build adjustment
if FFMPEG_ARCH_32BIT
FFMPEG_PIC_PREF = -prefer-non-pic
else
FFMPEG_PIC_PREF =
endif

# special options needed due to scarcity of registers
AM_CFLAGS = \
	$(LARGEFILE_CFLAGS) \
	$(CPU_CFLAGS) \
	-DHAVE_MMX $(SSE_FLAGS) \
	-O3 $(FFMPEG_PIC_PREF) \
	-D_GNU_SOURCE -DHAVE_AV_CONFIG_H -I$(srcdir)/../.. -I../..

libavcodeci386_la_SOURCES = \
	fdct_mmx.c cputest.c \
	dsputil_mmx.c mpegvideo_mmx.c \
	idct_mmx.c motion_est_mmx.c \
	simple_idct_mmx.c vp3dsp_mmx.c \
	vp3dsp_sse2.c \
	$(SSE_FILES)

EXTRA_DIST = cputest.c \
	dsputil_h264_template_mmx.c \
	dsputil_mmx_avg.h \
	dsputil_mmx_rnd.h \
	h264dsp_mmx.c \
	mmx.h \
	mpegvideo_mmx_template.c
