plugin_LTLIBRARIES = sharpen.la
sharpen_la_LDFLAGS = -avoid-version -module -shared 
sharpen_la_LIBADD = 
sharpen_la_SOURCES = sharpen.C sharpenwindow.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = picon_png.h sharpen.h sharpenwindow.h 
EXTRA_DIST = picon.png
