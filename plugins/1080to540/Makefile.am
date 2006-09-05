plugin_LTLIBRARIES = 1080to540.la
1080to540_la_LDFLAGS = -avoid-version -module -shared
1080to540_la_LIBADD =
1080to540_la_SOURCES = 1080to540.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = 1080to540.h picon_png.h 
EXTRA_DIST = picon.png
