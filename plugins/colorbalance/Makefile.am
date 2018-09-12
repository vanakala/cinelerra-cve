plugin_LTLIBRARIES = colorbalance.la
colorbalance_la_LDFLAGS = -avoid-version -module -shared 
colorbalance_la_LIBADD =
colorbalance_la_SOURCES = colorbalance.C colorbalancewindow.C 
AM_CXXFLAGS = $(CINELERRA_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/include \
	$(FFMPEG_CPPFLAGS)
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = aggregated.h colorbalance.h colorbalancewindow.h
EXTRA_DIST = picon.png

BUILT_SOURCES = picon_png.h

picon_png.h: picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }

clean-local:
	$(RM) picon_png.h

