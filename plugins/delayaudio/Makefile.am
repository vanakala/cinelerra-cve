plugin_LTLIBRARIES = delayaudio.la
delayaudio_la_LDFLAGS = -avoid-version -module -shared 
delayaudio_la_LIBADD = 
delayaudio_la_SOURCES = delayaudio.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = delayaudio.h picon_png.h 
EXTRA_DIST = picon.png
