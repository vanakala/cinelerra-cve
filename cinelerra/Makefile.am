BUILDDATE=$(shell date)

if BUILDINFO_RECOMPILE
BUILDINFO_NONRC_S=

main.o: versioninfo
aboutprefs.o: versioninfo

versioninfo:
else
BUILDINFO_NONRC_S=[unsure]

main.o: versioninfo.h
aboutprefs.o: versioninfo.h

versioninfo.h:
endif
	echo >versioninfo.h
	echo '#define CINELERRA_VERSION "$(VERSION)"' >> versioninfo.h
	echo >> versioninfo.h
	echo '#define BUILDDATE "$(BUILDDATE)"' >> versioninfo.h

if BUILDINFO_GIT
GITVERSION=$(shell if [ -d $(top_srcdir)/.git ]; then cd $(top_srcdir); git rev-parse HEAD; else echo "unknown"; fi)
	echo '#define REPOMAINTXT "GIT::$(GITVERSION) $(BUILDINFO_NONRC_S)"' >> versioninfo.h

else

if BUILDINFO_CUST

	echo '#define REPOMAINTXT      "$(BUILDINFO_CUSTOM_TAG)"' >> versioninfo.h

endif
endif

	echo '#define COPYRIGHTTEXT1 "(C) 2006 Heroine Virtual Ltd."' >> versioninfo.h
	echo '#define COPYRIGHTTEXT2 "(C) 2006-2015 The CinelerraCV Community"' >> versioninfo.h
	echo '#define COMPILEDATE "Compiled on $(BUILDDATE)"' >> versioninfo.h
	echo '#define FFMPEG_EXTERNALTEXT "@FFMPEG_EXTERNALTEXT@"' >> versioninfo.h

SUBDIRS = data

bin_PROGRAMS = cinelerracve

