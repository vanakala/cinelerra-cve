noinst_LTLIBRARIES = libtoolame.la

libtoolame_la_SOURCES = common.c \
	encode.c \
	ieeefloat.c \
	toolame.c \
	portableio.c \
	psycho_n1.c \
	psycho_0.c \
	psycho_1.c \
	psycho_2.c \
	psycho_3.c \
	psycho_4.c \
	fft.c \
	subband.c \
	audio_read.c \
	bitstream.c \
	mem.c \
	crc.c \
	tables.c \
	availbits.c \
	ath.c \
	encode_new.c

noinst_HEADERS = absthr.h \
	ath.h \
	audio_read.h \
	availbits.h \
	bitstream.h \
	common.h \
	crc.h \
	critband.h \
	encode.h \
	encode_new.h \
	encoder.h \
	enwindow.h \
	fft.h \
	freqtable.h \
	ieeefloat.h \
	mem.h \
	musicin.h \
	options.h \
	portableio.h \
	psycho_0.h \
	psycho_1.h \
	psycho_1_priv.h \
	psycho_2.h \
	psycho_3.h \
	psycho_3priv.h \
	psycho_4.h \
	psycho_n1.h \
	subband.h \
	tables.h \
	toolame.h

EXTRA_DIST = FUTURE HISTORY LGPL.txt pds_subband.c

AM_CFLAGS = $(LARGEFILE_CFLAGS) \
	-fomit-frame-pointer \
	-DNDEBUG -DINLINE=inline -DNEWENCODE -DNEWATAN

libtoolame_la_LDFLAGS =  -lm

LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTC_FLAGS)
