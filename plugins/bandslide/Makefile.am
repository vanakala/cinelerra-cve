plugin_LTLIBRARIES = bandslide.la
bandslide_la_LDFLAGS = -avoid-version -module -shared 
bandslide_la_LIBADD = 
bandslide_la_SOURCES = bandslide.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = bandslide.h picon_png.h 
EXTRA_DIST = picon.png
