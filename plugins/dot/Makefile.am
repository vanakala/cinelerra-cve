plugin_LTLIBRARIES = dot.la
dot_la_LDFLAGS = -avoid-version -module -shared 
dot_la_LIBADD = $(top_builddir)/plugins/libeffecttv/libeffecttv.la
dot_la_SOURCES = dot.C dotwindow.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/libeffecttv -I$(top_srcdir)/plugins/colors
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = dot.h dotwindow.h picon_png.h 
EXTRA_DIST = picon.png
