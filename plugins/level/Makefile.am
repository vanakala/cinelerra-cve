plugin_LTLIBRARIES = level.la
level_la_LDFLAGS = -avoid-version -module -shared 
level_la_LIBADD = 
level_la_SOURCES = leveleffect.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = leveleffect.h picon_png.h 
EXTRA_DIST = picon.png
