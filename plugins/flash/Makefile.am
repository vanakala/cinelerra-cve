plugin_LTLIBRARIES = flash.la
flash_la_LDFLAGS = -avoid-version -module -shared 
flash_la_LIBADD = 
flash_la_SOURCES = flash.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = flash.h picon_png.h 
EXTRA_DIST = picon.png
