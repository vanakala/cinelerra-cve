plugin_LTLIBRARIES = chromakeyhsv.la
chromakeyhsv_la_LDFLAGS = -avoid-version -module -shared 
chromakeyhsv_la_LIBADD = $(top_builddir)/plugins/colors/libcolors.la
chromakeyhsv_la_SOURCES = chromakey.C chromakey_sl.c
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/colors
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

BUILTSOURCES = chromakey_sl.c

chromakey_sl.c: chromakey.sl
	(echo "const char _binary_chromakey_sl_start[] ="; \
	sed -e 's/\\/\\\\/' -e 's/"/\\"/' \
	-e 's/^/"/' -e 's/$$/\\n"/' < $(srcdir)/chromakey.sl; \
	echo ";") > chromakey_sl.c

noinst_HEADERS = chromakey.h picon_png.h 
EXTRA_DIST = chromakey.sl picon.png
