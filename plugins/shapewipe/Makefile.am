plugin_LTLIBRARIES = shapewipe.la
shapewipe_la_LDFLAGS = -avoid-version -module -shared 
shapewipe_la_LIBADD = 
shapewipe_la_SOURCES = shapewipe.C 
AM_CFLAGS = 
AM_CXXFLAGS = -DDEFAULT_SHAPE=\"$(libdir)/cinelerra/wipeshapes/heart.png\"

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

shapewipedir = $(plugindir)/shapewipe
shapewipe_DATA = heart.png

noinst_HEADERS = shapewipe.h picon_png.h 
EXTRA_DIST = picon.png
