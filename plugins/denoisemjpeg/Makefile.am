plugin_LTLIBRARIES = denoisemjpeg.la
denoisemjpeg_la_LDFLAGS = -avoid-version -module -shared
denoisemjpeg_la_LIBADD =
denoisemjpeg_la_SOURCES = denoisemjpeg.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = denoisemjpeg.h picon_png.h 
EXTRA_DIST = picon.png
