plugin_LTLIBRARIES = slide.la
slide_la_LDFLAGS = -avoid-version -module -shared 
slide_la_LIBADD = 
slide_la_SOURCES = slide.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h slide.h 
EXTRA_DIST = picon.png
