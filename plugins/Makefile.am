THEMES= \
	microtheme \
	defaulttheme

SUBDIRS= \
	$(THEMES) \
	libfourier \
	libeffecttv \
	colors \
	1080to480 \
	720to480 \
	aging \
	bandslide \
	bandwipe \
	blur \
	brightness \
	burn \
	cdripper \
	chromakey \
	colorbalance \
	compressor \
	crossfade \
	decimate \
	deinterlace \
	delayaudio \
	delayvideo \
	denoise \
	denoisefft \
	denoisemjpeg \
	denoisevideo \
	despike \
	diffkey \
	dissolve \
	dot \
	downsample \
	fieldframe \
	flash \
	flip \
	framefield \
	freeverb \
	freezeframe \
	gain \
	gradient \
	histogram \
	holo \
	huesaturation \
	interpolateall \
	invertaudio \
	invertvideo \
	irissquare \
	ivtc \
	level \
	linearblur \
	loopaudio \
	loopvideo \
	motion \
	normalize \
	oilpainting \
	overlay \
	parametric \
	perspective \
	pitch \
	polar \
	radialblur \
	reframe \
	resample \
	reverb \
	reverseaudio \
	reversevideo \
	rgb601 \
	rotate \
	scale \
	sharpen \
	shiftinterlace \
	slide \
	spectrogram \
	svg \
	swapchannels \
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
	zoomblur \
	fonts

AUTOMAKE_OPTIONS=foreign

EXTRA_DIST = 

install-exec-hook:
	@echo "Postprocessing plugins in $(DESTDIR)$(plugindir)"
	list="$(THEMES)"; \
	for theme in $$list; do \
	echo $(DESTDIR)$(plugindir)/$$theme.so; \
	$(top_builddir)/guicast/bootstrap $(DESTDIR)$(plugindir)/$$theme.so $(srcdir)/$$theme/data/*.png; \
	done
	cd $(DESTDIR)$(plugindir); \
	rm -f *.a *.la
