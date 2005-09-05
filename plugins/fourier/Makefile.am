plugin_LTLIBRARIES=libfourier.la
libfourier_la_SOURCES=fourier.C
libfourier_la_LDFLAGS=-avoid-version -shared
libfourier_la_LDADD=-lfftw3
INCLUDES = -I$(top_srcdir)/guicast
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = fourier.h 
