DV_DIR := ../quicktime/libdv-0.99
ALSA_DIR := ../alsa-lib-0.9.0rc2
OGG_DIR := ../quicktime/libogg-1.0
VORBIS_DIR := ../quicktime/libvorbis-1.0
LIBRAW_DIR := ../libraw1394-0.9.0
TIFF_DIR := ../tiff-v3.5.7
UUID_DIR := ../uuid
TOOLAME_DIR := ../toolame-02l
LAME_DIR := ../quicktime/lame-3.93.1/include
SNDFILE_DIR := $(shell expr ../libsndfile* )
include ../global_config

OBJS = \
	$(OBJDIR)/aattachmentpoint.o \
	$(OBJDIR)/aautomation.o \
	$(OBJDIR)/aboutprefs.o \
	$(OBJDIR)/adeviceprefs.o \
	$(OBJDIR)/aedit.o \
	$(OBJDIR)/aedits.o \
	$(OBJDIR)/amodule.o \
	$(OBJDIR)/apatchgui.o \
	$(OBJDIR)/aplugin.o \
	$(OBJDIR)/apluginarray.o \
	$(OBJDIR)/apluginset.o \
	$(OBJDIR)/arender.o \
	$(OBJDIR)/assetedit.o \
	$(OBJDIR)/assetpopup.o \
	$(OBJDIR)/assetremove.o \
	$(OBJDIR)/assets.o \
	$(OBJDIR)/asset.o \
	$(OBJDIR)/atrack.o \
	$(OBJDIR)/attachmentpoint.o \
	$(OBJDIR)/audio1394.o \
	$(OBJDIR)/audioalsa.o \
	$(OBJDIR)/audiodevice.o \
	$(OBJDIR)/audioesound.o \
	$(OBJDIR)/audioidevice.o \
	$(OBJDIR)/audioodevice.o \
	$(OBJDIR)/audiooss.o \
	$(OBJDIR)/auto.o \
	$(OBJDIR)/autos.o \
	$(OBJDIR)/autoconf.o \
	$(OBJDIR)/automation.o \
	$(OBJDIR)/awindow.o \
	$(OBJDIR)/awindowgui.o \
	$(OBJDIR)/awindowmenu.o \
	$(OBJDIR)/batch.o \
	$(OBJDIR)/batchrender.o \
	$(OBJDIR)/bezierauto.o \
	$(OBJDIR)/bezierautos.o \
	$(OBJDIR)/bitspopup.o \
	$(OBJDIR)/brender.o \
	$(OBJDIR)/browsebutton.o \
	$(OBJDIR)/byteorderpopup.o \
	$(OBJDIR)/cache.o \
	$(OBJDIR)/canvas.o \
	$(OBJDIR)/canvastools.o \
	$(OBJDIR)/channel.o \
	$(OBJDIR)/channeledit.o \
	$(OBJDIR)/channelpicker.o \
	$(OBJDIR)/chantables.o \
	$(OBJDIR)/clipedit.o \
	$(OBJDIR)/commonrender.o \
	$(OBJDIR)/confirmsave.o \
	$(OBJDIR)/confirmquit.o \
	$(OBJDIR)/cpanel.o \
	$(OBJDIR)/cplayback.o \
	$(OBJDIR)/ctimebar.o \
	$(OBJDIR)/ctracking.o \
	$(OBJDIR)/cursor.o \
	$(OBJDIR)/cwindow.o \
	$(OBJDIR)/cwindowgui.o \
	$(OBJDIR)/cwindowtool.o \
	$(OBJDIR)/dcoffset.o \
	$(OBJDIR)/device1394input.o \
	$(OBJDIR)/device1394output.o \
	$(OBJDIR)/deleteallindexes.o \
	$(OBJDIR)/drivesync.o \
	$(OBJDIR)/edit.o \
	$(OBJDIR)/edithandles.o \
	$(OBJDIR)/editpanel.o \
	$(OBJDIR)/editpopup.o \
	$(OBJDIR)/edits.o \
	$(OBJDIR)/edl.o \
	$(OBJDIR)/edlsession.o \
	$(OBJDIR)/fadeengine.o \
	$(OBJDIR)/file.o \
	$(OBJDIR)/fileavi.o \
	$(OBJDIR)/filebase.o \
	$(OBJDIR)/filebaseaudio.o \
	$(OBJDIR)/filebaseima4.o \
	$(OBJDIR)/filebaseulaw.o \
	$(OBJDIR)/fileformat.o \
	$(OBJDIR)/filelist.o \
	$(OBJDIR)/filejpeg.o \
	$(OBJDIR)/filemov.o \
	$(OBJDIR)/filempeg.o \
	$(OBJDIR)/filepng.o \
	$(OBJDIR)/filesndfile.o \
	$(OBJDIR)/filetga.o \
	$(OBJDIR)/filethread.o \
	$(OBJDIR)/filetiff.o \
	$(OBJDIR)/filevorbis.o \
	$(OBJDIR)/filexml.o \
	$(OBJDIR)/floatauto.o \
	$(OBJDIR)/floatautos.o \
	$(OBJDIR)/formatcheck.o \
	$(OBJDIR)/formatpopup.o \
	$(OBJDIR)/formattools.o \
	$(OBJDIR)/indexfile.o \
	$(OBJDIR)/indexthread.o \
	$(OBJDIR)/intauto.o \
	$(OBJDIR)/intautos.o \
	$(OBJDIR)/interfaceprefs.o \
	$(OBJDIR)/keyframe.o \
	$(OBJDIR)/keyframes.o \
	$(OBJDIR)/keyframehandles.o \
	$(OBJDIR)/labelnavigate.o \
	$(OBJDIR)/labels.o \
	$(OBJDIR)/levelwindow.o \
	$(OBJDIR)/levelwindowgui.o \
	$(OBJDIR)/loadbalance.o \
	$(OBJDIR)/loadfile.o \
	$(OBJDIR)/loadmode.o \
	$(OBJDIR)/localsession.o \
	$(OBJDIR)/main.o \
	$(OBJDIR)/mainclock.o \
	$(OBJDIR)/maincursor.o \
	$(OBJDIR)/mainindexes.o \
	$(OBJDIR)/mainmenu.o \
	$(OBJDIR)/mainprogress.o \
	$(OBJDIR)/mainsession.o \
	$(OBJDIR)/mainundo.o \
	$(OBJDIR)/maskauto.o \
	$(OBJDIR)/maskautos.o \
	$(OBJDIR)/maskengine.o \
	$(OBJDIR)/mbuttons.o \
	$(OBJDIR)/menuaeffects.o \
	$(OBJDIR)/menueffects.o \
	$(OBJDIR)/menuveffects.o \
	$(OBJDIR)/messages.o \
	$(OBJDIR)/meterpanel.o \
	$(OBJDIR)/module.o \
	$(OBJDIR)/mtimebar.o \
	$(OBJDIR)/mwindow.o \
	$(OBJDIR)/mwindowedit.o \
	$(OBJDIR)/mwindowgui.o \
	$(OBJDIR)/mwindowmove.o \
	$(OBJDIR)/new.o \
	$(OBJDIR)/newfolder.o \
	$(OBJDIR)/overlayframe.o \
	$(OBJDIR)/packagedispatcher.o \
	$(OBJDIR)/packagerenderer.o \
	$(OBJDIR)/panauto.o \
	$(OBJDIR)/panautos.o \
	$(OBJDIR)/patchbay.o \
	$(OBJDIR)/patchgui.o \
	$(OBJDIR)/performanceprefs.o \
	$(OBJDIR)/playabletracks.o \
	$(OBJDIR)/playbackconfig.o \
	$(OBJDIR)/playbackengine.o \
	$(OBJDIR)/playbackprefs.o \
	$(OBJDIR)/playtransport.o \
	$(OBJDIR)/plugin.o \
	$(OBJDIR)/pluginaclient.o \
	$(OBJDIR)/pluginaclientlad.o \
	$(OBJDIR)/pluginarray.o \
	$(OBJDIR)/pluginautos.o \
	$(OBJDIR)/pluginclient.o \
	$(OBJDIR)/plugindialog.o \
	$(OBJDIR)/pluginpopup.o \
	$(OBJDIR)/pluginset.o \
	$(OBJDIR)/pluginserver.o \
	$(OBJDIR)/plugintclient.o \
	$(OBJDIR)/pluginvclient.o \
	$(OBJDIR)/preferences.o \
	$(OBJDIR)/preferencesthread.o \
	$(OBJDIR)/presentations.o \
	$(OBJDIR)/question.o \
	$(OBJDIR)/quit.o \
	$(OBJDIR)/recconfirmdelete.o \
	$(OBJDIR)/record.o \
	$(OBJDIR)/recordableatracks.o \
	$(OBJDIR)/recordablevtracks.o \
	$(OBJDIR)/recordaudio.o \
	$(OBJDIR)/recordconfig.o \
	$(OBJDIR)/recordgui.o \
	$(OBJDIR)/recordlabel.o \
	$(OBJDIR)/recordmonitor.o \
	$(OBJDIR)/recordprefs.o \
	$(OBJDIR)/recordthread.o \
	$(OBJDIR)/recordtransport.o \
	$(OBJDIR)/recordvideo.o \
	$(OBJDIR)/recordwindow.o \
	$(OBJDIR)/render.o \
	$(OBJDIR)/renderfarm.o \
	$(OBJDIR)/renderfarmclient.o \
	$(OBJDIR)/renderfarmfsclient.o \
	$(OBJDIR)/renderfarmfsserver.o \
	$(OBJDIR)/renderengine.o \
	$(OBJDIR)/resample.o \
	$(OBJDIR)/resizetrackthread.o \
	$(OBJDIR)/resourcepixmap.o \
	$(OBJDIR)/samplescroll.o \
	$(OBJDIR)/savefile.o \
	$(OBJDIR)/setformat.o \
	$(OBJDIR)/sharedlocation.o \
	$(OBJDIR)/shmemory.o \
	$(OBJDIR)/sighandler.o \
	$(OBJDIR)/splashgui.o \
	$(OBJDIR)/statusbar.o \
	$(OBJDIR)/theme.o \
	$(OBJDIR)/threadexec.o \
	$(OBJDIR)/threadindexer.o \
	$(OBJDIR)/threadloader.o \
	$(OBJDIR)/threadfork.o \
	$(OBJDIR)/timebar.o \
	$(OBJDIR)/timeentry.o \
	$(OBJDIR)/track.o \
	$(OBJDIR)/trackcanvas.o \
	$(OBJDIR)/tracking.o \
	$(OBJDIR)/tracks.o \
	$(OBJDIR)/trackscroll.o \
	$(OBJDIR)/tracksedit.o \
	$(OBJDIR)/transition.o \
	$(OBJDIR)/transitionhandles.o \
	$(OBJDIR)/transitionpopup.o \
	$(OBJDIR)/transportque.o \
	$(OBJDIR)/undostack.o \
	$(OBJDIR)/vattachmentpoint.o \
	$(OBJDIR)/vautomation.o \
	$(OBJDIR)/vdevice1394.o \
	$(OBJDIR)/vdevicebase.o \
	$(OBJDIR)/vdevicebuz.o \
	$(OBJDIR)/vdeviceprefs.o \
	$(OBJDIR)/vdevicev4l.o \
	$(OBJDIR)/vdevicex11.o \
	$(OBJDIR)/vedit.o \
	$(OBJDIR)/vedits.o \
	$(OBJDIR)/videodevice.o \
	$(OBJDIR)/viewmenu.o \
	$(OBJDIR)/virtualnode.o \
	$(OBJDIR)/virtualaconsole.o \
	$(OBJDIR)/virtualanode.o \
	$(OBJDIR)/virtualconsole.o \
	$(OBJDIR)/virtualvconsole.o \
	$(OBJDIR)/virtualvnode.o \
	$(OBJDIR)/vmodule.o \
	$(OBJDIR)/vpatchgui.o \
	$(OBJDIR)/vplayback.o \
	$(OBJDIR)/vplugin.o \
	$(OBJDIR)/vpluginarray.o \
	$(OBJDIR)/vpluginset.o \
	$(OBJDIR)/vtimebar.o \
	$(OBJDIR)/vrender.o \
	$(OBJDIR)/vtrack.o \
	$(OBJDIR)/vtracking.o \
	$(OBJDIR)/vwindow.o \
	$(OBJDIR)/vwindowgui.o \
	$(OBJDIR)/workarounds.o \
	$(OBJDIR)/zoombar.o \
	$(OBJDIR)/zoompanel.o



