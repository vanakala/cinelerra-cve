noinst_LTLIBRARIES=libfourier.la
libfourier_la_SOURCES=fourier.C
libfourier_la_LDFLAGS=
libfourier_la_LIBADD=-lfftw3
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)
INCLUDES = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = fourier.h 
