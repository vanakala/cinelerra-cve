plugin_LTLIBRARIES = interpolateaudio.la
interpolateaudio_la_LDFLAGS = -avoid-version -module -shared 
interpolateaudio_la_LIBADD = 
interpolateaudio_la_SOURCES = interpolateaudio.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h 
EXTRA_DIST = picon.png
