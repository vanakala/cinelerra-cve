noinst_LTLIBRARIES=libfourier.la
libfourier_la_SOURCES=fourier.C
libfourier_la_LDFLAGS=
libfourier_la_LIBADD=-lfftw3
AM_CXXFLAGS = $(CINELERRA_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/include $(FFMPEG_CPPFLAGS)
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = fourier.h 
