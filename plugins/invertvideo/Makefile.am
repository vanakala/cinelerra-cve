plugin_LTLIBRARIES = invertvideo.la
invertvideo_la_LDFLAGS = -avoid-version -module -shared 
invertvideo_la_LIBADD = $(top_srcdir)/plugins/libcolors/libcolors.la
invertvideo_la_SOURCES = invert.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/libcolors
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)
