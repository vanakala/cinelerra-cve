# i386 mmx specific stuff
if USEMMX
MM_DIR = i386
MM_LIB = i386/libavcodeci386.la
MM_FLAGS = -DHAVE_MMX
if TARGET_BUILTIN_VECTOR
MM_FLAGS += -DHAVE_BUILTIN_VECTOR
endif
endif

SUBDIRS = $(MM_DIR)

noinst_LTLIBRARIES = libavcodec.la

AM_CFLAGS = \
	$(LARGEFILE_CFLAGS) \
	$(CPU_CFLAGS) \
	$(MM_FLAGS) \
	-D_GNU_SOURCE -DHAVE_AV_CONFIG_H -I$(srcdir)/..

libavcodec_la_LIBADD = $(MM_LIB)
#CFLAGS=$(OPTFLAGS) -DHAVE_AV_CONFIG_H -I.. -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_GNU_SOURCE $(AMR_CFLAGS)

libavcodec_la_SOURCES = \
	bitstream.c \
	utils.c \
	mem.c \
	allcodecs.c \
	mpegvideo.c \
	jrevdct.c \
	jfdctfst.c \
	jfdctint.c \
	mpegaudio.c \
	ac3enc.c \
	mjpeg.c \
	resample.c \
	resample2.c \
	dsputil.c  \
	motion_est.c \
	imgconvert.c \
	imgresample.c  \
	mpeg12.c \
	mpegaudiodec.c \
	pcm.c \
	simple_idct.c  \
	ratecontrol.c \
	adpcm.c \
	eval.c \
	error_resilience.c  \
	fft.c \
	mdct.c \
	raw.c \
	golomb.c \
	cabac.c \
	dpcm.c \
	adx.c \
	rational.c \
	faandct.c \
	parser.c \
	g726.c  \
	vp3dsp.c \
	integer.c \
	h264idct.c \
	rangecoder.c \
	pnm.c \
	h263.c \
	msmpeg4.c \
	h263dec.c \
	dvdsub.c \
	dvbsub.c \
	dvbsubdec.c

libavcodec_la_SOURCES += aasc.c 
libavcodec_la_SOURCES += alac.c
libavcodec_la_SOURCES += asv1.c
libavcodec_la_SOURCES += cinepak.c
libavcodec_la_SOURCES += cljr.c
libavcodec_la_SOURCES += cyuv.c
libavcodec_la_SOURCES += dv.c
libavcodec_la_SOURCES += 8bps.c
libavcodec_la_SOURCES += ffv1.c
libavcodec_la_SOURCES += flac.c
libavcodec_la_SOURCES += flicvideo.c
libavcodec_la_SOURCES += 4xm.c
libavcodec_la_SOURCES += fraps.c
libavcodec_la_SOURCES += h261.c
libavcodec_la_SOURCES += h264.c
libavcodec_la_SOURCES += huffyuv.c
libavcodec_la_SOURCES += idcinvideo.c
libavcodec_la_SOURCES += indeo2.c
libavcodec_la_SOURCES += indeo3.c
libavcodec_la_SOURCES += interplayvideo.c
libavcodec_la_SOURCES += lcl.c
libavcodec_la_SOURCES += loco.c
libavcodec_la_SOURCES += mace.c
libavcodec_la_SOURCES += msrle.c
libavcodec_la_SOURCES += msvideo1.c
libavcodec_la_SOURCES += png.c
libavcodec_la_SOURCES += qdrw.c
libavcodec_la_SOURCES += qpeg.c
libavcodec_la_SOURCES += qtrle.c
libavcodec_la_SOURCES += ra144.c
libavcodec_la_SOURCES += ra288.c
libavcodec_la_SOURCES += roqvideo.c
libavcodec_la_SOURCES += rpza.c
libavcodec_la_SOURCES += rv10.c
libavcodec_la_SOURCES += shorten.c
libavcodec_la_SOURCES += smc.c
libavcodec_la_SOURCES += snow.c
libavcodec_la_SOURCES += sonic.c
libavcodec_la_SOURCES += svq1.c
libavcodec_la_SOURCES += truemotion1.c
libavcodec_la_SOURCES += tscc.c
libavcodec_la_SOURCES += ulti.c
libavcodec_la_SOURCES += vc9.c
libavcodec_la_SOURCES += vcr1.c
libavcodec_la_SOURCES += vmdav.c
libavcodec_la_SOURCES += vorbis.c
libavcodec_la_SOURCES += vp3.c
libavcodec_la_SOURCES += vqavideo.c
libavcodec_la_SOURCES += wmadec.c
libavcodec_la_SOURCES += wnv1.c
libavcodec_la_SOURCES += ws-snd1.c
libavcodec_la_SOURCES += xan.c
libavcodec_la_SOURCES += xl.c
#libavcodec_la_SOURCES += pthread.c
libavcodec_la_SOURCES += xvmcvideo.c
libavcodec_la_SOURCES += a52dec.c
libavcodec_la_LIBADD += -la52
#libavcodec_la_SOURCES += dtsdec.c
#AM_CFLAGS += $(DTS_INC)
#libavcodec_la_LIBADD += -ldts
libavcodec_la_SOURCES += faad.c
libavcodec_la_LIBADD += -lfaad
libavcodec_la_SOURCES += faac.c
libavcodec_la_LIBADD += -lfaac
#libavcodec_la_SOURCES += xvidff.c
#libavcodec_la_LIBADD += -lxvidcore
libavcodec_la_SOURCES += x264.c
libavcodec_la_LIBADD += -lx264
libavcodec_la_SOURCES  += mp3lameaudio.c
libavcodec_la_LIBADD += -lmp3lame
libavcodec_la_SOURCES  += oggvorbis.c
libavcodec_la_LIBADD += -lvorbisenc -lvorbis
libavcodec_la_SOURCES  += oggtheora.c
libavcodec_la_LIBADD += -ltheora
libavcodec_la_LIBADD += -logg
#libavcodec_la_SOURCES  += libgsm.c
#libavcodec_la_LIBADD += -lgsm

