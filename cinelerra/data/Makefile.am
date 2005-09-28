noinst_LIBRARIES = libimagedata.a
libimagedata_a_SOURCES =
nodist_libimagedata_a_SOURCES = theme.data

PNGS = \
	mode_add.png \
	mode_divide.png \
	mode_multiply.png \
	mode_normal.png \
	mode_replace.png \
	mode_subtract.png

# this rule creates the .o file from the concatenated PNGs
.data.$(OBJEXT):
	$(OBJCOPY) -I binary -B $(OBJCOPYARCH) -O $(OBJCOPYTARGET) $< $@

theme.data: $(PNGS)
	$(top_builddir)/guicast/bootstrap $@ $^ || { rm -f $@; exit 1; }

clean-local:
	rm -f theme.data

EXTRA_DIST = \
	$(PNGS) \
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
	heroine_logo12.png \
	heroine_logo12_png.h \
	heroine_logo9.png \
	heroine_logo9_png.h \
	lad_picon.png \
	lad_picon.xcf.bz2 \
	lad_picon_png.h \
	modes.xcf.bz2
