plugin_LTLIBRARIES = histogram.la
histogram_la_LDFLAGS = -avoid-version -module -shared 
histogram_la_LIBADD = $(top_srcdir)/plugins/libcolors/libcolors.la
histogram_la_SOURCES = histogram.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)
