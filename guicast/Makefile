include ../global_config

# Need the freetype include that matches the X server for Xft rendering
#FREETYPE_DIR := ..//usr/X11R6/include/freetype2
FREETYPE_DIR := $(shell expr ../freetype*/include )

CFLAGS += -I../quicktime -I$(FREETYPE_DIR)

# must only define here so applications using libpng don't see any of the XFT
# includes at all.  They use a conflicting setjmp.
ifeq ($(HAVE_XFT), y)
CFLAGS += -DHAVE_XFT
endif

#CFLAGS += -g

$(shell sh -c 'if ! test -d $(OBJDIR)\; then mkdir $(OBJDIR)\; fi' )

OBJS = \
	$(OBJDIR)/bcbitmap.o \
	$(OBJDIR)/bcbutton.o \
	$(OBJDIR)/bccapture.o \
	$(OBJDIR)/bccounter.o \
	$(OBJDIR)/bcclipboard.o \
	$(OBJDIR)/bcdialog.o \
	$(OBJDIR)/bcdisplayinfo.o \
	$(OBJDIR)/bcdragwindow.o \
	$(OBJDIR)/bcfilebox.o \
	$(OBJDIR)/bcipc.o \
	$(OBJDIR)/bclistbox.o \
	$(OBJDIR)/bclistboxitem.o \
	$(OBJDIR)/bcmenu.o \
	$(OBJDIR)/bcmenubar.o \
	$(OBJDIR)/bcmenuitem.o \
	$(OBJDIR)/bcmenupopup.o \
	$(OBJDIR)/bcmeter.o \
	$(OBJDIR)/bcpan.o \
	$(OBJDIR)/bcpixmap.o \
	$(OBJDIR)/bcpopup.o \
	$(OBJDIR)/bcpopupmenu.o \
	$(OBJDIR)/bcpot.o \
	$(OBJDIR)/bcprogress.o \
	$(OBJDIR)/bcprogressbox.o \
	$(OBJDIR)/bcrepeater.o \
	$(OBJDIR)/bcresources.o \
	$(OBJDIR)/bcscrollbar.o \
	$(OBJDIR)/bcsignals.o \
	$(OBJDIR)/bcslider.o \
	$(OBJDIR)/bcsubwindow.o \
	$(OBJDIR)/bctextbox.o \
	$(OBJDIR)/bctheme.o \
	$(OBJDIR)/bctitle.o \
	$(OBJDIR)/bctoggle.o \
	$(OBJDIR)/bctumble.o \
	$(OBJDIR)/bcwindow.o \
	$(OBJDIR)/bcwindowbase.o \
	$(OBJDIR)/bcwindowdraw.o \
	$(OBJDIR)/condition.o \
	$(OBJDIR)/errorbox.o \
	$(OBJDIR)/defaults.o \
	$(OBJDIR)/filesystem.o \
	$(OBJDIR)/mutex.o \
	$(OBJDIR)/rotateframe.o \
	$(OBJDIR)/sema.o \
	$(OBJDIR)/stringfile.o \
	$(OBJDIR)/thread.o \
	$(OBJDIR)/testobject.o \
	$(OBJDIR)/timer.o \
	$(OBJDIR)/units.o \
	$(OBJDIR)/vframe.o \
	$(OBJDIR)/workarounds.o

OUTPUT = $(OBJDIR)/libguicast.so
STATICOUTPUT = $(OBJDIR)/libguicast.a

TESTLIBS := \
		$(STATICOUTPUT) \
		../quicktime/$(OBJDIR)/libquicktime.a \
		$(LFLAGS) \
		$(GLLIBS) \
		-lX11 \
		-lXext \
		-lXv \
		-lpthread \
		-lm \
		$(XXF86VM) \
		-lpng

ifeq ($(HAVE_XFT), y)
TESTLIBS += -lXft
endif	

UTILS = $(OBJDIR)/bootstrap $(OBJDIR)/pngtoh

$(shell echo $(CFLAGS) > $(OBJDIR)/c_flags)
$(shell echo $(OBJS) $(CXXREPOSITORY) > $(OBJDIR)/objs)


# PTHREAD DOESN'T WORK WHEN LINKED HERE
all: $(STATICOUTPUT) $(UTILS)


$(STATICOUTPUT): $(OBJS)
	ar rcs $(STATICOUTPUT) `cat $(OBJDIR)/objs`

$(OBJDIR)/bootstrap:
	gcc -O2 bootstrap.c -o $(OBJDIR)/bootstrap

$(OBJDIR)/pngtoh:
	gcc -O2 pngtoh.c -o $(OBJDIR)/pngtoh

replace:  replace.o $(STATICOUTPUT)
	$(CC) -o replace replace.o $(STATICOUTPUT) \
	../quicktime/i686/libquicktime.a \
	$(LFLAGS) \
	$(XLIBS) \
	$(XXF86VM) \
	-lpng

test:	$(OBJDIR)/test.o $(STATICOUTPUT)
	$(CC) -o $(OBJDIR)/test \
		$(OBJDIR)/test.o \
		$(TESTLIBS)

