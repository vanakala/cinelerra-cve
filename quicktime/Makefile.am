SUBDIRS=decore50 encore50 


lib_LTLIBRARIES = libquicktime.la
libquicktime_la_CFLAGS = $(GLIB_CFLAGS) $(CPU_CFLAGS) $(FIREWIRE_CFLAGS) $(OGG_CFLAGS) $(VORBIS_CFLAGS) $(LIBAVCODEC_CFLAGS) -Iencore50/ -Idecore50/ -DDECORE_INCLUDE=\"decore50/decore.h\" -DENCORE_INCLUDE=\"encore50/encore.h\"
libquicktime_la_LDFLAGS = -version-info 1:0:0 -release 1.6.0 -Wl,--no-undefined 
libquicktime_la_LIBADD = $(GLIB_LIBS) $(FIREWIRE_LDFLAGS) -ldv -lxvidcore -ljpeg -lpng \
												 -logg -lvorbis -lvorbisenc -lmp3lame $(LIBAVCODEC_LIBS) \
												 encore50/libencore.la decore50/libdecore.la \
												 ../libmpeg3/libmpeg3.la -lpthread -lm
libquicktime_la_SOURCES = atom.c avi_hdrl.c avi_idx1.c avi_movi.c avi_strl.c \
			  avi_odml.c avi_ix.c avi_indx.c avi_riff.c \
													cmodel_default.c cmodel_yuv420p.c cmodel_yuv422.c codecs.c colormodels.c \
													ctab.c dinf.c dref.c dv.c edts.c elst.c graphics.c hdlr.c \
													ima4.c jpeg.c libdv.c libmjpeg.c matrix.c mdat.c mdhd.c mdia.c \
													minf.c moov.c mvhd.c plugin.c qtpng.c qtmp3.c quicktime.c raw.c rawaudio.c \
													smhd.c stbl.c stco.c stsc.c stsd.c stsdtable.c stss.c stsz.c stts.c \
													tkhd.c trak.c twos.c udta.c ulaw.c util.c v308.c v408.c v410.c vmhd.c \
													vorbis.c workarounds.c yuv2.c yuv4.c yv12.c wmx2.c \
													wma.c mpeg4.c

INCLUDES = -I/usr/include/lame -I$(top_srcdir)/libmpeg3 -I/usr/include/glib-1.2

pkgincludedir=$(includedir)/quicktime
pkginclude_HEADERS=quicktime.h qtprivate.h
