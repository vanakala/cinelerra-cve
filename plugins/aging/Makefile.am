plugin_LTLIBRARIES = aging.la
aging_la_LDFLAGS = -avoid-version -module -shared 
aging_la_LIBADD = $(top_builddir)/plugins/libeffecttv/libeffecttv.la
aging_la_SOURCES = aging.C agingwindow.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/libeffecttv -I$(top_srcdir)/plugins/colors
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = aging.h agingwindow.h picon_png.h 
EXTRA_DIST = picon.png
