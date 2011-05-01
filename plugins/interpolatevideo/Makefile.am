plugin_LTLIBRARIES = interpolatevideo.la
interpolatevideo_la_LDFLAGS = -avoid-version -module -shared 
interpolatevideo_la_LIBADD = 
interpolatevideo_la_SOURCES = interpolatevideo.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h 
EXTRA_DIST = picon.png
