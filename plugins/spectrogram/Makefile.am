plugin_LTLIBRARIES = spectrogram.la
spectrogram_la_LDFLAGS = -avoid-version -module -shared 
spectrogram_la_LIBADD = ../libfourier/libfourier.la
spectrogram_la_SOURCES = spectrogram.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/libfourier -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h spectrogram.h 
EXTRA_DIST = picon.png
