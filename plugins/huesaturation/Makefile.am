plugin_LTLIBRARIES = huesaturation.la
huesaturation_la_LDFLAGS = -avoid-version -module -shared 
huesaturation_la_LIBADD = $(top_builddir)/plugins/libcolors/libcolors.la
huesaturation_la_SOURCES = huesaturation.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/libcolors
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = huesaturation.h picon_png.h 
EXTRA_DIST = picon.png
