# This file was automatically generated from the Makefile.am
# DO NOT EDIT!

SUBDIRS = GSM610 G72x

lib_LTLIBRARIES = libsndfile.la
include_HEADERS = sndfile.h

EXTRA_DIST = sndfile.h.in config.h.in Version_script.in

noinst_HEADERS = common.h sfendian.h float_cast.h wav_w64.h au.h

COMMON = sndfile.c common.c file_io.c command.c pcm.c ulaw.c alaw.c float32.c \
		double64.c ima_adpcm.c ms_adpcm.c gsm610.c dwvw.c sfendian.c vox_adpcm.c \
		interleave.c strings.c dither.c

FILESPECIFIC = aiff.c au.c au_g72x.c ircam.c mat4.c mat5.c nist.c paf.c \
				pvf.c raw.c svx.c voc.c w64.c wav.c wav_w64.c htk.c \
				sd2.c rx2.c txw.c sds.c wve.c dwd.c ogg.c xi.c
				
libsndfile_la_SOURCES = $(COMMON) $(FILESPECIFIC) $(noinst_HEADERS) sndfile.h
libsndfile_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@
libsndfile_la_LIBADD  = GSM610/libgsm.la G72x/libg72x.la -lm

# Disable autoheader. 
AUTOHEADER=echo

aiff.c au.c au_g72x.c ircam.c mat4.c mat5.c nist.c paf.c pvf.c : sndfile.h common.h
raw.c svx.c voc.c w64.c wav.c wav_w64.c htk.c sd2.c rx2.c txw.c : sndfile.h common.h
sds.c wve.c dwd.c ogg.c xi.c sndfile.c common.c file_io.c : sndfile.h common.h
command.c pcm.c ulaw.c alaw.c float32.c double64.c ima_adpcm.c : sndfile.h common.h 
ms_adpcm.c gsm610.c dwvw.c sfendian.c vox_adpcm.c interleave.c : sndfile.h common.h
strings.c dither.c : sndfile.h common.h
