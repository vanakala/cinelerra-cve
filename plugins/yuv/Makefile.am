plugin_LTLIBRARIES = yuv.la
yuv_la_LDFLAGS = -avoid-version -module -shared 
yuv_la_LIBADD = $(top_builddir)/plugins/colors/libcolors.la
yuv_la_SOURCES = yuv.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/colors
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h yuv.h
EXTRA_DIST = picon.png
