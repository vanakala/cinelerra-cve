noinst_LTLIBRARIES = libdecore.la
AM_LDFLAGS = -static
libdecore_la_CFLAGS = -D_DECORE -DLINUX $(CPU_CFLAGS)
libdecore_la_LIBADD = 
libdecore_la_SOURCES = clearblock.c debug.c decore.c getbits.c idct.c mp4_block.c mp4_decoder.c mp4_header.c \
											 mp4_iquant.c mp4_mblock.c mp4_picture.c mp4_predict.c mp4_recon.c mp4_tables.c mp4_vars.c \
											 mp4_vld.c postprocess.c transferidct.c yuv2rgb.c 
if USEMMX
libdecore_la_SOURCES += basic_prediction_mmx.c
else
libdecore_la_SOURCES += basic_prediction.c
endif
INCLUDES=-I./

noinst_HEADERS = basic_prediction.h \
	clearblock.h \
	debug.h \
	decore.h \
	gen_draw.h \
	gen_usetime.h \
	getbits.h \
	global.h \
	idct.h \
	mp4_block.h \
	mp4_decoder.h \
	mp4_header.h \
	mp4_iquant.h \
	mp4_mblock.h \
	mp4_predict.h \
	mp4_vars.h \
	mp4_vld.h \
	portab.h \
	postprocess.h \
	store.h \
	timer.h \
	transferidct.h \
	yuv2rgb.h

LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTC_FLAGS)