test2:	$(OBJDIR)/test2.o $(STATICOUTPUT)
	$(CC) -o $(OBJDIR)/test2 \
		$(OBJDIR)/test2.o \
		$(TESTLIBS)

test3:	$(OBJDIR)/test3.o $(STATICOUTPUT)
	$(CC) -o $(OBJDIR)/test3 \
		$(OBJDIR)/test3.o \
		$(TESTLIBS)

clean:
	rm -rf $(OBJDIR)
	find \( -name core \
		-o -name '*.o' \
		-o -name '*.a' \
		-o -name '*.so' \) -exec rm -f {} \;

wc:
	cat *.C *.h | wc

backup: clean
	cd .. && \
	tar Icvf ~/guicast.tar.bz2 guicast

$(OBJS) $(OBJDIR)/test.o $(OBJDIR)/test2.o $(OBJDIR)/test3.o $(OBJDIR)/replace.o:
	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.C) -o $*.o

$(OBJDIR)/bootstrap: bootstrap.c
$(OBJDIR)/pngtoh: pngtoh.c

$(OBJDIR)/bcbitmap.o: 	   				      bcbitmap.C
$(OBJDIR)/bcbutton.o: 	   				      bcbutton.C
$(OBJDIR)/bccapture.o:     				      bccapture.C
$(OBJDIR)/bccounter.o:                                        bccounter.C
$(OBJDIR)/bcclipboard.o:   				      bcclipboard.C
$(OBJDIR)/bcdialog.o:                                         bcdialog.C
$(OBJDIR)/bcdisplayinfo.o: 				      bcdisplayinfo.C
$(OBJDIR)/bcdragwindow.o:  				      bcdragwindow.C
$(OBJDIR)/bcfilebox.o:     				      bcfilebox.C
$(OBJDIR)/bcipc.o: 	   				      bcipc.C
$(OBJDIR)/bclistbox.o:     				      bclistbox.C
$(OBJDIR)/bclistboxitem.o:     				      bclistboxitem.C
$(OBJDIR)/bcmenu.o:                                           bcmenu.C
$(OBJDIR)/bcmenubar.o:                                        bcmenubar.C
$(OBJDIR)/bcmenuitem.o:                                       bcmenuitem.C
$(OBJDIR)/bcmenupopup.o:                                      bcmenupopup.C
$(OBJDIR)/bcmeter.o: 	   				      bcmeter.C
$(OBJDIR)/bcpan.o: 	   				      bcpan.C
$(OBJDIR)/bcpixmap.o: 	   				      bcpixmap.C
$(OBJDIR)/bcpopup.o: 	   				      bcpopup.C
$(OBJDIR)/bcpopupmenu.o:   				      bcpopupmenu.C
$(OBJDIR)/bcpot.o: 	   				      bcpot.C
$(OBJDIR)/bcprogress.o:    				      bcprogress.C
$(OBJDIR)/bcprogressbox.o: 				      bcprogressbox.C
$(OBJDIR)/bcrepeater.o:    				      bcrepeater.C
$(OBJDIR)/bcresources.o:   				      bcresources.C
$(OBJDIR)/bcscrollbar.o:   				      bcscrollbar.C
$(OBJDIR)/bcsignals.o:     				      bcsignals.C
$(OBJDIR)/bcslider.o: 	   				      bcslider.C
$(OBJDIR)/bcsubwindow.o:   				      bcsubwindow.C
$(OBJDIR)/bctextbox.o:     				      bctextbox.C
$(OBJDIR)/bctitle.o: 	   				      bctitle.C
$(OBJDIR)/bctheme.o: 	   				      bctheme.C
$(OBJDIR)/bctoggle.o: 	   				      bctoggle.C
$(OBJDIR)/bctumble.o: 	   				      bctumble.C
$(OBJDIR)/bcwindow.o: 	   				      bcwindow.C
$(OBJDIR)/bcwindowbase.o:  				      bcwindowbase.C
$(OBJDIR)/bcwindowdraw.o:  				      bcwindowdraw.C
$(OBJDIR)/condition.o:  				      condition.C
$(OBJDIR)/errorbox.o: 	   				      errorbox.C
$(OBJDIR)/defaults.o: 	   				      defaults.C
$(OBJDIR)/filesystem.o:    				      filesystem.C
$(OBJDIR)/mutex.o: 	   				      mutex.C
$(OBJDIR)/rotateframe.o:                                      rotateframe.C
$(OBJDIR)/sema.o: 	   				      sema.C
$(OBJDIR)/stringfile.o:    				      stringfile.C
$(OBJDIR)/test.o: 	   				      test.C
$(OBJDIR)/test2.o: 	   				      test2.C
$(OBJDIR)/test3.o: 	   				      test3.C
$(OBJDIR)/testobject.o:                                       testobject.C
$(OBJDIR)/thread.o: 	   				      thread.C
$(OBJDIR)/timer.o: 	   				      timer.C
$(OBJDIR)/units.o: 	   				      units.C
$(OBJDIR)/vframe.o: 	   				      vframe.C
$(OBJDIR)/workarounds.o:   				      workarounds.C


