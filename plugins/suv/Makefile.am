SUBDIRS = data

plugin_LTLIBRARIES = suv.la
suv_la_LDFLAGS = -avoid-version -module -shared 
suv_la_LIBADD = data/libimagedata.a
suv_la_SOURCES = suv.C 
AM_CXXFLAGS = $(CINELERRA_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/include $(FFMPEG_CPPFLAGS)
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = suv.h 
