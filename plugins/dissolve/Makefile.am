plugin_LTLIBRARIES = dissolve.la
dissolve_la_LDFLAGS = -avoid-version -module -shared 
dissolve_la_LIBADD = 
dissolve_la_SOURCES = dissolve.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = dissolve.h picon_png.h 
EXTRA_DIST = picon.png
