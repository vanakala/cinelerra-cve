plugin_LTLIBRARIES = blurzoom.la
blurzoom_la_LDFLAGS = -avoid-version -module -shared
blurzoom_la_LIBADD =
blurzoom_la_SOURCES = blurzoom.C blurzoomwindow.C
AM_CFLAGS =
AM_CXXFLAGS =

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = blurzoom.h blurzoomwindow.h picon_png.h 
EXTRA_DIST = picon.png
