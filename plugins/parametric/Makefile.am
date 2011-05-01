plugin_LTLIBRARIES = parametric.la
parametric_la_LDFLAGS = -avoid-version -module -shared 
parametric_la_LIBADD = $(top_builddir)/plugins/libfourier/libfourier.la
parametric_la_SOURCES = parametric.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/libfourier -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = parametric.h picon_png.h 
EXTRA_DIST = picon.png
