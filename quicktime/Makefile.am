SUBDIRS=encore50 


lib_LTLIBRARIES = libquicktimehv.la
libquicktimehv_la_CFLAGS = $(CPU_CFLAGS) $(FIREWIRE_CFLAGS) $(OGG_CFLAGS) $(VORBIS_CFLAGS) $(LIBAVCODEC_CFLAGS) -DENCORE_INCLUDE=\"encore50/encore.h\"
libquicktimehv_la_LDFLAGS = -version-info 1:0:0 -release 1.6.0 -Wl,--no-undefined 

libquicktimehv_la_LIBADD = $(FIREWIRE_LDFLAGS) -ldv -lxvidcore -ljpeg -lpng \
                                                 -logg -lvorbis -lvorbisenc -lmp3lame $(LIBAVCODEC_LIBS) \
                                                 encore50/libencore.la \
                                                $(top_builddir)/libmpeg3/libmpeg3hv.la -lpthread -lm

libquicktimehv_la_SOURCES = atom.c avi_hdrl.c avi_idx1.c avi_movi.c avi_strl.c \
			  avi_odml.c avi_ix.c avi_indx.c avi_riff.c \
													cmodel_default.c cmodel_yuv420p.c cmodel_yuv422.c codecs.c colormodels.c \
													ctab.c dinf.c dref.c dv.c edts.c elst.c graphics.c hdlr.c \
													ima4.c jpeg.c libdv.c libmjpeg.c matrix.c mdat.c mdhd.c mdia.c \
													minf.c moov.c mvhd.c plugin.c qtpng.c qtmp3.c quicktime.c raw.c rawaudio.c \
	rle.c \
													smhd.c stbl.c stco.c stsc.c stsd.c stsdtable.c stss.c stsz.c stts.c \
													tkhd.c trak.c twos.c udta.c ulaw.c util.c v308.c v408.c v410.c vmhd.c \
													vorbis.c workarounds.c yuv2.c yuv4.c yv12.c wmx2.c \
													wma.c mpeg4.c

noinst_HEADERS = cmodel_permutation.h \
	codecs.h \
	colormodels.h \
	div3.h \
	divx.h \
	dv.h \
	fastjpg.h \
	fastjpgmacro.h \
	funcprotos.h \
	graphics.h \
	ima4.h \
	jpeg.h \
	jpeg_old.h \
	libdv.h \
	libmjpeg.h \
	mmx.h \
	mpeg4.h \
	qtmp3.h \
	qtpng.h \
	qtprivate.h \
	qtvorbis.h \
	quicktime.h \
	raw.h \
	rawaudio.h \
	rle.h \
	rtjpeg.h \
	rtjpeg_core.h \
	twos.h \
	ulaw.h \
	v308.h \
	v408.h \
	v410.h \
	wma.h \
	wmx1.h \
	wmx2.h \
	workarounds.h \
	xanimjpeg.h \
	yuv2.h \
	yuv4.h \
	yuv9.h \
	yv12.h
	
EXTRA_DIST = docs

INCLUDES = -I$(top_srcdir)/libmpeg3

pkgincludedir=$(includedir)/quicktime
pkginclude_HEADERS=quicktime.h qtprivate.h

LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTC_FLAGS)

dist-hook:
	-rm -rf $(distdir)/docs/CVS