OUTPUT = $(OBJDIR)/cinelerra


#ifeq ($(HAVE_AVIFILE), y)
AVIFILE_DIR := $(shell expr ../avifile* )
#endif

ifeq ($(HAVE_ALSA), y)
ALSA_CFLAGS := -DHAVE_ALSA -I$(ALSA_DIR)/include
ALSA_LIBS := $(ALSA_DIR)/$(OBJDIR)/libasound.a
endif

STATICLIBS = \
	../libavc/$(OBJDIR)/libavc.a \
	../quicktime/$(OBJDIR)/libquicktime.a \
        $(ESOUNDLIBS) \
	$(TIFF_DIR)/$(OBJDIR)/libtiff.a \
	../guicast/$(OBJDIR)/libguicast.a \
	../libmpeg3/$(OBJDIR)/libmpeg3.a \
	$(SNDFILE_DIR)/$(OBJDIR)/libsndfile.a \
	$(UUID_DIR)/$(OBJDIR)/libuuid.a \
	../mpeg2enc/$(OBJDIR)/mpeg2enc.a \
	$(TOOLAME_DIR)/$(OBJDIR)/toolame.a

LIBS := \
	-L./ \
	-L../guicast


ifeq ($(OBJDIR), x86_64)
LIBS += -L/usr/X11R6/lib64
else
LIBS += -L/usr/X11R6/lib
endif




