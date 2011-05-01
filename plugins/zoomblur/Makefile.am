plugin_LTLIBRARIES = zoomblur.la
zoomblur_la_LDFLAGS = -module -shared -avoid-version
zoomblur_la_LIBADD =
zoomblur_la_SOURCES = zoomblur.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h 
EXTRA_DIST = picon.png
