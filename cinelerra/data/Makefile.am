all-local: register_images.h

noinst_LIBRARIES = libimagedata.a
libimagedata_a_SOURCES = \
	mode_add.png \
	mode_divide.png \
	mode_multiply.png \
	mode_normal.png \
	mode_replace.png \
	mode_subtract.png

# this rule creates the .o files from the pngs
# the .png must be copied to the build directory so that only the basename
# (without a path) is used in the symbol name that objcopy inserts in the .o
# the test ensures that we don't erase the .png if $(srcdir) == $(builddir)
.png.$(OBJEXT):
	f=`basename $<`; \
	test -f $$f || ln -sf $< $$f; \
	$(OBJCOPY) -I binary -B $(OBJCOPYARCH) -O $(OBJCOPYTARGET) $$f $@

# this creates a file that, if included in the constructor
# of a BCTheme derived class, will link in and register
# the .png data from libimagedata.a
register_images.h: $(libimagedata_a_SOURCES) Makefile.am
	(echo "// This file is generated automatically. Do not edit."; \
	for i in $(libimagedata_a_SOURCES); do \
	  png=`basename $$i .png`; \
	  echo "extern unsigned char _binary_$${png}_png_start[], _binary_$${png}_png_end[];"; \
	  echo "register_image(\"$$png.png\", _binary_$${png}_png_start, _binary_$${png}_png_end-_binary_$${png}_png_start);"; \
	done) > $@ || { rm -f $@; exit 1; }

BUILTSOURCES = register_images.h

EXTRA_DIST = \
	about.png \
	about.xcf.bz2 \
	about_bg.png \
	about_bg_png.h \
	about_png.h \
	fake_ntsc.dv \
	fake_ntsc_dv.h \
	fake_pal.dv \
	fake_pal_dv.h \
	heroine_logo10.png \
	heroine_logo10_png.h \
	heroine_logo11.png \
	heroine_logo11_png.h \
	heroine_logo9.png \
	heroine_logo9_png.h \
	lad_picon.png \
	lad_picon.xcf.bz2 \
	lad_picon_png.h \
	microsoft_credit.png \
	microsoft_credit_png.h \
	modes.xcf.bz2
