SUBDIRS=libmpeg3 quicktime mpeg2enc toolame-02l libsndfile \
	guicast cinelerra mplexhi mplexlo plugins po
EXTRA_DIST = admin debian doc m4 depcomp README.BUILD
AUTOMAKE_OPTIONS=foreign
ACLOCAL_AMFLAGS = -I m4

dist-hook:
	-rm -rf $(distdir)/admin/CVS
	-rm -rf $(distdir)/debian/tmp
	-rm -rf $(distdir)/debian/CVS
	-rm -rf $(distdir)/debian/patches/CVS
	-rm -rf $(distdir)/doc/CVS
