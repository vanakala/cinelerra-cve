plugin_LTLIBRARIES = burn.la
burn_la_LDFLAGS = -avoid-version -module -shared 
burn_la_LIBADD = $(top_builddir)/plugins/libeffecttv/libeffecttv.la
burn_la_SOURCES = burn.C burnwindow.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/libeffecttv \
	-I$(top_srcdir)/plugins/colors -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = burn.h burnwindow.h picon_png.h 
EXTRA_DIST = picon.png
