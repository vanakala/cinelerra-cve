SUBDIRS= fonts libcolors libfourier libeffecttv \
				 parametric bandslide bandwipe blur brightness burn aging cdripper \
				 chromakey colorbalance compressor crossfade deinterlace delayaudio delayvideo \
				 denoise denoisefft denoisevideo despike dissolve dot fieldframe flip framefield \
				 freeverb freezeframe gain holo huesaturation interpolateall invertaudio invertvideo \
				 irissquare ivtc normalize oilpainting pitch polar reframe resample reverb reverseaudio \
				 reversevideo rgb601 rotate scale sharpen shiftinterlace slide spectrogram swapchannels \
				 synthesizer timeavg timestretch titler translate videoscope wave whirl wipe yuv microtheme \
				 defaulttheme downsample flash gradient lame level linearblur perspective radialblur zoomblur toolame \
				 mpeg2enc

AUTOMAKE_OPTIONS=foreign

noinst_PROGRAMS=bootstrap
bootstrap_SOURCES=bootstrap.c

install-exec-hook:
	./install_plugins $(DESTDIR)$(plugindir) 

