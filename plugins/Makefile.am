SUBDIRS=1080to480 720to480 denoisemjpeg motion libfourier libeffecttv \
				parametric bandslide bandwipe blur libcolors brightness burn aging cdripper \
				 chromakey colorbalance compressor crossfade deinterlace delayaudio delayvideo \
				 denoise denoisefft denoisevideo despike dissolve dot fieldframe flip framefield \
				 freeverb freezeframe gain holo huesaturation interpolateall invertaudio invertvideo \
				 irissquare ivtc normalize oilpainting pitch polar reframe resample reverb reverseaudio \
				 reversevideo rgb601 rotate scale sharpen shiftinterlace slide spectrogram swapchannels \
				 synthesizer timeavg timestretch titler translate videoscope wave whirl wipe yuv microtheme \
				 defaulttheme downsample flash gradient level linearblur \
				 perspective radialblur zoomblur \
				 svg histogram fonts

AUTOMAKE_OPTIONS=foreign

noinst_PROGRAMS=bootstrap
bootstrap_SOURCES=bootstrap.c

EXTRA_DIST = install_plugins themes

install-exec-hook:
	@echo "Postprocessing plugins in $(DESTDIR)$(plugindir)"
	for theme in `cat $(srcdir)/themes`; do \
	echo $(DESTDIR)$(plugindir)/$$theme.so; \
	./bootstrap $(DESTDIR)$(plugindir)/$$theme.so $(srcdir)/$$theme/data/*.png; \
	done
	cd $(DESTDIR)$(plugindir); \
	rm -f *.a *.la; \
	for file in *.so; do \
	newfile=$${file/.so/.plugin}; \
	mv -f $$file $$newfile; \
	done; \
	mv libcolors.plugin libcolors.so; \
	mv libfourier.plugin libfourier.so; \
	mv libeffecttv.plugin libeffecttv.so


