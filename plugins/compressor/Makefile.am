plugin_LTLIBRARIES = compressor.la
compressor_la_LDFLAGS = -avoid-version -module -shared 
compressor_la_LIBADD = 
compressor_la_SOURCES = compressor.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = compressor.h picon_png.h 
EXTRA_DIST = picon.png