LIBS += \
	$(ALSA_LIBS) \
	$(LIBRAW_DIR)/$(OBJDIR)/libraw1394.a \
	$(GLLIBS) \
	-lX11 \
	-lXext \
	-lXv \
	-lpthread \
	-lm \
	$(XXF86VM) \
	-lpng \
	-lz \
	-ldl


ifeq ($(HAVE_XFT), y)
LIBS += -lXft
endif

CFLAGS += \
	-I../guicast \
	-I../quicktime \
	-I$(DV_DIR) \
	-I$(OGG_DIR)/include \
	-I$(VORBIS_DIR)/include \
	-I$(LIBRAW_DIR)/src \
	-I$(TIFF_DIR)/libtiff \
	-I$(UUID_DIR) \
	-I$(DV_DIR) \
	-I$(TOOLAME_DIR) \
	-I$(LAME_DIR) \
	-I../audiofile \
	-I../libmpeg3 \
	-I$(SNDFILE_DIR)/src \
	-I../libavc/common \
	-I../libavc/libavc1394 \
	-I../libavc/librom1394 \
	$(ESOUNDCFLAGS) \
	$(ALSA_CFLAGS)


# Speed up linking with this linking sequence
ifeq ($(OBJDIR), alpha)



LDFLAGS1 = \
	--demangle=compaq -export-dynamic -L./ \
	-L../guicast -L/usr/X11R6/lib \
	-L`expr /usr/lib/compaq/cxx-*/alpha-linux/`lib -L/usr/local/lib \
	-L`expr /usr/lib/gcc-lib/alpha-redhat-linux/*` \
	-rpath `expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/ -m elf64alpha \
	-L`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/ -dynamic-linker \
	/lib/ld-linux.so.2 `expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/crt1.o \
	`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/crti.o \
	`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/crtbegin.o \
	`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/_main.o

