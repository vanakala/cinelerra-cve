plugin_LTLIBRARIES = graphic.la
graphic_la_LDFLAGS = -avoid-version -module -shared
graphic_la_LIBADD =
graphic_la_SOURCES = graphic.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)
