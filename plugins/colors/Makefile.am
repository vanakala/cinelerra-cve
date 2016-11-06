noinst_LTLIBRARIES = libcolors.la
libcolors_la_LDFLAGS = 
libcolors_la_LIBADD = 
libcolors_la_SOURCES = plugincolors.C colorpicker.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = colorpicker.h colorpicker.inc plugincolors.h plugincolors.inc
