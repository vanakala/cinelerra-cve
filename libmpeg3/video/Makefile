include ../global_config
export CFLAGS

OBJS = \
	getpicture.o \
	headers.o \
	idct.o \
	macroblocks.o \
	mmxtest.o \
	motion.o \
	mpeg3video.o \
	output.o \
	reconstruct.o \
	seek.o \
	slice.o \
	vlc.o


all: $(OBJS) $(MMXOBJS2)

.c.o:
	$(CC) -c `./c_flags` $*.c

.s.o:
	$(NASM) -f elf $*.s

.S.o:
	$(CC) -c `./c_flags` $*.S

clean:
	rm -f *.o 
