plugin_LTLIBRARIES = gain.la
gain_la_LDFLAGS = -avoid-version -module -shared 
gain_la_LIBADD = 
gain_la_SOURCES = gain.C gainwindow.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = gain.h gainwindow.h picon_png.h 
EXTRA_DIST = picon.png
