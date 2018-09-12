plugin_LTLIBRARIES = chromakeyhsv.la
chromakeyhsv_la_LDFLAGS = -avoid-version -module -shared 
chromakeyhsv_la_LIBADD =
chromakeyhsv_la_SOURCES = chromakey.C chromakey_sl.c
AM_CXXFLAGS = $(CINELERRA_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/include $(FFMPEG_CPPFLAGS)
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

BUILTSOURCES = chromakey_sl.c

chromakey_sl.c: chromakey.sl
	(echo "const char _binary_chromakey_sl_start[] ="; \
	sed -e 's/\\/\\\\/' -e 's/"/\\"/' \
	-e 's/^/"/' -e 's/$$/\\n"/' < $(srcdir)/chromakey.sl; \
	echo ";") > chromakey_sl.c

noinst_HEADERS = chromakey.h
EXTRA_DIST = chromakey.sl picon.png
BUILT_SOURCES = picon_png.h

picon_png.h: picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }

clean-local:
	$(RM) picon_png.h
