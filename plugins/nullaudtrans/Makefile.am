plugin_LTLIBRARIES = nullaudtrans.la
nullaudtrans_la_LDFLAGS = -avoid-version -module -shared
nullaudtrans_la_LIBADD =
nullaudtrans_la_SOURCES = nullaudtrans.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h nullaudtrans.h
EXTRA_DIST = picon.png
