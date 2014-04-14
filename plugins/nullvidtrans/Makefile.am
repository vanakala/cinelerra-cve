plugin_LTLIBRARIES = nullvidtrans.la
nullvidtrans_la_LDFLAGS = -avoid-version -module -shared
nullvidtrans_la_LIBADD =
nullvidtrans_la_SOURCES = nullvidtrans.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h nullvidtrans.h
EXTRA_DIST = picon.png