# armv4l specific stuff
#ifeq ($(TARGET_ARCH_ARMV4L),yes)
#ASM_OBJS += armv4l/jrevdct_arm.o armv4l/simple_idct_arm.o armv4l/dsputil_arm_s.o
#libavcodec_la_SOURCES  += armv4l/dsputil_arm.o armv4l/mpegvideo_arm.o
#ifeq ($(TARGET_IWMMXT),yes)
#libavcodec_la_SOURCES  += armv4l/dsputil_iwmmxt.o armv4l/mpegvideo_iwmmxt.o
#endif
#endif

# sun mediaLib specific stuff
# currently only works when libavcodec is used in mplayer
#ifeq ($(HAVE_MLIB),yes)
#libavcodec_la_SOURCES  += mlib/dsputil_mlib.o
#CFLAGS += $(MLIB_INC)
#endif

#if TARGET_ARCH_POWERPC
#libavcodec_la_SOURCES  += ppc/dsputil_ppc.o ppc/mpegvideo_ppc.o
#endif

#if TARGET_ALTIVEC
#libavcodec_la_SOURCES  += ppc/dsputil_altivec.o ppc/mpegvideo_altivec.o ppc/idct_altivec.o \
#        ppc/fft_altivec.o ppc/gmc_altivec.o ppc/fdct_altivec.o \
#        ppc/dsputil_h264_altivec.o
#endif

EXTRA_DIST = liba52 ppc \
	ac3.h \
	ac3tab.h \
	avcodec.h \
	bitstream.h \
	bswap.h \
	cabac.h \
	common.h \
	dsputil.h \
	dvdata.h \
	faandct.h \
	golomb.h \
	h261data.h \
	h263data.h \
	h264data.h \
	imgconvert_template.h \
	indeo2data.h \
	indeo3data.h \
	integer.h \
	mdec.c \
	motion_est_template.c \
	mpeg12data.h \
	mpeg4data.h \
	mpegaudio.h \
	mpegaudiodectab.h \
	mpegaudiotab.h \
	mpegvideo.h \
	msmpeg4data.h \
	ra144.h \
	ra288.h \
	rangecoder.h \
	rational.h \
	simple_idct.h \
	sp5x.h \
	svq1_cb.h \
	svq1_vlc.h \
	svq3.c \
	truemotion1data.h \
	ulti_cb.h \
	vc9data.h \
	vorbis.h \
	vp3data.h \
	wmadata.h \
	wmv2.c
