plugin_LTLIBRARIES = histogram.la
histogram_la_LDFLAGS = -avoid-version -module -shared 
histogram_la_LIBADD = $(top_builddir)/plugins/colors/libcolors.la
histogram_la_SOURCES = histogram.C histogramconfig.C histogramwindow.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/plugins/colors -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = aggregated.h histogram.h histogramconfig.h \
	histogramwindow.h histogram.inc 

EXTRA_DIST = picon.png max_picon.png mid_picon.png min_picon.png
BUILT_SOURCES = picon_png.h max_picon_png.h mid_picon_png.h min_picon_png.h

picon_png.h: picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }

max_picon_png.h: max_picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }

mid_picon_png.h: mid_picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }

min_picon_png.h: min_picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }

clean-local:
	$(RM) picon_png.h max_picon_png.h mid_picon_png.h min_picon_png.h
