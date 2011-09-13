bin_PROGRAMS=mplexlocve

INCLUDES=-I$(top_srcdir)/libmpeg3
AM_CFLAGS = $(LARGEFILE_CFLAGS)
mplexlocve_LDADD=../libmpeg3/libmpeg3cve.la
mplexlocve_LDFLAGS=-lm -lpthread
mplexlocve_SOURCES=mplex.c
