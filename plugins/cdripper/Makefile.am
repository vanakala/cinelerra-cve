plugin_LTLIBRARIES = cdripper.la
cdripper_la_LDFLAGS = -avoid-version -module -shared 
cdripper_la_LIBADD = 
cdripper_la_SOURCES = cdripper.C cdripwindow.C 
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = cdripper.h cdripwindow.h 
