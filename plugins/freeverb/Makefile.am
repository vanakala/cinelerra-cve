plugin_LTLIBRARIES = freeverb.la
freeverb_la_LDFLAGS = -avoid-version -module -shared 
freeverb_la_LIBADD = 
freeverb_la_SOURCES = Components/allpass.cpp Components/comb.cpp \
		      Components/revmodel.cpp freeverb.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(srcdir)/Components
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h \
	Components/allpass.hpp \
	Components/comb.hpp \
	Components/denormals.h \
	Components/revmodel.hpp \
	Components/tuning.h
	 
EXTRA_DIST = picon.png
