plugin_LTLIBRARIES = sharpen.la
sharpen_la_LDFLAGS = -avoid-version -module -shared 
sharpen_la_LIBADD = 
sharpen_la_SOURCES = sharpen.C sharpenwindow.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h sharpen.h sharpenwindow.h 
EXTRA_DIST = picon.png
