plugin_LTLIBRARIES = bandwipe.la
bandwipe_la_LDFLAGS = -avoid-version -module -shared 
bandwipe_la_LIBADD = 
bandwipe_la_SOURCES = bandwipe.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = bandwipe.h picon_png.h 
EXTRA_DIST = picon.png
