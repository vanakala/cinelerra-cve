#
# Automake Makefile template for mpeg2enc...
# Modified for distrobution with Cinelerra

noinst_LTLIBRARIES = libmpeg2enc.la

libmpeg2enc_la_CFLAGS =-I../quicktime -I../libmpeg3

libmpeg2enc_la_SOURCES = mpeg2enc.c conform.c putseq.c putpic.c puthdr.c putmpg.c \
	 putvlc.c putbits.c predict.c readpic.c writepic.c transfrm.c \
	fdctref.c idct.c quantize.c ratectl.c stats.c \
	global.h mpeg2enc.h vlc.h \
	simd.h synchrolib.h motion.c cpu_accel.c

if USEMMX
libmpeg2enc_la_SOURCES +=     fdct_mmx.s fdctdata.c idct_mmx.s idctdata.c \
    quant_mmx.s quantize_x86.c predict_mmx.s \
    predcomp_mmxe.s predcomp_mmx.s
endif

libmpeg2enc_la_LIBADD=@PTHREAD_LIBS@ @LIBGETOPT_LIB@ $(LIBM_LIBS)
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTC_FLAGS)
