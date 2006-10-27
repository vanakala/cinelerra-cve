noinst_LTLIBRARIES = libavcodecppc.la

# special options needed due to scarcity of registers
AM_CFLAGS = \
	$(LARGEFILE_CFLAGS) \
	$(CPU_CFLAGS) \
	-O3 -prefer-non-pic \
	-D_GNU_SOURCE -DHAVE_AV_CONFIG_H -I$(srcdir)/../..

libavcodecppc_la_SOURCES = \
	dsputil_ppc.c mpegvideo_ppc.c

EXTRA_DIST = dsputil_ppc.c \
	mpegvideo_ppc.c