cinelerracve_SOURCES = aattachmentpoint.C \
		    aautomation.C \
		    aboutprefs.C \
		    adeviceprefs.C \
		    aedit.C \
		    aedits.C \
		    aframe.C \
		    amodule.C \
		    apatchgui.C \
		    apluginarray.C \
		    aplugin.C \
		    apluginset.C \
		    arender.C \
		    asset.C \
		    assetedit.C \
		    assetpopup.C \
		    assets.C \
		    atrack.C \
		    attachmentpoint.C \
		    audioalsa.C \
		    audiodevice.C \
		    audioesound.C \
		    audioodevice.C \
		    audiooss.C \
		    auto.C \
		    autoconf.C \
		    automation.C \
		    autos.C \
		    awindow.C \
		    awindowgui.C \
		    awindowmenu.C \
		    batchrender.C \
		    brender.C \
		    browsebutton.C \
		    cache.C \
		    cachebase.C \
		    canvas.C \
		    canvastools.C \
		    clipedit.C \
		    commonrender.C \
		    confirmsave.C \
		    cpanel.C \
		    cplayback.C \
		    ctimebar.C \
		    ctracking.C \
		    cwindow.C \
		    cwindowgui.C \
		    cwindowtool.C \
		    deleteallindexes.C \
		    drivesync.C \
		    edit.C \
		    editpanel.C \
		    editpopup.C \
		    edits.C \
		    edl.C \
		    edlsession.C \
		    exportedl.C \
		    fadeengine.C \
		    fileac3.C \
		    filebase.C \
		    file.C \
		    filedv.C \
		    fileogg.C \
		    fileexr.C \
		    fileformat.C \
		    filejpeg.C \
		    filelist.C \
		    filemov.C \
		    filempeg.C \
		    filepng.C \
		    filesndfile.C \
		    filetga.C \
		    filethread.C \
		    filetiff.C \
		    filetoc.C \
		    filexml.C \
		    fileyuv.C \
		    floatauto.C \
		    floatautos.C \
		    formatcheck.C \
		    formatpresets.C \
		    formattools.C \
		    framecache.C \
		    garbage.C \
		    gwindow.C \
		    gwindowgui.C \
		    indexfile.C \
		    indexthread.C \
		    intauto.C \
		    intautos.C \
		    interfaceprefs.C \
		    keyframe.C \
		    keyframepopup.C \
		    keyframes.C \
		    labeledit.C \
		    labels.C \
		    levelhist.C \
		    levelwindow.C \
		    levelwindowgui.C \
		    loadbalance.C \
		    loadfile.C \
		    loadmode.C \
		    localsession.C \
		    main.C \
		    mainclock.C \
		    maincursor.C \
		    mainerror.C \
		    mainindexes.C \
		    mainmenu.C \
		    mainprogress.C \
		    mainsession.C \
		    mainundo.C \
		    manualgoto.C \
		    maplist.C \
		    maskauto.C \
		    maskautos.C \
		    maskengine.C \
		    mbuttons.C \
		    menuaeffects.C \
		    menueffects.C \
		    menuveffects.C \
		    meterpanel.C \
		    miscprefs.C \
		    module.C \
		    mtimebar.C \
		    mwindow.C \
		    mwindowedit.C \
		    mwindowgui.C \
		    mwindowmove.C \
		    new.C \
		    overlayframe.C \
		    packagedispatcher.C \
		    packagerenderer.C \
		    packagingengine.C \
		    panauto.C \
		    panautos.C \
		    patchbay.C \
		    patchgui.C \
		    performanceprefs.C \
		    pipe.C \
		    playabletracks.C \
		    playback3d.C \
		    playbackconfig.C \
		    playbackengine.C \
		    playbackprefs.C \
		    playtransport.C \
		    pluginaclient.C \
		    pluginaclientlad.C \
		    pluginarray.C \
		    plugin.C \
		    pluginclient.C \
		    plugindialog.C \
		    pluginpopup.C \
		    pluginserver.C \
		    pluginset.C \
		    plugintclient.C \
		    plugintoggles.C \
		    pluginvclient.C \
		    preferences.C \
		    preferencesthread.C \
		    question.C \
		    quit.C \
		    recordableatracks.C \
		    recordablevtracks.C \
		    removethread.C \
		    render.C \
		    renderengine.C \
		    renderfarm.C \
		    renderfarmclient.C \
		    renderprofiles.C \
		    resample.C \
		    resizetrackthread.C \
		    resourcepixmap.C \
		    resourcethread.C \
		    ruler.C \
		    samplescroll.C \
		    savefile.C \
		    selection.C \
		    setformat.C \
		    sharedlocation.C \
		    sighandler.C \
		    splashgui.C \
		    statusbar.C \
		    theme.C \
		    timebar.C \
		    tipwindow.C \
		    track.C \
		    trackcanvas.C \
		    tracking.C \
		    tracks.C \
		    trackscroll.C \
		    tracksedit.C \
		    transition.C \
		    transitionpopup.C \
		    transportcommand.C \
		    undostackitem.C \
		    vattachmentpoint.C \
		    vautomation.C \
		    vdevicebase.C \
		    vdeviceprefs.C \
		    vdevicex11.C \
		    vedit.C \
		    vedits.C \
		    videodevice.C \
		    viewmenu.C \
		    virtualaconsole.C \
		    virtualanode.C \
		    virtualconsole.C \
		    virtualnode.C \
		    virtualvconsole.C \
		    virtualvnode.C \
		    vmodule.C \
		    vpatchgui.C \
		    vplayback.C \
		    vpluginarray.C \
		    vplugin.C \
		    vpluginset.C \
		    vrender.C \
		    vtimebar.C \
		    vtrack.C \
		    vtracking.C \
		    vwindow.C \
		    vwindowgui.C \
		    wavecache.C \
		    yuvstream.C \
		    zoombar.C \
		    zoompanel.C

