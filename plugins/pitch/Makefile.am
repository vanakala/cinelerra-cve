plugin_LTLIBRARIES = pitch.la
pitch_la_LDFLAGS = -avoid-version -module -shared 
pitch_la_LIBADD = $(top_builddir)/plugins/libfourier/libfourier.la
pitch_la_SOURCES = pitch.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/libfourier
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h pitch.h 
EXTRA_DIST = picon.png
