plugin_LTLIBRARIES = reversevideo.la
reversevideo_la_LDFLAGS = -avoid-version -module -shared 
reversevideo_la_LIBADD = 
reversevideo_la_SOURCES = reversevideo.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h
EXTRA_DIST = picon.png
