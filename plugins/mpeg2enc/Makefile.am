plugin_LTLIBRARIES = mpeg2enc.la
mpeg2enc_la_LDFLAGS = -avoid-version -module -shared 
mpeg2enc_la_LIBADD = ../../libmpeg3/libmpeg3.la

mpeg2enc_la_SOURCES = conform.c cpu_accel.c fdctdata.c fdctref.c idct.c idctdata.c \
											motion.c predict.c putbits.c \
											puthdr.c putmpg.c putpic.c putseq.c putvlc.c quantize.c \
											ratectl.c readpic.c stats.c transfrm.c writepic.c mpeg2enc.c


if USEMMX
mpeg2enc_la_SOURCES += mblock_sub44_sads.c bdist1_mmx.s bdist2_mmx.s dist2_mmx.s fdct_mmx.s idct_mmx.s \
                       mblockq_sad_mmxe.s mblock_sad_mmxe.s mblock_sad_mmx.s \
                       predcomp_mmxe.s predcomp_mmx.s predict_mmx.s quant_mmx.s quantize_x86.c
endif

AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/libmpeg3
