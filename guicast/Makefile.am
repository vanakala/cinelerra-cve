lib_LTLIBRARIES = libguicast.la
noinst_PROGRAMS=bootstrap

libguicast_la_LIBADD = -lXxf86vm -lXv -lXext -lX11 $(X_EXTRA_LIBS)
libguicast_la_LDFLAGS = $(X_LIBS) -version-info 1:0:0 
libguicast_la_SOURCES = \
	bcbar.C \
	bcbitmap.C \
	bcbutton.C \
	bccapture.C \
	bcclipboard.C \
	bcdelete.C \
	bcdialog.C \
	bcdisplayinfo.C \
	bcdragwindow.C \
	bcfilebox.C \
	bchash.C \
	bcipc.C \
	bclistbox.C \
	bclistboxitem.C \
	bcmenu.C \
	bcmenubar.C \
	bcmenuitem.C \
	bcmenupopup.C \
	bcmeter.C \
	bcnewfolder.C \
	bcpan.C \
	bcpbuffer.C \
	bcpixmap.C \
	bcpixmapsw.C \
	bcpopup.C \
	bcpopupmenu.C \
	bcpot.C \
	bcprogress.C \
	bcprogressbox.C	\
	bcrecentlist.C \
	bcrelocatablewidget.C \
	bcrepeater.C \
	bcresources.C \
	bcscrollbar.C \
	bcsignals.C \
	bcslider.C  \
	bcsubwindow.C \
	bcsynchronous.C \
	bctextbox.C \
	bctexture.C \
	bctheme.C \
	bctimer.C \
	bctitle.C \
	bctoggle.C \
	bctumble.C \
	bcwidgetgrid.C \
	bcwindow.C \
	bcwindow3d.C \
	bcwindowbase.C \
	bcwindowdraw.C \
	bcwindowevents.C \
	condition.C \
	debug.h \
        error.h \
	errorbox.C \
	filesystem.C \
	mutex.C \
	properties.C \
	rotateframe.C \
	sema.C \
	stringfile.C \
	thread.C \
	units.C \
	vframe.C \
	vframe3d.C \
	workarounds.C

noinst_HEADERS = \
	arraylist.h \
	bcbar.h \
	bcbar.inc \
	bcbitmap.h \
	bcbitmap.inc \
	bcbutton.h \
	bcbutton.inc \
	bccapture.h \
	bccapture.inc \
	bcclipboard.h \
	bcclipboard.inc \
	bcdelete.h \
	bcdelete.inc \
	bcdialog.h \
	bcdialog.inc \
	bcdisplayinfo.h \
	bcdisplayinfo.inc \
	bcdragwindow.h \
	bcdragwindow.inc \
	bcfilebox.h \
	bcfilebox.inc \
	bchash.h \
	bchash.inc \
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
	bcnewfolder.h \
	bcnewfolder.inc \
	bcpan.h bcpan.inc \
	bcpbuffer.h \
	bcpbuffer.inc \
	bcpixmap.h \
	bcpixmap.inc \
	bcpixmapsw.h \
	bcpixmapsw.inc \
	bcpopup.h bcpopup.inc \
	bcpopupmenu.h \
	bcpopupmenu.inc \
	bcpot.h \
	bcpot.inc \
	bcprogressbox.h \
	bcprogressbox.inc \
	bcprogress.h \
	bcprogress.inc \
	bcrecentlist.h \
	bcrelocatablewidget.h \
	bcrelocatablewidget.inc \
	bcrepeater.h \
	bcrepeater.inc \
	bcresources.h \
	bcresources.inc \
	bcscrollbar.h \
	bcscrollbar.inc \
	bcsignals.h \
	bcsignals.inc \
	bcslider.h \
	bcslider.inc \
	bcsubwindow.h \
	bcsubwindow.inc \
	bcsynchronous.h \
	bcsynchronous.inc \
	bctextbox.h \
	bctextbox.inc \
	bctextbox.h \
	bctextbox.inc \
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
	bcwidgetgrid.h \
	bcwidgetgrid.inc \
	bcwindowbase.h \
	bcwindowbase.inc \
	bcwindowevents.h \
	bcwindowevents.inc \
	bcwindow.h \
	bcwindow.inc \
	clip.h \
	colors.h \
	condition.inc \
	cursors.h \
	errorbox.h \
	errorbox.inc \
	filesystem.h \
	filesystem.inc \
	fonts.h \
	guicast.h \
	keys.h \
	language.h \
	linklist.h \
	mutex.h \
	mutex.inc \
	properties.h \
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

INCLUDES = -I$(top_srcdir)/quicktime
AM_CXXFLAGS = $(LARGEFILE_CFLAGS) $(X_CFLAGS)
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

# special CFLAGS that can be set on the make command line
BOOTSTRAP_CFLAGS=
bootstrap_SOURCES=bootstrap.c
bootstrap_LDADD =

# custom rule that disregards any fancy flags that the user might have set
bootstrap.$(OBJEXT): bootstrap.c
	$(CC) $(BOOTSTRAP_CFLAGS) -c $<

EXTRA_DIST = images pngtoh.c

dist-hook:
	-rm -rf $(distdir)/images/CVS
	-rm -rf $(distdir)/images/7seg_small/CVS
