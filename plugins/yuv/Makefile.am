plugin_LTLIBRARIES = yuv.la
yuv_la_LDFLAGS = -avoid-version -module -shared 
yuv_la_LIBADD = $(top_srcdir)/plugins/libcolors/libcolors.la
yuv_la_SOURCES = yuv.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/libcolors
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)
