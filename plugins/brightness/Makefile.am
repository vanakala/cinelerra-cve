plugin_LTLIBRARIES = brightness.la
brightness_la_LDFLAGS = -avoid-version -module -shared 
brightness_la_LIBADD = $(top_srcdir)/plugins/libcolors/libcolors.la
brightness_la_SOURCES = brightness.C brightnesswindow.C 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/libcolors
