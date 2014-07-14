plugin_LTLIBRARIES = aging.la
aging_la_LDFLAGS = -avoid-version -module -shared 
aging_la_LIBADD = $(top_builddir)/plugins/libeffecttv/libeffecttv.la
aging_la_SOURCES = aging.C agingwindow.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime \
	-I$(top_srcdir)/plugins/libeffecttv -I$(top_srcdir)/plugins/colors -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = aging.h agingwindow.h
EXTRA_DIST = picon.png
BUILT_SOURCES = picon_png.h

picon_png.h: picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }

clean-local:
	$(RM) picon_png.h
