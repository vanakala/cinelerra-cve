plugin_LTLIBRARIES = svg.la
svg_la_LDFLAGS = -avoid-version -module -shared 
svg_la_LIBADD = 
svg_la_SOURCES = svg.C svgwin.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = empty_svg.h picon_png.h svg.h svgwin.h 
EXTRA_DIST = picon.png
