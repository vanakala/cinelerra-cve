plugin_LTLIBRARIES = ivtc.la
ivtc_la_LDFLAGS = -avoid-version -module -shared 
ivtc_la_LIBADD = 
ivtc_la_SOURCES = ivtc.C ivtcwindow.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = ivtc.h ivtcwindow.h picon_png.h 
EXTRA_DIST = picon.png
