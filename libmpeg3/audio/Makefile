include ../global_config
export CFLAGS

OBJS = \
	ac3.o \
	bit_allocation.o \
	dct.o \
	exponents.o \
	header.o \
	layer2.o \
	layer3.o \
	mantissa.o \
	mpeg3audio.o \
	pcm.o \
	synthesizers.o \
	tables.o

all: $(OBJS)

.c.o:
	$(CC) -c `./c_flags` $*.c

.s.o:
	$(CC) -f elf $*.s

clean:
	rm -f *.o 
