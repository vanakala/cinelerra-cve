plugin_LTLIBRARIES = denoiseseltempavg.la
denoiseseltempavg_la_LDFLAGS = -avoid-version -module -shared 
denoiseseltempavg_la_LIBADD = 
denoiseseltempavg_la_SOURCES = seltempavg.C seltempavgwindow.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h seltempavg.h seltempavgwindow.h 
EXTRA_DIST = picon.png
