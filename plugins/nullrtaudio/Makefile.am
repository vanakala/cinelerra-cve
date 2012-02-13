plugin_LTLIBRARIES = nullrtaudio.la
nullrtaudio_la_LDFLAGS = -avoid-version -module -shared
nullrtaudio_la_LIBADD =
nullrtaudio_la_SOURCES = nullrtaudio.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h nullrtaudio.h
EXTRA_DIST = picon.png
