plugin_LTLIBRARIES = dot.la
dot_la_LDFLAGS = -avoid-version -module -shared 
dot_la_LIBADD = $(top_srcdir)/plugins/libeffecttv/libeffecttv.la
dot_la_SOURCES = dot.C dotwindow.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/libeffecttv
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)
