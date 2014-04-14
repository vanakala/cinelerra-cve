plugin_LTLIBRARIES = 720to480.la
720to480_la_LDFLAGS = -avoid-version -module -shared
720to480_la_LIBADD =
720to480_la_SOURCES = 720to480.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = 720to480.h picon_png.h
EXTRA_DIST = picon.png
