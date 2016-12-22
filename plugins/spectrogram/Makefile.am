plugin_LTLIBRARIES = spectrogram.la
spectrogram_la_LDFLAGS = -avoid-version -module -shared 
spectrogram_la_LIBADD = ../libfourier/libfourier.la
spectrogram_la_SOURCES = spectrogram.C 
AM_CXXFLAGS = $(CINELERRA_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/plugins/libfourier -I$(top_srcdir)/include \
	$(FFMPEG_CPPFLAGS)
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = spectrogram.h 
EXTRA_DIST = picon.png
BUILT_SOURCES = picon_png.h

picon_png.h: picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }

clean-local:
	$(RM) picon_png.h
