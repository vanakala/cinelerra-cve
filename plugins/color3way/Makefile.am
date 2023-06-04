plugin_LTLIBRARIES = color3way.la
color3way_la_LDFLAGS = -avoid-version -module -shared
color3way_la_LIBADD =
color3way_la_SOURCES = color3way.C color3waywindow.C

AM_CXXFLAGS = $(CINELERRA_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
    -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = aggregated.h color3way.h color3waywindow.h

EXTRA_DIST = picon.png

BUILT_SOURCES = picon_png.h

picon_png.h: picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }

clean-local:
	$(RM) picon_png.h