LDFLAGS2 = -lcpml -lcxxstdma_rh60 -lcxxma_rh60 -lc -lots \
	`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/crtend.o \
	`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/crtn.o --no-demangle \
	--warn-once
LINKER = ld -o $(OUTPUT)
CFLAGS += -DUSE_ALPHA



else



LDFLAGS1 = -export-dynamic
LDFLAGS2 = 
LINKER = g++ -o $(OUTPUT)


endif




# Enable AVIFile support
ifeq ($(HAVE_AVIFILE), y)

CFLAGS += -DUSE_AVIFILE -I$(AVIFILE_DIR)/include
LIBS += \
	$(AVIFILE_DIR)/libmmxnow/src/.libs/libmmxnow.a \
	$(AVIFILE_DIR)/lib/.libs/libaviplay.a

endif






$(shell echo "\#define BUILDDATE \"`date`\"" > builddate.h )
$(shell echo $(CFLAGS) > $(OBJDIR)/c_flags)
$(shell echo $(LDFLAGS1) $(OBJS) $(AVIOBJS) $(CXXREPOSITORY) $(STATICLIBS) $(LIBS) $(LDFLAGS2) > $(OBJDIR)/objs)





# Static linking is not possible because of the way the plugins are loaded
$(OUTPUT): $(OBJS) $(BOOTSTRAP)
	$(LINKER) `cat $(OBJDIR)/objs`
	strip $(OUTPUT)
	../guicast/$(OBJDIR)/bootstrap $(OUTPUT) data/mode_*.png

$(OBJDIR)/test: 
	$(CC) -shared -o $(OBJDIR)/test.so \
		$(OBJDIR)/threadfork.o \
		../guicast/$(OBJDIR)/thread.o \
		../guicast/$(OBJDIR)/mutex.o
	$(CC) -o $(OBJDIR)/test test.C \
		`cat $(OBJDIR)/c_flags` \
		$(OBJDIR)/test.so \
		$(LIBS)

clean:
	rm -rf $(OBJDIR)
	find \( -name core \
		-o -name '*.o' \
		-o -name '*.a' \
		-o -name '*.so' \) -exec rm -f {} \; -prune

wc:
	cat *.C *.h | wc

install:
	rm -f /usr/bin/cinelerra
	cp $(OUTPUT) /usr/bin


$(LIBOBJS) $(OBJS):
	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.C) -o $*.o

