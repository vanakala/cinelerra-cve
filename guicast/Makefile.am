lib_LTLIBRARIES = libguicast.la
noinst_PROGRAMS=bootstrap

libguicast_la_LIBADD = -lXxf86vm -lX11 -lXv -lXext
libguicast_la_LDFLAGS = $(X_LIBS) -version-info 1:0:0 
libguicast_la_SOURCES = \
	bcbitmap.C \
	bcbutton.C \
	bccapture.C \
	bcclipboard.C \
	bcdialog.C \
	bcdisplayinfo.C \
	bcdragwindow.C \
	bcfilebox.C \
	bcipc.C \
	bclistbox.C \
	bcmenu.C \
	bcmenubar.C \
	bcmenuitem.C \
	bcmenupopup.C \
	bcmeter.C \
	bcpan.C \
	bcpixmap.C \
	bcpopup.C \
	bcpopupmenu.C \
	bcpot.C \
	bcprogress.C \
												bcprogressbox.C bcrepeater.C bcresources.C bcscrollbar.C bcsignals.C bcslider.C bcsubwindow.C \
	bctextbox.C \
	bctimer.C \
	bctitle.C \
	bctoggle.C \
	bctumble.C \
	bcwindow.C \
	bcwindowbase.C \
	bcwindowdraw.C \
	errorbox.C \
	defaults.C \
	filesystem.C \
	mutex.C \
	rotateframe.C \
	sema.C \
	stringfile.C \
	thread.C \
	units.C \
												vframe.C workarounds.C bclistboxitem.C bctheme.C condition.C
noinst_HEADERS = arraylist.h bcbitmap.h bcbitmap.inc bcbutton.h bcbutton.inc bccapture.h bccapture.inc bcclipboard.h \
	bcclipboard.inc \
	bcdialog.h \
	bcdialog.inc \
	bcdisplayinfo.h \
	bcdisplayinfo.inc \
	bcdragwindow.h \
	bcdragwindow.inc \
	bcfilebox.h \
	bcfilebox.inc \
	bcipc.h \
	bclistbox.h \
	bclistbox.inc \
	bclistboxitem.h \
	bclistboxitem.inc \
	bcmenu.h \
	bcmenu.inc \
	bcmenubar.h \
	bcmenubar.inc \
	bcmenuitem.h \
	bcmenuitem.inc \
	bcmenupopup.h \
	bcmenupopup.inc \
	bcmeter.h \
	bcmeter.inc \
								 bcpan.h bcpan.inc bcpixmap.h bcpixmap.inc bcpopup.h bcpopup.inc \
								 bcpopupmenu.h bcpopupmenu.inc bcpopuplistbox.inc bcpot.h bcpot.inc bcprogressbox.h bcprogressbox.inc bcprogress.h \
								 bcprogress.inc bcrepeater.h bcrepeater.inc bcresources.h bcresources.inc bcscrollbar.h bcscrollbar.inc \
								 bcsignals.h bcsignals.inc bcslider.h bcslider.inc bcsubwindow.h bcsubwindow.inc bctextbox.h bctextbox.inc \
	bctheme.h \
	bctheme.inc \
	bctimer.h \
	bctimer.inc \
	bctitle.h \
	bctitle.inc \
	bctoggle.h \
	bctoggle.inc \
	bctumble.h \
	bctumble.inc \
	bcwindowbase.h \
	bcwindowbase.inc \
								 bcwindow.h bcwindow.inc clip.h colors.h condition.inc cursors.h defaults.h defaults.inc errorbox.h errorbox.inc \
	filesystem.h \
	filesystem.inc \
	fonts.h \
	guicast.h \
	keys.h \
	language.h \
	linklist.h \
	mutex.h \
	mutex.inc \
	rotateframe.h \
	rotateframe.inc \
	sema.h \
	sema.inc \
	sizes.h \
	stringfile.h \
	stringfile.inc \
	testobject.h \
	thread.h \
	units.h \
	vframe.h \
	vframe.inc \
	workarounds.h \
	condition.h

INCLUDES = -I$(top_srcdir)/quicktime -I/usr/include/qt
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

bootstrap_SOURCES=bootstrap.c
bootstrap_LDADD =

EXTRA_DIST = images pngtoh.c

dist-hook:
	-rm -rf $(distdir)/images/CVS
	-rm -rf $(distdir)/images/7seg_small/CVS
