plugin_LTLIBRARIES = flip.la
flip_la_LDFLAGS = -avoid-version -module -shared 
flip_la_LIBADD = 
flip_la_SOURCES = flip.C flipwindow.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = flip.h flipwindow.h picon_png.h 
EXTRA_DIST = picon.png
