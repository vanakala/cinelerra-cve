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

libtoolame_la_CFLAGS = -fomit-frame-pointer -O3 -DNDEBUG -DINLINE=inline -DNEWENCODE -DNEWATAN

libtoolame_la_LDFLAGS =  -lm
