plugin_LTLIBRARIES = colorbalance.la
colorbalance_la_LDFLAGS = -avoid-version -module -shared 
colorbalance_la_LIBADD = $(top_builddir)/plugins/colors/libcolors.la
colorbalance_la_SOURCES = colorbalance.C colorbalancewindow.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/colors
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = colorbalance.h colorbalancewindow.h picon_png.h 
EXTRA_DIST = picon.png
