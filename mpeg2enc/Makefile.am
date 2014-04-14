#
# Automake Makefile template for mpeg2enc...
# Modified for distrobution with Cinelerra

noinst_LTLIBRARIES = libmpeg2enc.la

AM_CPPFLAGS = -I$(top_srcdir)/quicktime -I$(top_srcdir)/libmpeg3
AM_CFLAGS = $(LARGEFILE_CFLAGS)

libmpeg2enc_la_SOURCES = conform.c mpeg2enc.c putseq.c putpic.c puthdr.c putmpg.c \
	 putvlc.c putbits.c predict.c readpic.c writepic.c transfrm.c \
	fdctref.c idct.c quantize.c ratectl.c stats.c \
	global.h mpeg2enc.h vlc.h \
	simd.h motion.c cpu_accel.c

if USEMMX
libmpeg2enc_la_SOURCES +=     fdct_mmx.s fdctdata.c idct_mmx.s idctdata.c \
    quant_mmx.s quantize_x86.c predict_mmx.s \
    predcomp_mmxe.s predcomp_mmx.s
endif

noinst_HEADERS = attributes.h \
	config.h \
	cpu_accel.h \
	fastintfns.h \
	global.h \
	mmx.h \
	mpeg2enc.h \
	mtable.h \
	putbits.h \
	simd.h \
	vlc.h

libmpeg2enc_la_LIBADD=$(LIBM_LIBS)
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTC_FLAGS)
