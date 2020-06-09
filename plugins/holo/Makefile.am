plugin_LTLIBRARIES = holo.la
holo_la_LDFLAGS = -avoid-version -module -shared 
holo_la_LIBADD = $(top_builddir)/plugins/libeffecttv/libeffecttv.la
holo_la_SOURCES = holo.C holowindow.C
AM_CXXFLAGS = $(CINELERRA_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/plugins/libeffecttv \
	-I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = holo.h holowindow.h
EXTRA_DIST = picon.png
BUILT_SOURCES = picon_png.h

picon_png.h: picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }

clean-local:
	$(RM) picon_png.h
