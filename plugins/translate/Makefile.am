plugin_LTLIBRARIES = translate.la
translate_la_LDFLAGS = -avoid-version -module -shared 
translate_la_LIBADD = 
translate_la_SOURCES = translate.C translatewin.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h translate.h translatewin.h 
EXTRA_DIST = picon.png
