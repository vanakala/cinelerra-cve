plugin_LTLIBRARIES = freeverb.la
freeverb_la_LDFLAGS = -avoid-version -module -shared 
freeverb_la_LIBADD = 
freeverb_la_SOURCES = allpass.cpp comb.cpp revmodel.cpp freeverb.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = \
	allpass.hpp \
	comb.hpp \
	denormals.h \
	revmodel.hpp \
	tuning.h

EXTRA_DIST = picon.png
BUILT_SOURCES = picon_png.h

picon_png.h: picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }

clean-local:
	$(RM) picon_png.h
