plugin_LTLIBRARIES = reframe.la
reframe_la_LDFLAGS = -avoid-version -module -shared 
reframe_la_LIBADD = 
reframe_la_SOURCES = reframe.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h reframe.h 
EXTRA_DIST = picon.png
