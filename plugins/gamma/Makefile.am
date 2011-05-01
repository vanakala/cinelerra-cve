plugin_LTLIBRARIES = gamma.la
gamma_la_LDFLAGS = -avoid-version -module -shared
gamma_la_LIBADD = $(top_builddir)/plugins/colors/libcolors.la
gamma_la_SOURCES = gamma.C gammawindow.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/colors -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = aggregated.h gamma.h gammawindow.h picon_png.h
EXTRA_DIST = picon.png
