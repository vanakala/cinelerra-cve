OBJDIR := $(shell uname --machine)
USE_MMX = 0
CC = gcc




ifeq ($(origin CFLAGS), environment)
HAVE_CFLAGS := y
else
HAVE_CFLAGS := n
endif

#ENCORE_DIR := encore2/src
#DECORE_DIR := decore2/src
ENCORE_DIR := encore50
#DECORE_DIR := decore50
ENCORE_INCLUDE := $(ENCORE_DIR)/encore.h
#DECORE_INCLUDE := $(DECORE_DIR)/decore.h
MPEG3_DIR := $(shell expr ../libmpeg3* )
OBJS := 





ifneq ($(HAVE_CFLAGS), y)


ifeq ($(OBJDIR), alpha)
  CFLAGS := -O4 -arch ev67 -ieee -accept c99_keywords -gcc_messages
endif

ifeq ($(OBJDIR), i686)
  CFLAGS := -O3 -fmessage-length=0 -funroll-all-loops -fomit-frame-pointer -falign-loops=2 -falign-jumps=2 -falign-functions=2 -I/usr/local/include
endif


ifeq ($(OBJDIR), x86_64)
  CFLAGS := -O2 -funroll-all-loops -fomit-frame-pointer  -I/usr/local/include
endif




endif

DIRS = $(OBJDIR)




include Makefile.jpeg
include Makefile.vorbis
include Makefile.ffmpeg
include Makefile.lame
include Makefile.dv
include Makefile.encore50






#CFLAGS += -g
CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS += -DHAVE_FIREWIRE
CFLAGS += -I. \
	-I$(JPEG_DIR) \
	-I$(MPEG3_DIR) \
	-I$(LAME_DIR)/include


#	$(OBJDIR)/div3.o \
#	$(OBJDIR)/divx.o \






OBJS += \
	$(OBJDIR)/atom.o \
	$(OBJDIR)/avi_hdrl.o \
	$(OBJDIR)/avi_ix.o \
	$(OBJDIR)/avi_idx1.o \
	$(OBJDIR)/avi_indx.o \
	$(OBJDIR)/avi_movi.o \
	$(OBJDIR)/avi_odml.o \
	$(OBJDIR)/avi_riff.o \
	$(OBJDIR)/avi_strl.o \
	$(OBJDIR)/cmodel_default.o \
	$(OBJDIR)/cmodel_yuv420p.o \
	$(OBJDIR)/cmodel_yuv422.o \
	$(OBJDIR)/codecs.o \
	$(OBJDIR)/colormodels.o \
	$(OBJDIR)/ctab.o \
	$(OBJDIR)/dinf.o \
	$(OBJDIR)/dref.o \
	$(OBJDIR)/dv.o \
	$(OBJDIR)/edts.o \
	$(OBJDIR)/elst.o \
	$(OBJDIR)/graphics.o \
	$(OBJDIR)/hdlr.o \
	$(OBJDIR)/ima4.o \
	$(OBJDIR)/jpeg.o \
	$(OBJDIR)/jpeg_old.o \
	$(OBJDIR)/libdv.o \
	$(OBJDIR)/libmjpeg.o \
	$(OBJDIR)/matrix.o \
	$(OBJDIR)/mdat.o \
	$(OBJDIR)/mdhd.o \
	$(OBJDIR)/mdia.o \
	$(OBJDIR)/minf.o \
	$(OBJDIR)/moov.o \
	$(OBJDIR)/mpeg4.o \
	$(OBJDIR)/mvhd.o \
	$(OBJDIR)/plugin.o \
	$(OBJDIR)/qtpng.o \
	$(OBJDIR)/qtmp3.o \
	$(OBJDIR)/quicktime.o \
	$(OBJDIR)/raw.o \
	$(OBJDIR)/rawaudio.o \
	$(OBJDIR)/smhd.o \
	$(OBJDIR)/stbl.o \
	$(OBJDIR)/stco.o \
	$(OBJDIR)/stsc.o \
	$(OBJDIR)/stsd.o \
	$(OBJDIR)/stsdtable.o \
	$(OBJDIR)/stss.o \
	$(OBJDIR)/stsz.o \
	$(OBJDIR)/stts.o \
	$(OBJDIR)/tkhd.o \
	$(OBJDIR)/trak.o \
	$(OBJDIR)/twos.o \
	$(OBJDIR)/udta.o \
	$(OBJDIR)/ulaw.o \
	$(OBJDIR)/util.o \
	$(OBJDIR)/v308.o \
	$(OBJDIR)/v408.o \
	$(OBJDIR)/v410.o \
	$(OBJDIR)/vmhd.o \
	$(OBJDIR)/vorbis.o \
	$(OBJDIR)/wma.o \
	$(OBJDIR)/wmx2.o \
	$(OBJDIR)/workarounds.o \
	$(OBJDIR)/yuv2.o \
	$(OBJDIR)/yuv4.o \
	$(OBJDIR)/yv12.o

