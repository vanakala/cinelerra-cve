plugin_LTLIBRARIES = denoisevideo.la
denoisevideo_la_LDFLAGS = -avoid-version -module -shared 
denoisevideo_la_LIBADD = 
denoisevideo_la_SOURCES = denoisevideo.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = denoisevideo.h picon_png.h 
EXTRA_DIST = picon.png
