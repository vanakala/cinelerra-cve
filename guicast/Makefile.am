lib_LTLIBRARIES = libguicastcve.la
noinst_PROGRAMS = bootstrap pngtoh

libguicastcve_la_LIBADD = $(OPENGL_LIBS) $(X_PRE_LIBS) $(X_LIBS) $(X_EXTRA_LIBS) $(XFT_LIBS) $(FFMPEG_SWSCALE)
libguicastcve_la_LDFLAGS = -version-info 3:0:0 
libguicastcve_la_SOURCES = \
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
	bcfontentry.C \
	bchash.C \
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
	condition.C \
	colormodels.C \
	colormodels_default.C \
	colormodels_float.C \
	colormodels_yuv420p.C \
	colormodels_yuv422.C \
	filesystem.C \
	hashcache.C \
	mutex.C \
	rotateframe.C \
	sema.C \
	thread.C \
	tmpframecache.C \
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
	bcfontentry.h \
	bcfontentry.inc \
	bchash.h \
	bchash.inc \
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
	bctexture.h \
	bctexture.inc \
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
	bcwindow.h \
	bcwindow.inc \
	clip.h \
	colors.h \
	colormodels.h \
	colormodels.inc \
	condition.inc \
	cursors.h \
	filesystem.h \
	filesystem.inc \
	fonts.h \
	guicast.h \
	hashcache.h \
	hashcache.inc \
	keys.h \
	language.h \
	linklist.h \
	mutex.h \
	mutex.inc \
	rotateframe.h \
	rotateframe.inc \
	sema.h \
	sema.inc \
	thread.h \
	tmpframecache.h \
	units.h \
	vframe.h \
	vframe.inc \
	workarounds.h \
	condition.h

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CXXFLAGS = $(LARGEFILE_CFLAGS) $(X_CFLAGS) $(FFMPEG_CFLAGS)
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

# special CFLAGS that can be set on the make command line
BOOTSTRAP_CFLAGS=
bootstrap_SOURCES=bootstrap.c
bootstrap_LDADD =

# custom rule that disregards any fancy flags that the user might have set
bootstrap.$(OBJEXT): bootstrap.c
	$(CC) $(BOOTSTRAP_CFLAGS) -c $<

pngtoh_SOURCES = pngtoh.c

pngtoh$(EXEEXT): pngtoh.c
	$(CC) -o pngtoh$(EXEEXT) $<

EXTRA_DIST = images

dist-hook:
	-rm -rf $(distdir)/images/CVS
	-rm -rf $(distdir)/images/7seg_small/CVS
