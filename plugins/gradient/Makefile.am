plugin_LTLIBRARIES = gradient.la
gradient_la_LDFLAGS = -avoid-version -module -shared 
gradient_la_LIBADD = $(top_srcdir)/plugins/libcolors/libcolors.la
gradient_la_SOURCES = gradient.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I../libcolors
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)
