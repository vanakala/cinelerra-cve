plugin_LTLIBRARIES = chromakey.la
chromakey_la_LDFLAGS = -avoid-version -module -shared 
chromakey_la_LIBADD = $(top_builddir)/plugins/colors/libcolors.la
chromakey_la_SOURCES = chromakey.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/colors
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = chromakey.h picon_png.h 
EXTRA_DIST = picon.png
