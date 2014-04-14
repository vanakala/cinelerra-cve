SUBDIRS = data

plugin_LTLIBRARIES = bluedottheme.la
bluedottheme_la_LDFLAGS = -avoid-version -module -shared 
bluedottheme_la_LIBADD = data/libimagedata.a
bluedottheme_la_SOURCES = bluedottheme.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = bluedottheme.h 

EXTRA_DIST = 
