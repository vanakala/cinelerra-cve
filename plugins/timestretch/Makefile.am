plugin_LTLIBRARIES = timestretch.la
timestretch_la_LDFLAGS = -avoid-version -module -shared 
timestretch_la_LIBADD = ../libfourier/libfourier.la
timestretch_la_SOURCES = timestretch.C timestretchengine.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/libfourier
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h timestretch.h timestretchengine.h timestretchengine.inc
EXTRA_DIST = picon.png