UTILS := \
	$(OBJDIR)/qtdump \
	$(OBJDIR)/make_streamable \
	$(OBJDIR)/qtinfo \
	$(OBJDIR)/dechunk \
	$(OBJDIR)/yuv4toyuv \
	$(OBJDIR)/recover \
	$(OBJDIR)/rechunk





OUTPUT = $(OBJDIR)/libquicktime.a
SHAREDOUTPUT = $(OBJDIR)/libquicktime.so

CFLAGS += \
	-I$(ENCORE_DIR) \
	-DENCORE_INCLUDE=\"$(ENCORE_INCLUDE)\" \
	-DLINUX

#	-DDECORE_INCLUDE=\"$(DECORE_INCLUDE)\" \
#	-I$(DECORE_DIR) \
#	-D_DECORE


CFLAGS += \
	-ffast-math \
	-mno-ieee-fp \
	-DHAVE_ALLOCA_H=1 \
	-DHAVE_ALLOCA=1 \
	-DHAVE_SQRTF=1 \
	-DHAVE_LOGF=1 \
	-DHAVE_EXPF=1 \
	-DHAVE_ACOSF=1 \
	-DHAVE_ATANF=1 \
	-DHAVE_FREXPF=1 \
	-DHAVE_RINTF=1 \
	-DUSE_MEMORY_H \
	-D_REENTRANT \
	-DHAVE_ALLOCA=1 \
	-DHAVE_ALLOCA_H=1 \
	-DVORBIS_IEEE_FLOAT32 \
	-DHAVE_DLFCN_H=1 \
	-DSTDC_HEADERS=1 \
	-DSIZEOF_SHORT=2 \
	-DSIZEOF_INT=4 \
	-DSIZEOF_LONG=4 \
	-DSIZEOF_LONG_LONG=8 \
	-DUSE_MEMORY_H

CFLAGS += \
	-I$(FFMPEG_DIR)/libavcodec/ \
	-I$(FFMPEG_DIR)/libav



#DIRS += \
#	$(OBJDIR)/$(DECORE_DIR)/intel_mmx/postprocess

DIRS += \
	$(OBJDIR)/$(ENCORE_DIR) \
	$(OBJDIR)/$(ENCORE_DIR)/att_mmx \
	$(OBJDIR)/$(ENCORE_DIR)/colorspace \
	$(OBJDIR)/$(ENCORE_DIR)/colorspace/intel_mmx \
	$(OBJDIR)/$(ENCORE_DIR)/intel_mmx \
	$(OBJDIR)/$(ENCORE_DIR)/mbcoding \
	$(OBJDIR)/$(ENCORE_DIR)/mbmotionestcomp \
	$(OBJDIR)/$(ENCORE_DIR)/mbmotionestcomp/intel_mmx \
	$(OBJDIR)/$(ENCORE_DIR)/mbprediction \
	$(OBJDIR)/$(ENCORE_DIR)/mbtransquant \
	$(OBJDIR)/$(ENCORE_DIR)/mbtransquant/intel_mmx


LIBS := $(OUTPUT) $(MPEG3_DIR)/$(OBJDIR)/libmpeg3.a -lpng -lz -lpthread -ldl -lm


$(shell if ! test -d $(OBJDIR) \; then mkdir -p $(DIRS) \; fi )
$(shell echo  $(CFLAGS) > $(OBJDIR)/c_flags)
$(shell echo  $(FFMPEG_CFLAGS) > $(OBJDIR)/ffmpeg_flags)
$(shell echo  $(LAME_CFLAGS) > $(OBJDIR)/lame_flags)
$(shell echo  $(OBJS) $(DV_OBJS) $(DVMMXOBJS) $(ENCOREMMX_OBJS) $(JPEG_NASM) $(JPEG_FAIL) $(FFMPEG_OBJS) $(FFMPEGIA32_OBJS) $(LAME_OBJS) > $(OBJDIR)/objs)
#$(shell echo  $(OBJS) $(DV_OBJS) $(DVMMXOBJS) $(DECOREMMX_OBJS) $(ENCOREMMX_OBJS) $(JPEG_NASM) $(JPEG_FAIL) $(FFMPEG_OBJS) $(FFMPEGIA32_OBJS) $(LAME_OBJS) > $(OBJDIR)/objs)

all: $(OBJDIR) $(OUTPUT)

