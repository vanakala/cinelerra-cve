plugin_LTLIBRARIES = invertvideo.la
invertvideo_la_LDFLAGS = -avoid-version -module -shared 
invertvideo_la_LIBADD = $(top_builddir)/plugins/colors/libcolors.la
invertvideo_la_SOURCES = invert.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/colors
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = invert.h picon_png.h 
EXTRA_DIST = picon.png
