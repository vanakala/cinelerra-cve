plugin_LTLIBRARIES = deinterlace.la
deinterlace_la_LDFLAGS = -avoid-version -module -shared 
deinterlace_la_LIBADD = 
deinterlace_la_SOURCES = deinterlace.C deinterwindow.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = deinterlace.h deinterwindow.h picon_png.h 
EXTRA_DIST = picon.png
