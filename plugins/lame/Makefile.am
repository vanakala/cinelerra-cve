plugin_LTLIBRARIES = lame.la
lame_la_CFLAGS = -DCHAR_BIT=8
lame_la_LDFLAGS = -avoid-version -module -shared 
lame_la_LIBADD = -lmp3lame
lame_la_SOURCES = get_audio.c lametime.c main.c parse.c portableio.c timestatus.c 
AM_CFLAGS = 
AM_CXXFLAGS = 

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I/usr/include/lame

LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = brhist.h get_audio.h lametime.h main.h parse.h portableio.h timestatus.h 
