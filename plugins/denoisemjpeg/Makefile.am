plugin_LTLIBRARIES = denoisemjpeg.la
denoisemjpeg_la_LDFLAGS = -avoid-version -module -shared
denoisemjpeg_la_LIBADD =
denoisemjpeg_la_SOURCES = denoisemjpeg.C
AM_CFLAGS =
AM_CXXFLAGS =

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = denoisemjpeg.h picon_png.h 
EXTRA_DIST = picon.png
