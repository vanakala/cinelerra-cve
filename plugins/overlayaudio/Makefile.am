plugin_LTLIBRARIES = overlayaudio.la
overlayaudio_la_LDFLAGS = -avoid-version -module -shared
overlayaudio_la_LIBADD =
overlayaudio_la_SOURCES = overlayaudio.C
AM_CXXFLAGS = $(CINELERRA_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/include $(FFMPEG_CPPFLAGS)
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

EXTRA_DIST = picon.png
BUILT_SOURCES = picon_png.h

picon_png.h: picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }

clean-local:
	$(RM) picon_png.h
