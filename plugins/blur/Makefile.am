plugin_LTLIBRARIES = blur.la
blur_la_LDFLAGS = -avoid-version -module -shared 
blur_la_LIBADD = 
blur_la_SOURCES = blur.C blurwindow.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = blur.h blur.inc blurwindow.h blurwindow.inc picon_png.h 
EXTRA_DIST = picon.png