# guicast must come first, to pick up correct workarounds.h
AM_CPPFLAGS = \
	-I$(top_srcdir)/guicast \
	-I$(top_srcdir)/quicktime \
	-I$(top_srcdir)/libmpeg3 \
	-I$(top_srcdir)/include

AM_CXXFLAGS = \
	$(LARGEFILE_CFLAGS) \
	$(SOUND_CFLAGS) \
	$(XIPH_CFLAGS) \
	$(MJPEG_CFLAGS) \
	$(OPENEXR_CFLAGS) \
	$(LIBDV_CFLAGS) \
	$(FFMPEG_CFLAGS) \
	-DPLUGIN_DIR=\"$(plugindir)\" \
	-DLOCALE_DIR=\"$(localedir)\"

AM_LDFLAGS = -export-dynamic

noinst_HEADERS = aattachmentpoint.h \
		 aautomation.h \
		 aboutprefs.h \
		 adeviceprefs.h \
		 aedit.h \
		 aedits.h \
		 aframe.h \
		 amodule.h \
		 apatchgui.h \
		 apluginarray.h \
		 aplugin.h \
		 apluginset.h \
		 arender.h \
		 asset.h \
		 assetedit.h \
		 assetpopup.h \
		 assets.h \
		 atrack.h \
		 attachmentpoint.h \
		 audioalsa.h \
		 audiodevice.h \
		 audioesound.h \
		 audiooss.h \
		 autoconf.h \
		 auto.h \
		 automation.h \
		 autos.h \
		 awindowgui.h \
		 awindow.h \
		 awindowmenu.h \
		 batchrender.h \
		 brender.h \
		 browsebutton.h \
		 byteorder.h \
		 cache.h \
		 cachebase.h \
		 canvas.h \
		 canvastools.h \
		 clipedit.h \
		 commonrender.h \
		 confirmsave.h \
		 cpanel.h \
		 cplayback.h \
		 ctimebar.h \
		 ctracking.h \
		 cwindowgui.h \
		 cwindow.h \
		 cwindowtool.h \
		 deleteallindexes.h \
		 drivesync.h \
		 dv1394.h \
		 edit.h \
		 editpanel.h \
		 editpopup.h \
		 edits.h \
		 edl.h \
		 edlsession.h \
		 exportedl.h \
		 fadeengine.h \
		 feather.h \
		 fileac3.h \
		 filebase.h \
		 fileformat.h \
		 file.h \
		 filedv.h \
		 fileogg.h \
		 fileexr.h \
		 filejpeg.h \
		 filelist.h \
		 filemov.h \
		 filempeg.h \
		 filepng.h \
		 filesndfile.h \
		 filetga.h \
		 filethread.h \
		 filetiff.h \
		 filetoc.h \
		 filexml.h \
		 fileyuv.h \
		 floatauto.h \
		 floatautos.h \
		 formatcheck.h \
		 formatpresets.h \
		 formattools.h \
		 framecache.h \
		 garbage.h \
		 gwindow.h \
		 gwindowgui.h \
		 headers.h \
		 indexfile.h \
		 indexthread.h \
		 intauto.h \
		 intautos.h \
		 interfaceprefs.h \
		 keyframe.h \
		 keyframepopup.h \
		 keyframes.h \
		 labeledit.h \
		 labels.h \
		 levelhist.h \
		 levelwindowgui.h \
		 levelwindow.h \
		 loadbalance.h \
		 loadfile.h \
		 loadmode.h \
		 localsession.h \
		 mainclock.h \
		 maincursor.h \
		 mainerror.h \
		 mainindexes.h \
		 mainmenu.h \
		 mainprogress.h \
		 mainsession.h \
		 mainundo.h \
		 manualgoto.h \
		 maplist.h \
		 maskauto.h \
		 maskautos.h \
		 maskengine.h \
		 mbuttons.h \
		 menuaeffects.h \
		 menueffects.h \
		 menuveffects.h \
		 meterpanel.h \
		 miscprefs.h \
		 module.h \
		 mtimebar.h \
		 mwindowgui.h \
		 mwindow.h \
		 new.h \
		 ntsczones.h \
		 overlayframe.h \
		 packagedispatcher.h \
		 packagerenderer.h \
		 packagingengine.h \
		 panauto.h \
		 panautos.h \
		 patchbay.h \
		 patchgui.h \
		 performanceprefs.h \
		 pipe.h \
		 playabletracks.h \
		 playback3d.h \
		 playbackconfig.h \
		 playbackengine.h \
		 playbackprefs.h \
		 playtransport.h \
		 pluginaclient.h \
		 pluginaclientlad.h \
		 pluginarray.h \
		 pluginclient.h \
		 plugindialog.h \
		 pluginmacros.h \
		 plugin.h \
		 pluginpopup.h \
		 pluginserver.h \
		 pluginset.h \
		 plugintclient.h \
		 plugintoggles.h \
		 pluginvclient.h \
		 pluginwindow.h \
		 preferences.h \
		 preferencesthread.h \
		 question.h \
		 quit.h \
		 recordableatracks.h \
		 recordablevtracks.h \
		 removethread.h \
		 renderengine.h \
		 renderfarmclient.h \
		 renderfarm.h \
		 render.h \
		 resample.h \
		 renderprofiles.h \
		 resizetrackthread.h \
		 resourcepixmap.h \
		 resourcethread.h \
		 ruler.h \
		 samplescroll.h \
		 savefile.h \
		 selection.h \
		 setformat.h \
		 sharedlocation.h \
		 sighandler.h \
		 splashgui.h \
		 statusbar.h \
		 theme.h \
		 timebar.h \
		 tipwindow.h \
		 trackcanvas.h \
		 track.h \
		 tracking.h \
		 trackscroll.h \
		 tracks.h \
		 transition.h \
		 transitionpopup.h \
		 transportcommand.h \
		 undostackitem.h \
		 vattachmentpoint.h \
		 vautomation.h \
		 vdevicebase.h \
		 vdeviceprefs.h \
		 vdevicex11.h \
		 vedit.h \
		 vedits.h \
		 videodevice.h \
		 viewmenu.h \
		 virtualaconsole.h \
		 virtualanode.h \
		 virtualconsole.h \
		 virtualnode.h \
		 virtualvconsole.h \
		 virtualvnode.h \
		 vmodule.h \
		 vpatchgui.h \
		 vplayback.h \
		 vpluginarray.h \
		 vplugin.h \
		 vpluginset.h \
		 vrender.h \
		 vtimebar.h \
		 vtrack.h \
		 vtracking.h \
		 vwindowgui.h \
		 vwindow.h \
		 wavecache.h \
		 yuvstream.h \
		 zoombar.h \
		 zoompanel.h \
		$(srcdir)/*.inc

cinelerracve_LDADD = \
	data/libimagedata.a \
	$(top_builddir)/mpeg2enc/libmpeg2enc.la \
	$(top_builddir)/toolame-02l/libtoolame.la \
	$(top_builddir)/quicktime/libquicktimecve.la \
	$(top_builddir)/guicast/libguicastcve.la \
	$(top_builddir)/libmpeg3/libmpeg3cve.la \
	@FFMPEG_LIBS@ \
	$(XIPH_LIBS) \
	$(A52DEC_LIBS) \
	$(OPENEXR_LIBS) \
	-lsndfile \
	-lpng -ljpeg -ltiff -lz \
	$(SOUND_LDFLAGS) \
	$(LIBDV_LIBS) \
	$(OPENGL_LIBS) \
	$(MP3LAME_LIBS) \
	$(X_PRE_LIBS) $(X_LIBS) \
	-luuid \
	$(MJPEG_LIBS)

EXTRA_DIST = gen-feather-h
