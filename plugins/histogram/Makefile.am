plugin_LTLIBRARIES = histogram.la
histogram_la_LDFLAGS = -avoid-version -module -shared 
histogram_la_LIBADD = $(top_builddir)/plugins/colors/libcolors.la
histogram_la_SOURCES = histogram.C histogramconfig.C histogramwindow.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/colors
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = \
	histogram.h histogramconfig.h histogramwindow.h \
	histogram.inc histogramconfig.inc histogramwindow.inc \
	max_picon_png.h mid_picon_png.h min_picon_png.h picon_png.h 
EXTRA_DIST = picon.png max_picon.png mid_picon.png min_picon.png
