
CC = gcc

c_sources = \
	common.c \
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

OBJ = $(c_sources:.c=.o)

#Uncomment this if you want to do some profiling/debugging
#PG = -g -pg
PG = -fomit-frame-pointer

# Optimize flag. 3 is about as high as you can sanely go with GCC3.2.
OPTIM = -O3

# These flags are pretty much mandatory
REQUIRED = -DNDEBUG -DINLINE=inline

#pick your architecture
ARCH = -march=pentium
#Possible x86 architectures
#gcc3.2 => i386, i486, i586, i686, pentium, pentium-mmx
#          pentiumpro, pentium2, pentium3, pentium4, k6, k6-2, k6-3,
#          athlon, athlon-tbird, athlon-4, athlon-xp and athlon-mp.

#TWEAK the hell out of the compile. Some of these are real dodgy
# and will cause program instability
#TWEAKS = -finline-functions -fexpensive-optimizations -ffast-math \
#	-malign-double \
#	-mfancy-math-387 -funroll-loops -funroll-all-loops -pipe \
#	-fschedule-insns2 -fno-strength-reduce

#Set a stack of warnings to overcome my atrocious coding style . MFC.
WARNINGS = -Wall
WARNINGS2 = -Wstrict-prototypes -Wmissing-prototypes -Wunused -Wunused-function -Wunused-label -Wunused-parameter -Wunused-variable -Wunused-value -Wredundant-decls

NEW_02L_FIXES = -DNEWENCODE -DNEWATAN

CC_SWITCHES = $(OPTIM) $(REQUIRED) $(ARCH) $(PG) $(TWEAKS) $(WARNINGS) $(NEW_02L_FIXES)

PGM = toolame

LIBS =  -lm

#nick burch's OS/2 fix  gagravarr@SoftHome.net
UNAME = $(shell uname)
ifeq ($(UNAME),OS/2)
   SHELL=sh     
   PGM = toolame.exe
   PG = -Zcrtdll -Zexe
   LIBS =
endif

%.o: %.c Makefile
	$(CC) $(CC_SWITCHES) -c $< -o $@

$(PGM):	$(OBJ) Makefile
	$(CC) $(PG) -o $(PGM) $(OBJ) $(LIBS)

clean:
	-rm $(OBJ) $(DEP)

megaclean:
	-rm $(OBJ) $(DEP) $(PGM) \#*\# *~

distclean:
	-rm $(OBJ) $(DEP) $(PGM) \#* *~ gmon.out gprof* core *shit* *.wav *.mp2 *.c.* *.mp2.* *.da *.h.* *.d *.mp3 *.pcm *.wav logfile

tags: TAGS

TAGS: ${c_sources}
	etags -T ${c_sources}

