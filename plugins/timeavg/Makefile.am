plugin_LTLIBRARIES = timeavg.la
timeavg_la_LDFLAGS = -avoid-version -module -shared 
timeavg_la_LIBADD = 
timeavg_la_SOURCES = timeavg.C timeavgwindow.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h timeavg.h timeavgwindow.h 
EXTRA_DIST = picon.png
