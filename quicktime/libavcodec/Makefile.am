AM_CFLAGS= $(OPTFLAGS) -Wall -g -DHAVE_AV_CONFIG_H -I.. -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_GNU_SOURCE
AM_LDFLAGS= -g

ASM_EXTRASOURCES=
EXTRASOURCES=
EXTRALIBS=

# currently using liba52 for ac3 decoding
if FFMPEG_AC3
EXTRASOURCES +=a52dec.c
if FFMPEG_A52BIN
  EXTRALIBS += -la52
else
  EXTRASOURCES += liba52/bit_allocate.c liba52/bitstream.c liba52/downmix.c \
			liba52/imdct.c  liba52/parse.c

endif

endif

# using builtin liba52 or runtime linked liba52.so.0
if FFMPEG_MP3LAME
EXTRASOURCES +=mp3lameaudio.c
EXTRALIBS += -lmp3lame
endif

if FFMPEG_VORBIS
EXTRASOURCES +=oggvorbis.c
EXTRALIBS += -lvorbis -lvorbisenc
endif

# i386 mmx specific stuff
if USEMMX
AM_CFLAGS += -DHAVE_MMX
EXTRASOURCES +=i386/fdct_mmx.c i386/cputest.c \
	i386/dsputil_mmx.c i386/mpegvideo_mmx.c \
	i386/idct_mmx.c i386/motion_est_mmx.c \
	i386/simple_idct_mmx.c
endif

# armv4l specific stuff
if FFMPEG_TARGET_ARCH_ARMV4L
ASM_EXTRASOURCES +=armv4l/jrevdct_arm.c
EXTRASOURCES +=armv4l/dsputil_arm.c armv4l/mpegvideo_arm.c
endif

# sun mediaLib specific stuff
# currently only works when libavcodec is used in mplayer
if FFMPEG_HAVE_MLIB
EXTRASOURCES +=mlib/dsputil_mlib.c
AM_CFLAGS += $(MLIB_INC)
endif

# alpha specific stuff
if FFMPEG_TARGET_ARCH_ALPHA
EXTRASOURCES +=alpha/dsputil_alpha.c alpha/mpegvideo_alpha.c \
	alpha/simple_idct_alpha.c alpha/motion_est_alpha.c
ASM_EXTRASOURCES +=alpha/dsputil_alpha_asm.c alpha/motion_est_mvi_asm.c
AM_CFLAGS += -fforce-addr -freduce-all-givs
endif

if FFMPEG_TARGET_ARCH_POWERPC
EXTRASOURCES +=ppc/dsputil_ppc.c ppc/mpegvideo_ppc.c
endif

if FFMPEG_TARGET_MMI
EXTRASOURCES +=ps2/dsputil_mmi.c ps2/idct_mmi.c ps2/mpegvideo_mmi.c
endif

if FFMPEG_TARGET_ALTIVEC
AM_CFLAGS += -faltivec
EXTRASOURCES +=ppc/dsputil_altivec.c ppc/mpegvideo_altivec.c ppc/idct_altivec.c
endif

lib_LTLIBRARIES=libavcodec.la
libavcodec_la_SOURCES=common.c utils.c mem.c allcodecs.c \
      mpegvideo.c h263.c jrevdct.c jfdctfst.c jfdctint.c\
      mpegaudio.c ac3enc.c mjpeg.c resample.c dsputil.c \
      motion_est.c imgconvert.c imgresample.c msmpeg4.c \
      mpeg12.c h263dec.c svq1.c rv10.c mpegaudiodec.c pcm.c simple_idct.c \
      ratecontrol.c adpcm.c eval.c dv.c error_resilience.c \
      wmadec.c fft.c mdct.c mace.c $(EXTRASOURCES) $(ASM_EXTRASOURCES)
libavcodec_la_LDFLAGS=-static $(EXTRALIBS)

