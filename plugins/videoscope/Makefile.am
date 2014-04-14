plugin_LTLIBRARIES = videoscope.la
videoscope_la_LDFLAGS = -avoid-version -module -shared 
videoscope_la_LIBADD = $(top_builddir)/plugins/colors/libcolors.la
videoscope_la_SOURCES = videoscope.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/colors \
	-I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h 
EXTRA_DIST = picon.png
