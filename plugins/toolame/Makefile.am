plugin_LTLIBRARIES = toolame.la
toolame_la_CFLAGS = -DPURGE -DUNIX -DNDEBUG -DINLINE=inline
toolame_la_LDFLAGS = -module -shared -avoid-version
toolame_la_LIBADD =
toolame_la_SOURCES = common.c encode.c ieeefloat.c musicin.c portableio.c psy.c subs.c tonal.c
AM_CFLAGS =
AM_CXXFLAGS =

INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)
