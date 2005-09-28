plugin_LTLIBRARIES = linearize.la
linearize_la_LDFLAGS = -avoid-version -module -shared 
linearize_la_LIBADD = 
linearize_la_SOURCES = linearize.C linearizewindow.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/colors
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = linearize.h linearizewindow.h picon_png.h 
EXTRA_DIST = picon.png
