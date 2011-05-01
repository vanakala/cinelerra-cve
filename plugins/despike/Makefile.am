plugin_LTLIBRARIES = despike.la
despike_la_LDFLAGS = -avoid-version -module -shared 
despike_la_LIBADD = 
despike_la_SOURCES = despike.C despikewindow.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = despike.h despikewindow.h picon_png.h 
EXTRA_DIST = picon.png
