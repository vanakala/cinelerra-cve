SUBDIRS = data

plugin_LTLIBRARIES = blondtheme.la
blondtheme_la_LDFLAGS = -avoid-version -module -shared 
blondtheme_la_LIBADD = data/libimagedata.a
blondtheme_la_SOURCES = defaulttheme.C 
AM_CXXFLAGS = $(CINELERRA_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra \
	-I$(top_srcdir)/include $(FFMPEG_CPPFLAGS)
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = defaulttheme.h 

EXTRA_DIST = \
	buttons.xcf.bz2 \
	cwindow.xcf.bz2 \
	cwindow2.xcf.bz2 \
	hscroll.xcf.bz2 \
	hscroll2.xcf.bz2 \
	mwindow.xcf.bz2 \
	mwindow2.xcf.bz2 \
	preferencesbutton.xcf.bz2 \
	recordgui.xcf.bz2 \
	recordmonitor.xcf.bz2 \
	scrollbar.xcf.bz2 \
	slider_bg.xcf.bz2 \
	slider_button.xcf.bz2 \
	statusbar_cancel.xcf.bz2 \
	tumbler.xcf.bz2 \
	vscrollbar.xcf.bz2 \
	vwindow.xcf.bz2
