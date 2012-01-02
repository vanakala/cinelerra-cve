plugin_LTLIBRARIES = c41.la
c41_la_LDFLAGS = -avoid-version -module -shared 
c41_la_LIBADD = $(top_builddir)/plugins/colors/libcolors.la
c41_la_SOURCES = c41.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime \
	-I$(top_srcdir)/plugins/colors  -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h 
EXTRA_DIST = picon.png
