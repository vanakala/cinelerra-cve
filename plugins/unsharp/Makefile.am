plugin_LTLIBRARIES = unsharp.la
unsharp_la_LDFLAGS = -avoid-version -module -shared 
unsharp_la_LIBADD = 
unsharp_la_SOURCES = unsharp.C unsharpwindow.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = \
	unsharp.h unsharp.inc \
	unsharpwindow.h unsharpwindow.inc \
	picon_png.h 
EXTRA_DIST = picon.png
