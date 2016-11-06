plugin_LTLIBRARIES = invertvideo.la
invertvideo_la_LDFLAGS = -avoid-version -module -shared 
invertvideo_la_LIBADD = $(top_builddir)/plugins/colors/libcolors.la
invertvideo_la_SOURCES = invert.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/plugins/colors -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

EXTRA_DIST = picon.png
BUILT_SOURCES = picon_png.h

picon_png.h: picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }

clean-local:
	$(RM) picon_png.h