$(OBJDIR)/aattachmentpoint.o: 			  aattachmentpoint.C
$(OBJDIR)/aautomation.o: 			  aautomation.C
$(OBJDIR)/aboutprefs.o:                           aboutprefs.C
$(OBJDIR)/adeviceprefs.o: 			  adeviceprefs.C
$(OBJDIR)/aedit.o: 				  aedit.C
$(OBJDIR)/aedits.o: 				  aedits.C
$(OBJDIR)/amodule.o: 				  amodule.C
$(OBJDIR)/apanel.o: 				  apanel.C
$(OBJDIR)/apatchgui.o:  			  apatchgui.C
$(OBJDIR)/aplugin.o: 				  aplugin.C
$(OBJDIR)/apluginarray.o: 			  apluginarray.C
$(OBJDIR)/apluginset.o: 			  apluginset.C
$(OBJDIR)/arender.o: 				  arender.C
$(OBJDIR)/asset.o:                                asset.C
$(OBJDIR)/assetedit.o:  			  assetedit.C
$(OBJDIR)/assetmanager.o: 			  assetmanager.C
$(OBJDIR)/assetpopup.o: 			  assetpopup.C
$(OBJDIR)/assetremove.o: 			  assetremove.C
$(OBJDIR)/assets.o: 				  assets.C
$(OBJDIR)/atrack.o: 				  atrack.C
$(OBJDIR)/attachmentpoint.o: 			  attachmentpoint.C
$(OBJDIR)/audio1394.o:  			  audio1394.C
$(OBJDIR)/audioalsa.o:  			  audioalsa.C
$(OBJDIR)/audioconfig.o: 			  audioconfig.C
$(OBJDIR)/audiodevice.o: 			  audiodevice.C
$(OBJDIR)/audioesound.o: 			  audioesound.C
$(OBJDIR)/audioidevice.o: 			  audioidevice.C
$(OBJDIR)/audioinprefs.o: 			  audioinprefs.C
$(OBJDIR)/audioodevice.o: 			  audioodevice.C
$(OBJDIR)/audiooss.o: 				  audiooss.C
$(OBJDIR)/audiooutprefs.o: 			  audiooutprefs.C
$(OBJDIR)/auto.o: 				  auto.C
$(OBJDIR)/autos.o: 				  autos.C
$(OBJDIR)/autoconf.o: 				  autoconf.C
$(OBJDIR)/automation.o: 			  automation.C
$(OBJDIR)/awindow.o: 				  awindow.C
$(OBJDIR)/awindowgui.o: 			  awindowgui.C
$(OBJDIR)/awindowmenu.o: 			  awindowmenu.C
$(OBJDIR)/batch.o: 				  batch.C
$(OBJDIR)/batchrender.o:			  batchrender.C
$(OBJDIR)/bezierauto.o: 			  bezierauto.C
$(OBJDIR)/bezierautos.o: 			  bezierautos.C
$(OBJDIR)/bitspopup.o:  			  bitspopup.C
$(OBJDIR)/brender.o:                              brender.C
$(OBJDIR)/browsebutton.o: 			  browsebutton.C
$(OBJDIR)/byteorderpopup.o: 			  byteorderpopup.C
$(OBJDIR)/cache.o: 				  cache.C
$(OBJDIR)/canvas.o: 				  canvas.C
$(OBJDIR)/canvastools.o: 			  canvastools.C
$(OBJDIR)/channel.o: 				  channel.C
$(OBJDIR)/channeledit.o: 			  channeledit.C
$(OBJDIR)/channelpicker.o: 			  channelpicker.C
$(OBJDIR)/chantables.o: 			  chantables.C
$(OBJDIR)/clipedit.o: 			  	  clipedit.C
$(OBJDIR)/commonrender.o: 			  commonrender.C
$(OBJDIR)/confirmsave.o: 			  confirmsave.C
$(OBJDIR)/confirmquit.o: 			  confirmquit.C
$(OBJDIR)/console.o: 				  console.C
$(OBJDIR)/consolescroll.o: 			  consolescroll.C
$(OBJDIR)/cpanel.o: 				  cpanel.C
$(OBJDIR)/cplayback.o:  			  cplayback.C
$(OBJDIR)/cropvideo.o:  			  cropvideo.C
$(OBJDIR)/ctimebar.o:                             ctimebar.C
$(OBJDIR)/ctracking.o:  			  ctracking.C
$(OBJDIR)/cursor.o: 				  cursor.C
$(OBJDIR)/cwindow.o: 				  cwindow.C
$(OBJDIR)/cwindowgui.o: 			  cwindowgui.C
$(OBJDIR)/cwindowtool.o: 			  cwindowtool.C
$(OBJDIR)/dcoffset.o: 				  dcoffset.C
$(OBJDIR)/deleteallindexes.o: 			  deleteallindexes.C
$(OBJDIR)/device1394input.o:                      device1394input.C
$(OBJDIR)/device1394output.o:                     device1394output.C
$(OBJDIR)/drivesync.o:  			  drivesync.C
$(OBJDIR)/edit.o: 				  edit.C
$(OBJDIR)/edithandles.o: 			  edithandles.C
$(OBJDIR)/editpanel.o:  			  editpanel.C
$(OBJDIR)/editpopup.o:  			  editpopup.C
$(OBJDIR)/edits.o: 				  edits.C
$(OBJDIR)/edl.o: 				  edl.C
$(OBJDIR)/edlsession.o: 			  edlsession.C
$(OBJDIR)/fadeengine.o: 			  fadeengine.C
$(OBJDIR)/file.o: 				  file.C
$(OBJDIR)/fileavi.o: 				  fileavi.C
$(OBJDIR)/filebase.o: 				  filebase.C
$(OBJDIR)/filebaseaudio.o: 			  filebaseaudio.C
$(OBJDIR)/filebaseima4.o: 			  filebaseima4.C
$(OBJDIR)/filebaseulaw.o: 			  filebaseulaw.C
$(OBJDIR)/filebasevideo.o: 			  filebasevideo.C
$(OBJDIR)/fileformat.o: 			  fileformat.C
$(OBJDIR)/filejpeg.o: 				  filejpeg.C
$(OBJDIR)/filelist.o: 				  filelist.C
$(OBJDIR)/filemov.o: 				  filemov.C
$(OBJDIR)/filempeg.o:                             filempeg.C
$(OBJDIR)/filepng.o: 				  filepng.C
$(OBJDIR)/filesndfile.o: 			  filesndfile.C
$(OBJDIR)/filetga.o: 			  	  filetga.C
$(OBJDIR)/filethread.o: 			  filethread.C
$(OBJDIR)/filetiff.o: 				  filetiff.C
$(OBJDIR)/filevorbis.o: 			  filevorbis.C
$(OBJDIR)/filexml.o: 				  filexml.C
$(OBJDIR)/floatauto.o:  			  floatauto.C
$(OBJDIR)/floatautos.o: 			  floatautos.C
$(OBJDIR)/formatcheck.o: 			  formatcheck.C
$(OBJDIR)/formatpopup.o: 			  formatpopup.C
$(OBJDIR)/formattools.o: 			  formattools.C
$(OBJDIR)/indexfile.o:  			  indexfile.C
$(OBJDIR)/indexthread.o: 			  indexthread.C
$(OBJDIR)/intauto.o:                              intauto.C
$(OBJDIR)/intautos.o:                             intautos.C
$(OBJDIR)/interfaceprefs.o: 			  interfaceprefs.C
$(OBJDIR)/keyframe.o: 				  keyframe.C
$(OBJDIR)/keyframes.o:  			  keyframes.C
$(OBJDIR)/keyframehandles.o: 			  keyframehandles.C
$(OBJDIR)/labelnavigate.o: 			  labelnavigate.C
$(OBJDIR)/labels.o: 				  labels.C
$(OBJDIR)/levelwindow.o: 			  levelwindow.C
$(OBJDIR)/levelwindowgui.o: 			  levelwindowgui.C
$(OBJDIR)/loadbalance.o: 			  loadbalance.C
$(OBJDIR)/loadfile.o: 				  loadfile.C
$(OBJDIR)/loadmode.o: 				  loadmode.C
$(OBJDIR)/localsession.o: 			  localsession.C
$(OBJDIR)/main.o:	  			  main.C
$(OBJDIR)/mainclock.o:  			  mainclock.C
$(OBJDIR)/maincursor.o: 			  maincursor.C
$(OBJDIR)/mainindexes.o: 			  mainindexes.C
$(OBJDIR)/mainmenu.o: 				  mainmenu.C
$(OBJDIR)/mainprogress.o: 			  mainprogress.C
$(OBJDIR)/mainsession.o: 			  mainsession.C
$(OBJDIR)/mainundo.o: 				  mainundo.C
$(OBJDIR)/maskauto.o:                             maskauto.C
$(OBJDIR)/maskautos.o:                            maskautos.C
$(OBJDIR)/maskengine.o:                           maskengine.C
$(OBJDIR)/mbuttons.o: 				  mbuttons.C
$(OBJDIR)/menuaeffects.o: 			  menuaeffects.C
$(OBJDIR)/menueffects.o: 			  menueffects.C
$(OBJDIR)/menuveffects.o: 			  menuveffects.C
$(OBJDIR)/messages.o: 				  messages.C
$(OBJDIR)/meterpanel.o: 			  meterpanel.C
$(OBJDIR)/module.o: 				  module.C
$(OBJDIR)/mtimebar.o:                             mtimebar.C
$(OBJDIR)/mwindow.o: 				  mwindow.C
$(OBJDIR)/mwindowedit.o: 			  mwindowedit.C
$(OBJDIR)/mwindowgui.o: 			  mwindowgui.C
$(OBJDIR)/mwindowmove.o: 			  mwindowmove.C
$(OBJDIR)/new.o: 				  new.C
$(OBJDIR)/newfolder.o:  			  newfolder.C
$(OBJDIR)/overlayframe.o: 			  overlayframe.C
$(OBJDIR)/packagedispatcher.o:                    packagedispatcher.C
$(OBJDIR)/packagerenderer.o: 			  packagerenderer.C
$(OBJDIR)/panauto.o: 			          panauto.C
$(OBJDIR)/panautos.o: 			          panautos.C
$(OBJDIR)/patch.o: 				  patch.C
$(OBJDIR)/patchbay.o: 				  patchbay.C
$(OBJDIR)/patchgui.o: 				  patchgui.C
$(OBJDIR)/performanceprefs.o: 			  performanceprefs.C
$(OBJDIR)/playabletracks.o: 			  playabletracks.C
$(OBJDIR)/playbackconfig.o: 			  playbackconfig.C
$(OBJDIR)/playbackengine.o: 			  playbackengine.C
$(OBJDIR)/playbackprefs.o:                        playbackprefs.C
$(OBJDIR)/playtransport.o: 			  playtransport.C
$(OBJDIR)/plugin.o: 				  plugin.C
$(OBJDIR)/pluginaclient.o: 			  pluginaclient.C
$(OBJDIR)/pluginaclientlad.o: 			  pluginaclientlad.C
$(OBJDIR)/pluginarray.o: 			  pluginarray.C
$(OBJDIR)/pluginautos.o: 			  pluginautos.C
$(OBJDIR)/pluginclient.o: 			  pluginclient.C
$(OBJDIR)/plugindialog.o: 			  plugindialog.C
$(OBJDIR)/pluginpopup.o: 			  pluginpopup.C
$(OBJDIR)/pluginprefs.o: 			  pluginprefs.C
$(OBJDIR)/pluginset.o:  			  pluginset.C
$(OBJDIR)/pluginserver.o: 			  pluginserver.C
$(OBJDIR)/plugintclient.o: 			  plugintclient.C
$(OBJDIR)/pluginvclient.o: 			  pluginvclient.C
$(OBJDIR)/preferences.o: 			  preferences.C
$(OBJDIR)/preferencesthread.o:  		  preferencesthread.C
$(OBJDIR)/presentations.o: 			  presentations.C
$(OBJDIR)/previewaudio.o: 			  previewaudio.C
$(OBJDIR)/previewvideo.o: 			  previewvideo.C
$(OBJDIR)/question.o: 				  question.C
$(OBJDIR)/quit.o: 				  quit.C
$(OBJDIR)/recconfirmdelete.o: 			  recconfirmdelete.C
$(OBJDIR)/record.o: 				  record.C
$(OBJDIR)/recordableatracks.o:  		  recordableatracks.C
$(OBJDIR)/recordablevtracks.o:  		  recordablevtracks.C
$(OBJDIR)/recordaudio.o: 			  recordaudio.C
$(OBJDIR)/recordconfig.o: 			  recordconfig.C
$(OBJDIR)/recordgui.o:  			  recordgui.C
$(OBJDIR)/recordlabel.o: 			  recordlabel.C
$(OBJDIR)/recordmonitor.o: 			  recordmonitor.C
$(OBJDIR)/recordprefs.o:                          recordprefs.C
$(OBJDIR)/recordpreview.o: 			  recordpreview.C
$(OBJDIR)/recordthread.o: 			  recordthread.C
$(OBJDIR)/recordtransport.o: 			  recordtransport.C
$(OBJDIR)/recordvideo.o: 			  recordvideo.C
$(OBJDIR)/recordwindow.o: 			  recordwindow.C
$(OBJDIR)/render.o: 				  render.C
$(OBJDIR)/renderengine.o: 			  renderengine.C
$(OBJDIR)/renderfarm.o: 			  renderfarm.C
$(OBJDIR)/renderfarmclient.o: 			  renderfarmclient.C
$(OBJDIR)/renderfarmfsclient.o: 		  renderfarmfsclient.C
$(OBJDIR)/renderfarmfsserver.o: 		  renderfarmfsserver.C
$(OBJDIR)/resample.o: 				  resample.C
$(OBJDIR)/resizetrackthread.o:                    resizetrackthread.C
$(OBJDIR)/resourcepixmap.o: 			  resourcepixmap.C
$(OBJDIR)/samplescroll.o: 			  samplescroll.C
$(OBJDIR)/savefile.o: 				  savefile.C
$(OBJDIR)/setformat.o:                            setformat.C
$(OBJDIR)/sharedlocation.o: 			  sharedlocation.C
$(OBJDIR)/shmemory.o: 				  shmemory.C
$(OBJDIR)/sighandler.o:                           sighandler.C
$(OBJDIR)/splashgui.o: 				  splashgui.C
$(OBJDIR)/statusbar.o:  			  statusbar.C
$(OBJDIR)/theme.o: 				  theme.C
$(OBJDIR)/threadexec.o:                           threadexec.C
$(OBJDIR)/threadfork.o:                           threadfork.C
$(OBJDIR)/threadindexer.o: 			  threadindexer.C
$(OBJDIR)/threadloader.o: 			  threadloader.C
$(OBJDIR)/timebar.o: 				  timebar.C
$(OBJDIR)/timebomb.o: 				  timebomb.C
$(OBJDIR)/timeentry.o:  			  timeentry.C
$(OBJDIR)/toggleauto.o: 			  toggleauto.C
$(OBJDIR)/toggleautos.o: 			  toggleautos.C
$(OBJDIR)/track.o: 				  track.C
$(OBJDIR)/trackcanvas.o: 			  trackcanvas.C
$(OBJDIR)/tracking.o: 				  tracking.C
$(OBJDIR)/tracks.o: 				  tracks.C
$(OBJDIR)/trackscroll.o: 			  trackscroll.C
$(OBJDIR)/tracksedit.o: 			  tracksedit.C
$(OBJDIR)/transition.o: 			  transition.C
$(OBJDIR)/transitionhandles.o:  		  transitionhandles.C
$(OBJDIR)/transitionpopup.o: 			  transitionpopup.C
$(OBJDIR)/transportque.o: 			  transportque.C
$(OBJDIR)/undostack.o:  			  undostack.C
$(OBJDIR)/vattachmentpoint.o: 			  vattachmentpoint.C
$(OBJDIR)/vautomation.o: 			  vautomation.C
$(OBJDIR)/vdevice1394.o: 			  vdevice1394.C
$(OBJDIR)/vdevicebase.o: 			  vdevicebase.C
$(OBJDIR)/vdevicebuz.o: 			  vdevicebuz.C
$(OBJDIR)/vdeviceprefs.o: 			  vdeviceprefs.C
$(OBJDIR)/vdevicev4l.o: 			  vdevicev4l.C
$(OBJDIR)/vdevicex11.o: 			  vdevicex11.C
$(OBJDIR)/vedit.o: 				  vedit.C
$(OBJDIR)/vedits.o: 				  vedits.C
$(OBJDIR)/videoconfig.o: 			  videoconfig.C
$(OBJDIR)/videodevice.o: 			  videodevice.C
$(OBJDIR)/videoinprefs.o: 			  videoinprefs.C
$(OBJDIR)/videooutprefs.o: 			  videooutprefs.C
$(OBJDIR)/videowindow.o: 			  videowindow.C
$(OBJDIR)/videowindowgui.o: 			  videowindowgui.C
$(OBJDIR)/viewmenu.o: 				  viewmenu.C
$(OBJDIR)/virtualnode.o: 			  virtualnode.C
$(OBJDIR)/virtualaconsole.o: 			  virtualaconsole.C
$(OBJDIR)/virtualanode.o: 			  virtualanode.C
$(OBJDIR)/virtualconsole.o: 			  virtualconsole.C
$(OBJDIR)/virtualvconsole.o: 			  virtualvconsole.C
$(OBJDIR)/virtualvnode.o: 			  virtualvnode.C
$(OBJDIR)/vmodule.o: 				  vmodule.C
$(OBJDIR)/vpatchgui.o:  			  vpatchgui.C
$(OBJDIR)/vplayback.o:  			  vplayback.C
$(OBJDIR)/vplugin.o: 				  vplugin.C
$(OBJDIR)/vpluginarray.o: 			  vpluginarray.C
$(OBJDIR)/vpluginset.o: 			  vpluginset.C
$(OBJDIR)/vtimebar.o:                             vtimebar.C
$(OBJDIR)/vrender.o: 				  vrender.C
$(OBJDIR)/vtrack.o: 				  vtrack.C
$(OBJDIR)/vtracking.o:  			  vtracking.C
$(OBJDIR)/vwindow.o: 				  vwindow.C
$(OBJDIR)/vwindowgui.o: 			  vwindowgui.C
$(OBJDIR)/workarounds.o:                          workarounds.C
$(OBJDIR)/zoombar.o: 				  zoombar.C
$(OBJDIR)/zoompanel.o:  			  zoompanel.C
