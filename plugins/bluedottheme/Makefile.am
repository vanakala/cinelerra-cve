SUBDIRS = data

plugin_LTLIBRARIES = bluedottheme.la
bluedottheme_la_LDFLAGS = -avoid-version -module -shared 
bluedottheme_la_LIBADD = data/libimagedata.a
bluedottheme_la_SOURCES = bluedottheme.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = bluedottheme.h 

EXTRA_DIST = 