#$(OUTPUT): $(DV_OBJS) $(DVMMXOBJ) $(OBJS) $(DVMMXOBJS) $(DECOREMMX_OBJS) $(ENCOREMMX_OBJS) $(JPEG_NASM) $(FFMPEG_OBJS) $(FFMPEGIA32_OBJS) $(LAME_OBJS) $(JPEG_FAIL) 
$(OUTPUT): $(DV_OBJS) $(DVMMXOBJ) $(OBJS) $(DVMMXOBJS) $(ENCOREMMX_OBJS) $(JPEG_NASM) $(FFMPEG_OBJS) $(FFMPEGIA32_OBJS) $(LAME_OBJS) $(JPEG_FAIL) 
	ar rcs $(OUTPUT) `cat $(OBJDIR)/objs`


$(OBJDIR)/qtdump: $(OBJDIR)/dump.o $(OUTPUT)
	$(CC) -o $(OBJDIR)/qtdump $(OBJDIR)/dump.o $(LIBS)

$(OBJDIR)/make_streamable: $(OBJDIR)/make_streamable.o $(OUTPUT)
	$(CC) -o $(OBJDIR)/make_streamable $(OBJDIR)/make_streamable.o $(LIBS)

$(OBJDIR)/dechunk: $(OBJDIR)/dechunk.o $(OUTPUT)
	$(CC) -o $(OBJDIR)/dechunk $(OBJDIR)/dechunk.o $(LIBS)

$(OBJDIR)/rechunk: $(OBJDIR)/rechunk.o $(OUTPUT)
	$(CC) -o $(OBJDIR)/rechunk $(OBJDIR)/rechunk.o $(LIBS)

$(OBJDIR)/recover: $(OBJDIR)/recover.o $(OUTPUT)
	$(CC) -o $(OBJDIR)/recover $(OBJDIR)/recover.o $(LIBS)

$(OBJDIR)/yuv4toyuv: $(OBJDIR)/yuv4toyuv.o $(OUTPUT)
	$(CC) -o $(OBJDIR)/yuv4toyuv $(OBJDIR)/yuv4toyuv.o $(LIBS)

$(OBJDIR)/qtinfo: $(OBJDIR)/qtinfo.o
	$(CC) -o $(OBJDIR)/qtinfo $(OBJDIR)/qtinfo.o $(LIBS)

util:  $(UTILS)
	

clean:
	rm -rf $(OBJDIR)
	rm -f $(DVMMXOBJ)

wc:
	cat *.c *.h | wc

backup: clean
	cd .. && \
	tar zcvf quicktime.tar.gz quicktime

install:
	cp $(UTILS) /usr/bin

$(OBJDIR)/dump.o \
$(OBJDIR)/make_streamable.o \
$(OBJDIR)/dechunk.o \
$(OBJDIR)/recover.o \
$(OBJDIR)/yuv4toyuv.o \
$(OBJDIR)/qtinfo.o \
$(OBJDIR)/rechunk.o \
$(OBJS):
	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.c) -o $*.o

$(JPEG_FAIL):
	$(CC) -c -O3 -march=i686 -fmessage-length=0 $(subst $(OBJDIR)/,, $*.c) -o $*.o

$(DV_OBJS):
	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.c) -o $*.o

$(DVMMXOBJS):
	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.S) -o $*.o

$(DVMMXOBJ): $(DV_DIR)/libdv/gasmoff.c
	$(CC) `cat $(OBJDIR)/c_flags` $(DV_DIR)/libdv/gasmoff.c -o $(OBJDIR)/$(DV_DIR)/libdv/gasmoff -lm
	$(OBJDIR)/$(DV_DIR)/libdv/gasmoff > $(DVMMXOBJ)

$(JPEG_NASM):
	nasm -f elf $(subst $(OBJDIR)/,, $*.s) -o $*.o

$(ENCOREMMX_OBJS):
	nasm -f elf $(subst $(OBJDIR)/,, $*.asm) -o $*.o

#$(DECOREMMX_OBJS):
#	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.S) -o $*.o


$(FFMPEG_OBJS) $(FFMPEGIA32_OBJS):
	$(CC) `cat $(OBJDIR)/ffmpeg_flags` -c $(subst $(OBJDIR)/,, $*.c) -o $*.o

$(LAME_OBJS):
	$(CC) `cat $(OBJDIR)/lame_flags` -c $(subst $(OBJDIR)/,, $*.c) -o $*.o

#$(FFMPEGIA32_OBJS):
#	nasm -f elf $(subst $(OBJDIR)/,, $*.s) -o $*.o




#$(OBJDIR)/div3.o:                         div3.c
#$(OBJDIR)/divx.o:			  divx.c


