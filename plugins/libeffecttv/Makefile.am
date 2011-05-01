noinst_LTLIBRARIES = libeffecttv.la
libeffecttv_la_LDFLAGS = 
libeffecttv_la_LIBADD = $(top_builddir)/plugins/colors/libcolors.la
libeffecttv_la_SOURCES = effecttv.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/colors -I$(top_srcdir)/include
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = effecttv.h effecttv.inc
