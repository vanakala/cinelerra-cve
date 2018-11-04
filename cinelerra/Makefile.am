BUILDDATE=$(shell date)
if NEED_OPENEXR
    EXR_FILE_NAME=fileexr.C
endif

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
	echo '#define COPYRIGHTTEXT2 "(C) 2006-2018 The CinelerraCV Community"' >> versioninfo.h
	echo '#define COMPILEDATE "Compiled on $(BUILDDATE)"' >> versioninfo.h
	echo '#define FFMPEG_EXTERNALTEXT "@FFMPEG_EXTERNALTEXT@"' >> versioninfo.h

SUBDIRS = data

bin_PROGRAMS = cinelerracve

cinelerracve_SOURCES = aattachmentpoint.C \
		    aautomation.C \
		    aboutprefs.C \
		    adeviceprefs.C \
		    aframe.C \
		    affine.C \
		    amodule.C \
		    apatchgui.C \
		    apluginarray.C \
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
		    avlibsconfig.C \
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
		    colorpicker.C \
		    confirmsave.C \
		    cpanel.C \
		    cplayback.C \
		    ctimebar.C \
		    ctracking.C \
		    cwindow.C \
		    cwindowgui.C \
		    cwindowtool.C \
		    deleteallindexes.C \
		    edit.C \
		    editpanel.C \
		    editpopup.C \
		    edits.C \
		    edl.C \
		    edlsession.C \
		    exportedl.C \
		    fadeengine.C \
		    fileavlibs.C \
		    filebase.C \
		    file.C \
		    $(EXR_FILE_NAME) \
		    fileformat.C \
		    filejpeg.C \
		    filelist.C \
		    filepng.C \
		    filesndfile.C \
		    filetga.C \
		    filethread.C \
		    filetiff.C \
		    filetoc.C \
		    filexml.C \
		    floatauto.C \
		    floatautos.C \
		    formatpresets.C \
		    formattools.C \
		    framecache.C \
		    garbage.C \
		    guidelines.C \
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
		    paramlist.C \
		    paramlistwindow.C \
		    patchbay.C \
		    patchgui.C \
		    performanceprefs.C \
		    pipe.C \
		    pipeconfig.C \
		    playabletracks.C \
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
		    pluginmsgs.C \
		    pluginpopup.C \
		    pluginserver.C \
		    pluginset.C \
		    plugintclient.C \
		    plugintoggles.C \
		    pluginvclient.C \
		    pluginwindow.C \
		    preferences.C \
		    preferencesthread.C \
		    question.C \
		    quit.C \
		    recordableatracks.C \
		    recordablevtracks.C \
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
		    subselection.C \
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
		    vrender.C \
		    vtimebar.C \
		    vtrack.C \
		    vtracking.C \
		    vwindow.C \
		    vwindowgui.C \
		    wavecache.C \
		    zoombar.C \
		    zoompanel.C

# guicast must come first, to pick up correct workarounds.h
AM_CPPFLAGS = \
	-I$(top_srcdir)/guicast \
	-I$(top_srcdir)/include $(FFMPEG_CPPFLAGS)

AM_CXXFLAGS = \
	$(CINELERRA_CFLAGS) \
	$(SOUND_CFLAGS) \
	$(XIPH_CFLAGS) \
	$(MJPEG_CFLAGS) \
	$(OPENEXR_CFLAGS) \
	$(LIBDV_CFLAGS) \
	-DPLUGIN_DIR=\"$(plugindir)\" \
	-DLOCALE_DIR=\"$(localedir)\"

AM_LDFLAGS = -export-dynamic

noinst_HEADERS = aattachmentpoint.h \
		 aautomation.h \
		 aboutprefs.h \
		 adeviceprefs.h \
		 aframe.h \
		 affine.h \
		 amodule.h \
		 apatchgui.h \
		 apluginarray.h \
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
		 avlibsconfig.h \
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
		 colorpicker.h \
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
		 fileavlibs.h \
		 filebase.h \
		 fileformat.h \
		 file.h \
		 fileexr.h \
		 filejpeg.h \
		 filelist.h \
		 filepng.h \
		 filesndfile.h \
		 filetga.h \
		 filethread.h \
		 filetiff.h \
		 filetoc.h \
		 filexml.h \
		 floatauto.h \
		 floatautos.h \
		 formatpresets.h \
		 formattools.h \
		 framecache.h \
		 garbage.h \
		 guidelines.h \
		 gwindow.h \
		 gwindowgui.h \
		 headers.h \
		 indexfile.h \
		 indexthread.h \
		 intauto.h \
		 intautos.h \
		 interfaceprefs.h \
		 interlacemodes.h \
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
		 paramlist.h \
		 paramlistwindow.h \
		 patchbay.h \
		 patchgui.h \
		 performanceprefs.h \
		 pipe.h \
		 pipeconfig.h \
		 playabletracks.h \
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
		 pluginmsgs.h \
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
		 subselection.h \
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
		 vrender.h \
		 vtimebar.h \
		 vtrack.h \
		 vtracking.h \
		 vwindowgui.h \
		 vwindow.h \
		 wavecache.h \
		 zoombar.h \
		 zoompanel.h \
		$(srcdir)/*.inc

cinelerracve_LDADD = \
	data/libimagedata.a \
	$(top_builddir)/guicast/libguicastcve.la \
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
