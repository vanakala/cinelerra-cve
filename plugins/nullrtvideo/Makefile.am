plugin_LTLIBRARIES = nullrtvideo.la
nullrtvideo_la_LDFLAGS = -avoid-version -module -shared
nullrtvideo_la_LIBADD =
nullrtvideo_la_SOURCES = nullrtvideo.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h nullrtvideo.h
EXTRA_DIST = picon.png
