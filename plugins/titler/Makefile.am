plugin_LTLIBRARIES = titler.la
titler_la_LDFLAGS = -avoid-version -module -shared 
titler_la_LIBADD = $(top_srcdir)/plugins/libcolors/libcolors.la -lfreetype
titler_la_SOURCES = title.C titlewindow.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I/usr/include/freetype2 -I$(top_srcdir)/plugins/libcolors
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)
