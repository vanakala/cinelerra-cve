plugin_LTLIBRARIES = quark.la
quark_la_LDFLAGS = -avoid-version -module -shared
quark_la_LIBADD =
quark_la_SOURCES = quark.C quarkwindow.C
AM_CFLAGS =
AM_CXXFLAGS =

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h quark.h quarkwindow.h 
EXTRA_DIST = picon.png
