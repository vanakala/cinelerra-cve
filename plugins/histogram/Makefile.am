plugin_LTLIBRARIES = histogram.la
histogram_la_LDFLAGS = -avoid-version -module -shared 
histogram_la_LIBADD =
histogram_la_SOURCES = histogram.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = max_picon_png.h mid_picon_png.h min_picon_png.h picon_png.h 
EXTRA_DIST = picon.png max_picon.png mid_picon.png min_picon.png
