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
	bcpixmap.C \
	bcpopup.C \
	bcpopupmenu.C \
	bcpot.C \
	bcprogress.C \
	bcprogressbox.C \
	bcrecentlist.C \
	bcrepeater.C \
	bcresources.C \
	bcscrollbar.C \
	bcsignals.C \
	bcslider.C  \
	bcsubwindow.C \
	bctextbox.C \
	bctheme.C \
	bctimer.C \
	bctitle.C \
	bctoggle.C \
	bctumble.C \
	bcwindow.C \
	bcwindowGL.C \
	bcwindowbase.C \
	bcwindowdbg.C \
	bcwindowdraw.C \
	condition.C \
	colormodels.C \
	colorspaces.C \
	filesystem.C \
	glguides.C \
	glthread.C \
	hashcache.C \
	mutex.C \
	rotateframe.C \
	sema.C \
	shaders.C \
	thread.C \
	tmpframecache.C \
	units.C \
	vframe.C

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
	bcpixmap.h \
	bcpixmap.inc \
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
	bcwindowbase.h \
	bcwindowbase.inc \
	bcwindow.h \
	bcwindow.inc \
	clip.h \
	colors.h \
	colormodels.h \
	colormodels.inc \
	colormodels_private.h \
	colorspaces.h \
	colorspaces.inc \
	condition.inc \
	cursors.h \
	filesystem.h \
	filesystem.inc \
	glthread.h \
	glthread.inc \
	glguides.h \
	glguides.inc \
	fonts.h \
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
	shaders.h \
	shaders.inc \
	thread.h \
	tmpframecache.h \
	units.h \
	vframe.h \
	vframe.inc \
	condition.h

AM_CPPFLAGS = -I$(top_srcdir)/include $(FFMPEG_CPPFLAGS)
AM_CXXFLAGS = $(CINELERRA_CFLAGS) $(X_CFLAGS)
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
