plugin_LTLIBRARIES = denoisefft.la
denoisefft_la_LDFLAGS = -avoid-version -module -shared 
denoisefft_la_LIBADD = $(top_builddir)/plugins/libfourier/libfourier.la
denoisefft_la_SOURCES = denoisefft.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/libfourier
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h 
EXTRA_DIST = picon.png