$(OBJDIR)/atom.o:			  atom.c
$(OBJDIR)/avi_hdrl.o:                     avi_hdrl.c
$(OBJDIR)/avi_ix.o:                       avi_ix.c
$(OBJDIR)/avi_idx1.o:                     avi_idx1.c
$(OBJDIR)/avi_indx.o:                     avi_indx.c
$(OBJDIR)/avi_movi.o:                     avi_movi.c
$(OBJDIR)/avi_odml.o:                     avi_odml.c
$(OBJDIR)/avi_riff.o:                     avi_riff.c
$(OBJDIR)/avi_strl.o:                     avi_strl.c
$(OBJDIR)/cmodel_default.o:		  cmodel_default.c cmodel_permutation.h
$(OBJDIR)/cmodel_yuv420p.o:		  cmodel_yuv420p.c cmodel_permutation.h
$(OBJDIR)/cmodel_yuv422.o:		  cmodel_yuv422.c cmodel_permutation.h
$(OBJDIR)/codecs.o:			  codecs.c
$(OBJDIR)/colormodels.o:		  colormodels.c
$(OBJDIR)/ctab.o:			  ctab.c
$(OBJDIR)/dechunk.o:			  dechunk.c
$(OBJDIR)/dinf.o:			  dinf.c
$(OBJDIR)/dref.o:			  dref.c
$(OBJDIR)/dump.o:			  dump.c
$(OBJDIR)/dv.o:			  	  dv.c
$(OBJDIR)/edts.o:			  edts.c
$(OBJDIR)/elst.o:			  elst.c
$(OBJDIR)/graphics.o:			  graphics.c
$(OBJDIR)/hdlr.o:			  hdlr.c
$(OBJDIR)/ima4.o:			  ima4.c
$(OBJDIR)/jpeg.o:			  jpeg.c
$(OBJDIR)/jpeg_old.o:			  jpeg_old.c
$(OBJDIR)/libdv.o:			  libdv.c
$(OBJDIR)/libmjpeg.o:			  libmjpeg.c
$(OBJDIR)/make_streamable.o:		  make_streamable.c
$(OBJDIR)/matrix.o:			  matrix.c
$(OBJDIR)/mdat.o:			  mdat.c
$(OBJDIR)/mdhd.o:			  mdhd.c
$(OBJDIR)/mdia.o:			  mdia.c
$(OBJDIR)/minf.o:			  minf.c
$(OBJDIR)/moov.o:			  moov.c
$(OBJDIR)/mpeg4.o:			  mpeg4.c
$(OBJDIR)/mvhd.o:			  mvhd.c
$(OBJDIR)/plugin.o:			  plugin.c
$(OBJDIR)/qtinfo.o:			  qtinfo.c
$(OBJDIR)/qtmp3.o:                        qtmp3.c
$(OBJDIR)/qtpng.o:			  qtpng.c
$(OBJDIR)/quicktime.o: 		  	  quicktime.c
$(OBJDIR)/raw.o:			  raw.c
$(OBJDIR)/rawaudio.o:			  rawaudio.c
$(OBJDIR)/rechunk.o:			  rechunk.c
$(OBJDIR)/recover.o:			  recover.c
$(OBJDIR)/smhd.o:			  smhd.c
$(OBJDIR)/stbl.o:			  stbl.c
$(OBJDIR)/stco.o:			  stco.c
$(OBJDIR)/strl.o:			  strl.c
$(OBJDIR)/stsc.o:			  stsc.c
$(OBJDIR)/stsd.o:			  stsd.c
$(OBJDIR)/stsdtable.o: 		  	  stsdtable.c
$(OBJDIR)/stss.o:			  stss.c
$(OBJDIR)/stsz.o:			  stsz.c
$(OBJDIR)/stts.o:			  stts.c
$(OBJDIR)/tkhd.o:			  tkhd.c
$(OBJDIR)/trak.o:			  trak.c
$(OBJDIR)/twos.o:			  twos.c
$(OBJDIR)/udta.o:			  udta.c
$(OBJDIR)/ulaw.o:			  ulaw.c
$(OBJDIR)/util.o:			  util.c
$(OBJDIR)/v308.o:                         v308.c
$(OBJDIR)/v408.o:                         v408.c
$(OBJDIR)/v410.o:                         v410.c
$(OBJDIR)/vmhd.o:			  vmhd.c
$(OBJDIR)/vorbis.o:                       vorbis.c
$(OBJDIR)/wma.o:		          wma.c
$(OBJDIR)/wmx2.o:		          wmx2.c
$(OBJDIR)/workarounds.o:		  workarounds.c
$(OBJDIR)/yuv2.o:			  yuv2.c
$(OBJDIR)/yuv4.o:			  yuv4.c
$(OBJDIR)/yuv4toyuv.o:			  yuv4toyuv.c
$(OBJDIR)/yv12.o:			  yv12.c









































include depend.vorbis
include depend.ffmpeg
include depend.jpeg
include depend.lame
include depend.dv
include depend.encore50

