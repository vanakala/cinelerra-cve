THEMES= \
	defaulttheme

SUBDIRS= \
	$(THEMES) \
	libfourier \
	libeffecttv \
	colors \
	1080to480 \
	720to480 \
	denoisemjpeg \
	motion \
	parametric \
	bandslide \
	bandwipe \
	blur \
	brightness \
	burn \
	aging \
	cdripper \
				 chromakey colorbalance compressor crossfade deinterlace delayaudio delayvideo \
				 denoise denoisefft denoisevideo despike dissolve dot fieldframe flip framefield \
				 freeverb freezeframe gain holo huesaturation interpolateall invertaudio invertvideo \
				 irissquare ivtc normalize oilpainting pitch polar reframe resample reverb reverseaudio \
				 reversevideo rgb601 rotate scale sharpen svg shiftinterlace slide spectrogram swapchannels \
	synthesizer \
	timeavg \
	timestretch \
	titler \
	translate \
	videoscope \
	wave \
	whirl \
	wipe \
	yuv \
	downsample \
	flash \
	gradient \
	level \
	linearblur \
				 perspective radialblur zoomblur \
	histogram \
	fonts

AUTOMAKE_OPTIONS=foreign

EXTRA_DIST = install_plugins themes

install-exec-hook:
	@echo "Postprocessing plugins in $(DESTDIR)$(plugindir)"
	list="$(THEMES)"; \
	for theme in $$list; do \
	echo $(DESTDIR)$(plugindir)/$$theme.so; \
	$(top_builddir)/guicast/bootstrap $(DESTDIR)$(plugindir)/$$theme.so $(srcdir)/$$theme/data/*.png; \
	done
	cd $(DESTDIR)$(plugindir); \
	rm -f *.a *.la
