plugin_LTLIBRARIES = seltempavg.la
seltempavg_la_LDFLAGS = -avoid-version -module -shared 
seltempavg_la_LIBADD = 
seltempavg_la_SOURCES = seltempavg.C seltempavgwindow.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h seltempavg.h seltempavgwindow.h 
EXTRA_DIST = picon.png
