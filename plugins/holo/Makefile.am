plugin_LTLIBRARIES = holo.la
holo_la_LDFLAGS = -avoid-version -module -shared 
holo_la_LIBADD = $(top_srcdir)/plugins/libeffecttv/libeffecttv.la
holo_la_SOURCES = holo.C holowindow.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/libeffecttv
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)
