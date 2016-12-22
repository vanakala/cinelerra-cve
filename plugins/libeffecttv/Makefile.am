noinst_LTLIBRARIES = libeffecttv.la
libeffecttv_la_LDFLAGS = 
libeffecttv_la_LIBADD = $(top_builddir)/plugins/colors/libcolors.la
libeffecttv_la_SOURCES = effecttv.C
AM_CXXFLAGS = $(CINELERRA_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/plugins/colors -I$(top_srcdir)/include $(FFMPEG_CPPFLAGS)
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = effecttv.h effecttv.inc
