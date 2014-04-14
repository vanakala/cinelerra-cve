plugin_LTLIBRARIES = brightness.la
brightness_la_LDFLAGS = -avoid-version -module -shared 
brightness_la_LIBADD = $(top_builddir)/plugins/colors/libcolors.la
brightness_la_SOURCES = brightness.C brightnesswindow.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
    -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/colors -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = brightness.h brightnesswindow.h picon_png.h 
EXTRA_DIST = picon.png
