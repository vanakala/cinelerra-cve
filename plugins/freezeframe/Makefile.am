plugin_LTLIBRARIES = freezeframe.la
freezeframe_la_LDFLAGS = -avoid-version -module -shared 
freezeframe_la_LIBADD = 
freezeframe_la_SOURCES = freezeframe.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = freezeframe.h picon_png.h 
EXTRA_DIST = picon.png
