plugin_LTLIBRARIES = timeavg.la
timeavg_la_LDFLAGS = -avoid-version -module -shared 
timeavg_la_LIBADD = 
timeavg_la_SOURCES = timeavg.C timeavgwindow.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h timeavg.h timeavgwindow.h 
EXTRA_DIST = picon.png
