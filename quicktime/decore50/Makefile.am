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
