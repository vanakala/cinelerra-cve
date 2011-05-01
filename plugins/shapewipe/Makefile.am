plugin_LTLIBRARIES = shapewipe.la
shapewipe_la_LDFLAGS = -avoid-version -module -shared 
shapewipe_la_LIBADD = 
shapewipe_la_SOURCES = shapewipe.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS) \
	-DDEFAULT_SHAPE=\"$(shapewipedir)/heart.png\"

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

shapewipedir = $(plugindir)/shapewipe
shapewipe_DATA = \
	circle.png \
	clock.png \
	heart.png \
	tile2x2h.png \
	tile2x2v.png

noinst_HEADERS = shapewipe.h picon_png.h 
EXTRA_DIST = picon.png $(shapewipe_DATA)
