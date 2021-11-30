AC_INIT(Cinelerracve, 2.3.5, einarrunkaru@gmail.com)

AC_CANONICAL_SYSTEM

# Test for minimal version of autoconf
AC_PREREQ(2.57)

AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])
IT_PROG_INTLTOOL
GETTEXT_PACKAGE="AC_PACKAGE_NAME"
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[The prefix for our gettext translation domains])
AM_ICONV

AC_LANG_CPLUSPLUS
AC_LANG_C
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX(11, ext, optional)

AC_ENABLE_SHARED
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

# hack for libtool 1.5 -- this is ugly and probably wrong, but works
LTC_FLAGS="--tag=CC"
LTCXX_FLAGS="--tag=CXX"
AC_SUBST(LTC_FLAGS)
AC_SUBST(LTCXX_FLAGS)

if test "$am_cv_func_iconv" = "yes"; then
    funiconv=yes
fi

dnl Check for ladspa
AH_TEMPLATE(HAVE_LADSPA, [Define to 1 if ladspa-sdk is available.])
AC_CHECK_HEADER([ladspa.h],
	[AC_CHECK_MEMBER(struct _LADSPA_Descriptor.Name, [AC_DEFINE(HAVE_LADSPA) enable_ladspa=ok; ladspa_h=yes], [],
		[#include <ladspa.h>])],
	[],[] )

############## SPECIAL DIRECTORIES
AC_ARG_WITH(plugindir,
	AC_HELP_STRING([--with-plugindir], [sets the directory where the plugins should be installed]),
	[ plugindir=$withval ],
	[ plugindir="\${exec_prefix}/lib/cinelerra-cve" ])
AC_SUBST(plugindir)
############## END OF SPECIAL DIRECTORIES

############## XLIB
AC_PATH_XTRA
AC_CHECK_HEADERS([X11/extensions/XShm.h X11/extensions/Xvlib.h X11/extensions/xf86vmode.h],,[no_x=yes],
[ #include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h> ]
)
AC_CHECK_LIB([Xext], [XShmAttach],,[no_x=yes])
AC_CHECK_LIB([Xv], [XvGrabPort],,[no_x=yes])
AC_CHECK_LIB([Xxf86vm], [XF86VidModeGetAllModeLines],,[no_x=yes])
if test "$no_x" = yes; then
	if test "x$have_x" = "xdisabled"; then
		AC_MSG_WARN([Do not use --without-x or --with-x=no.])
	else
		AC_MSG_WARN([X Windows development tools were not found.])
		AC_MSG_WARN([Please install xlib-dev or xorg-x11-devel and])
		AC_MSG_WARN([development packages for Xext, Xv, Xxf86vm])
	fi
	AC_MSG_ERROR([Cinelerra requires X Windows.])
fi
X_LIBS="$X_LIBS -lXxf86vm -lXv -lXext -lX11"
AH_TEMPLATE(HAVE_LIBXXF86VM, [Define to 1 if libXxf86vm is available.])
AC_DEFINE(HAVE_LIBXXF86VM)
############## END XLIB

############## UTF8 suppport
AC_CHECK_DECL([X_HAVE_UTF8_STRING],,[no_utf=yes],[#include <X11/Xlib.h>])
if test "$no_utf" = "yes"; then
	AC_MSG_ERROR([Cinelerra requires utf8 support in X Windows.])
fi
############## UTF8 suppport

############## Freetype2 and fontconfig suppport
PKG_CHECK_MODULES(FREETYPE,freetype2,[freetype2=yes],:)
PKG_CHECK_MODULES(FONTCONFIG,fontconfig >= 2.4.2,[fontconfig=yes],:)
if test "x$freetype2" != "xyes" -o "x$fontconfig" != "xyes"; then
	AC_MSG_ERROR([Cinelerra requires freetype2 and fontconfig ver 2.4.2 or later])
fi
############## End of freetype2 and fontconfig suppport

############# XFT
AH_TEMPLATE(HAVE_XFT, [Define to 1 if libXft is available.])
PKG_CHECK_MODULES(XFT, xft, [libxft=yes],:)
if test "$libxft" = "yes"; then
	AC_DEFINE(HAVE_XFT)
	AC_SUBST(XFT_CFLAGS)
	AC_SUBST(XFT_LIBS)
	AC_DEFINE([PNG_SETJMP_NOT_SUPPORTED], [1], [Workaround to fix compile with xft])
	CXXFLAGS="$XFT_CFLAGS $CXXFLAGS"
fi
############## END XFT

############## SOUND
# the sound options are propagated by SOUND_CFLAGS and SOUND_LDFLAGS
AC_ARG_ENABLE(alsa,
	AC_HELP_STRING([--disable-alsa],[disable support for ALSA (default=autodetect)]),
	[enable_alsa=$enableval],[enable_alsa=yes])
AC_ARG_ENABLE(esd,
	AC_HELP_STRING([--disable-esd], [disable support for ESD (default=autodetect)]),
	[enable_esd=$enableval],[enable_esd=yes])

if test "x$enable_alsa" = "xyes"; then
	AM_PATH_ALSA(1.0.2,[alsa=yes])
	if test "x$alsa" = "xyes"; then
		SOUND_CFLAGS="$ALSA_CFLAGS -DHAVE_ALSA $SOUND_CFLAGS"
		SOUND_LDFLAGS="$ALSA_LIBS $SOUND_LDFLAGS"
		enable_alsa=ok
	fi
	dnl This added -lasound to LIBS. LIBS is used in every link command.
	dnl But we do not want to link all shared libraries against -lasound.
	dnl So we remove it again.
	LIBS=`echo "$LIBS" | sed -e s/-lasound//g`
fi

if test "x$enable_esd" = "xyes"; then
	AM_PATH_ESD(,[esound=yes])
	if test "x$esound" = xyes; then
		SOUND_CFLAGS="$ESD_CFLAGS -DHAVE_ESOUND $SOUND_CFLAGS"
		SOUND_LDFLAGS="$ESD_LIBS $SOUND_LDFLAGS"
		enable_esd=ok
	fi
fi
AC_SUBST(SOUND_CFLAGS)
AC_SUBST(SOUND_LDFLAGS)
############### END OF SOUND

############## MISC LIBRARIES
AC_CHECK_HEADER(uuid/uuid.h,[libuuidh=yes])
AC_CHECK_LIB(uuid, uuid_clear,[libuuid=yes])

AC_CHECK_HEADER(fftw3.h,[libfftw3h=yes])
AC_CHECK_LIB(fftw3, fftw_free,[libfftw3=yes])

PKG_CHECK_MODULES(PNG,libpng,[libpng=yes],:)

AC_CHECK_HEADER(jpeglib.h,[libjpegh=yes])
AC_CHECK_LIB(jpeg,jpeg_start_decompress,[libjpeg=yes])

AC_CHECK_HEADER(tiffio.h,[libtiffh=yes])
AC_CHECK_LIB(tiff,TIFFOpen,[libtiff=yes])
############## END OF MISC LIBRARIES

CINELERRA_CFLAGS="-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
 -D__STDC_FORMAT_MACROS  -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS"
AC_SUBST(CINELERRA_CFLAGS)

############# BUILDINFO display, (for displaying version / date)
AC_ARG_WITH(buildinfo,
		AC_HELP_STRING(
		[--with-buildinfo],
		[includes revision information in the 'About' box; allowed values: svn, git; add /recompile to force rebuild on each make invocation, or cust/"Custom string" for packagers]),
		[ buildinfo=$withval],
		[ buildinfo="none" ])
AC_SUBST(buildinfo)

BUILDINFO_repository=$(echo $buildinfo | sed 's=/recompile==')
BUILDINFO_recompile=$(echo $buildinfo | sed 's=.*/recompile=yes=')
BUILDINFO_custom=$(echo $buildinfo | sed 's=cust.*=yes=')
BUILDINFO_CUSTOM_TAG=$(echo $buildinfo | sed 's=cust/==')
AC_SUBST(BUILDINFO_CUSTOM_TAG)
AM_CONDITIONAL(BUILDINFO_GIT, test "x$BUILDINFO_repository" = "xgit")
AM_CONDITIONAL(BUILDINFO_SVN, test "x$BUILDINFO_repository" = "xsvn")
AM_CONDITIONAL(BUILDINFO_RECOMPILE, test "x$BUILDINFO_recompile" = "xyes")
AM_CONDITIONAL(BUILDINFO_CUST, test "x$BUILDINFO_custom" = "xyes")

echo removing cinelerra/versioninfo.h to force a re-build
rm -f cinelerra/versioninfo.h > /dev/null 2>&1

############# END BUILDINFO display, (for displaying version / date)

############ external ffmpeg libraries
AC_ARG_WITH([external-ffmpeg], AC_HELP_STRING([--with-external-ffmpeg], [use external ffmpeg library]))

if test "x$with_external_ffmpeg" = "xyes"; then
	PKG_CHECK_MODULES([FFMPEG],
		[libavcodec >= 56.60.100 libavformat >= 56.40.101
			libswresample >= 1.2.101 libavutil >= 54.31.100 libswscale >= 3.1.101],
		ffmpeg_external=yes)
	FFMPEG_FOLDER=""
	FFMPEG_EXTERNALTEXT="External ffmpeg"
else
	if test ! -d ffmpeg ; then
		FFMPEG_BRANCH="-b release/4.4"
		if ! git clone --depth 1 $FFMPEG_BRANCH https://git.ffmpeg.org/ffmpeg.git ffmpeg ; then
			rm -rf ffmpeg
			echo "Failed to get a FFmpeg checkout"
			exit 1
		fi
	fi
	FFMPEG_FOLDER=ffmpeg
	FFMPEG_CPPFLAGS="-I\$(top_srcdir)/ffmpeg"
	FFMPEG_LIBS="-L\$(top_builddir)/ffmpeg/libavcodec -lavcodec-cve \
		-L\$(top_builddir)/ffmpeg/libavformat -lavformat-cve \
		-L\$(top_builddir)/ffmpeg/libswresample -lswresample-cve \
		-L\$(top_builddir)/ffmpeg/libavutil -lavutil-cve \
		-L\$(top_builddir)/ffmpeg/libswscale -lswscale-cve"
	FFMPEG_EXTERNALTEXT="Internal ffmpeg"
fi
AC_SUBST(FFMPEG_FOLDER)
AC_SUBST(FFMPEG_CPPFLAGS)
AC_SUBST(FFMPEG_LIBS)
AC_SUBST(FFMPEG_EXTERNALTEXT)
############# END external ffmpeg libraries

echo --- FFMPEG CONFIGURATION SECTION --------------
if test "x$ffmpeg_external" = "xyes"; then
	echo Using external ffmpeg - no configuration needed
else
#### Check optional components of avlibs
	AC_ARG_ENABLE([nonfree], AC_HELP_STRING([--enable-nonfree], [enable nonfree codecs of avlibs]), [avlibs_nonfree=yes])
	if test "x$avlibs_nonfree" = xyes; then
		FFMPEG_CONF="--enable-nonfree"
	else
		FFMPEG_CONF=""
	fi

############## LIBFDK-AAC
	if test "x$avlibs_nonfree" = xyes; then
		PKG_CHECK_MODULES(libfdkaac,fdk-aac,[libfdkaac=yes],:)

		if test "x$libfdkaac" = xyes; then
			FFMPEG_CONF="$FFMPEG_CONF --enable-libfdk-aac"
		fi
	fi
############## End of LIBFDK-AAC

############## LAME
	# Just test to see if we have lame installed.
	AC_CHECK_HEADER(lame/lame.h,[libmp3lameh=yes],:)
	AC_CHECK_LIB(mp3lame,lame_init,[libmp3lame=yes])
	if test "x$libmp3lame$libmp3lameh" = xyesyes; then
		FFMPEG_CONF="$FFMPEG_CONF --enable-libmp3lame"
	else
		libmp3lame=no
	fi
############## End of LAME

############## x264
	PKG_CHECK_MODULES(x264,x264,[libx264=yes],:)
	if test "x$libx264" = xyes; then
		FFMPEG_CONF="$FFMPEG_CONF --enable-libx264"
	fi
############## End of x264

############## x265
	PKG_CHECK_MODULES(x265,x265,[libx265=yes],:)
	if test "x$libx265" = xyes; then
		FFMPEG_CONF="$FFMPEG_CONF --enable-libx265"
	fi
############## End of x265

############## vorbis
	PKG_CHECK_MODULES(libvorbis,vorbis,[libvorbis=yes],:)
	if test "x$libvorbis" = xyes; then
		FFMPEG_CONF="$FFMPEG_CONF --enable-libvorbis"
	fi
############## End of vorbis

############## theora
	PKG_CHECK_MODULES(libtheora,theora,[libtheora=yes],:)
	if test "x$libtheora" = xyes; then
		FFMPEG_CONF="$FFMPEG_CONF --enable-libtheora"
	fi
############## End of theora

############## librsvg
	PKG_CHECK_MODULES(librsvg,librsvg-2.0,[librsvg=yes],:)
	if test "x$librsvg" = xyes; then
		FFMPEG_CONF="$FFMPEG_CONF --enable-librsvg"
	fi
############## End of librsvg

############## opencore-amrnb
	PKG_CHECK_MODULES(amrnb,opencore-amrnb,[amrnb=yes],:)
	if test "x$amrnb" = xyes; then
		FFMPEG_CONF="$FFMPEG_CONF --enable-libopencore-amrnb"
	fi
############## End of opencore-amrnb

############## opencore-amrwb
	PKG_CHECK_MODULES(amrwb,opencore-amrwb,[amrwb=yes],:)
	if test "x$amrwb" = xyes; then
		FFMPEG_CONF="$FFMPEG_CONF --enable-libopencore-amrwb"
	fi
############## End of opencore-amrwb

############## libvpx
	PKG_CHECK_MODULES(libvpx,vpx,[libvpx=yes],:)
	if test "x$libvpx" = xyes; then
		FFMPEG_CONF="$FFMPEG_CONF --enable-libvpx"
	fi
############## End of libvpx

############## libopus
	PKG_CHECK_MODULES(libopus,opus,[libopus=yes],:)
	if test "x$libopus" = xyes; then
		FFMPEG_CONF="$FFMPEG_CONF --enable-libopus"
	fi
############## End of libopus

############## upgrade (L)GPL of avlibs to version 3
	if test "x$amrnb" = xyes -o "x$amrwb" = xyes; then
		FFMPEG_CONF="$FFMPEG_CONF --enable-version3"
	fi
############## End of version 3

############# Configure internal AVlibs
	ff_conf_args="--disable-programs --disable-doc \
		--disable-postproc --disable-avfilter --disable-network \
		--disable-static --enable-shared --enable-gpl \
		$FFMPEG_CONF \
		--build-suffix=-cve"

	srcdir_c=$(readlink -f $srcdir) &&
	mkdir -p ffmpeg/ &&
	(
		cd ffmpeg/
		# if --libdir was not set, then it expands to '$exec_prefix/lib'
		# if --exec-prefix was not set, then it expands to NONE
		# this makes sure $libdir is expanded to a useful value
		if test "x$exec_prefix" = xNONE; then
			exec_prefix=$acl_final_prefix
		fi
		cmd="ffmpeg/configure \$ff_conf_args \
			--prefix=\"\$acl_final_prefix\" \
			--libdir=\"$libdir\" --shlibdir=\"$libdir\" \
			--incdir=\"\$includedir\"/cinelerra-cve"
		eval echo $cmd
		eval \$srcdir_c/$cmd
	)
fi
echo --------------------------------------------------
############# END of Configure internal AVlibs

############## OpenGL
AH_TEMPLATE(HAVE_GL, [Define to 1 if OpenGL 2.0 is available.])
AC_ARG_ENABLE(opengl,
	AC_HELP_STRING([--disable-opengl],
		[disables hardware accelerated rendering (default=autodetect, requires OpenGL 2.0)]),
	[ enable_opengl=$enableval ],
	[ enable_opengl=auto ])
if test "x$enable_opengl" = "xauto"; then
	AC_CHECK_LIB([GL], [glUseProgram],
		[OPENGL_LIBS="-lGL"; libGL=yes],
# On SUSE/OpenSUSE, NVidia places the OpenGL 2.0 capable library in /usr/X11R6/lib
# but it doesn't place a libGL.so there, so the linker won't pick it up
# we have to use the explicit libGL.so.1 path.
		save_LIBS="$LIBS"
		for l in /usr/X11R6/lib /usr/X11R6/lib64; do
			LIBS="$l/libGL.so.1"
			AC_MSG_CHECKING(for glUseProgram in $l/libGL.so.1)
			AC_TRY_LINK([],[extern int glUseProgram(); glUseProgram();],
				[OPENGL_LIBS="$l/libGL.so.1"; libGL=yes],[libGL=no])
			AC_MSG_RESULT([$libGL])
			test $libGL = yes && break
		done
		LIBS="$save_LIBS"
	)
	if test "x$libGL" = "xyes"; then
		enable_opengl=ok
		OPENGL_LIBS="-lGLU $OPENGL_LIBS"
		AC_DEFINE(HAVE_GL)
	fi
else
	if test "x$enable_opengl" = "xyes"; then
		enable_opengl=ok
		libGL=skip
		OPENGL_LIBS="-lGLU -lGL"
		AC_DEFINE(HAVE_GL)
	fi
fi
AC_SUBST(OPENGL_LIBS)
# END OpenGL

############## .png TO .o CONVERSION

AC_CHECK_TOOL(OBJCOPY, objcopy)
if test "x$OBJCOPY" = "x"; then
	AC_MSG_ERROR("objcopy from GNU binutils >= 2.11.90 not found")
fi
AC_CHECK_TOOL(OBJDUMP, objdump)
dnl extract target and architecture if objdump was found
if test "x$OBJDUMP" = "x"; then :; else
	AC_MSG_CHECKING(for object target)
	octarget=`$OBJDUMP --info | sed -ne '2p'` # extract first target
	AC_MSG_RESULT($octarget)
	AC_MSG_CHECKING(for object architecture)
	ocarch=`$OBJDUMP --info | sed -ne '4p'` # extract corresponding arch
	AC_MSG_RESULT($ocarch)
fi
AC_SUBST(OBJCOPYTARGET, $octarget)
AC_SUBST(OBJCOPYARCH, $ocarch)

############## END of .png TO .o CONVERSION

AC_OUTPUT(Makefile cinelerra-cvs-current.spec po/Makefile.in
	m4/Makefile
	guicast/Makefile
	cinelerra/Makefile
	cinelerra/data/Makefile
	plugins/libeffecttv/Makefile
	plugins/Makefile
	plugins/histogram/Makefile
	plugins/C41/Makefile
	plugins/bandslide/Makefile
	plugins/bandwipe/Makefile
	plugins/blur/Makefile
	plugins/bluebanana/Makefile
	plugins/brightness/Makefile
	plugins/burn/Makefile
	plugins/parametric/Makefile
	plugins/aging/Makefile
	plugins/chromakey/Makefile
	plugins/chromakeyhsv/Makefile
	plugins/colorbalance/Makefile
	plugins/colorspace/Makefile
	plugins/compressor/Makefile
	plugins/crossfade/Makefile
	plugins/deinterlace/Makefile
	plugins/delayaudio/Makefile 
	plugins/delayvideo/Makefile
	plugins/denoise/Makefile
	plugins/denoisefft/Makefile
	plugins/denoisevideo/Makefile
	plugins/despike/Makefile
	plugins/dissolve/Makefile
	plugins/dot/Makefile
	plugins/fieldframe/Makefile
	plugins/flip/Makefile
	plugins/framefield/Makefile
	plugins/freeverb/Makefile
	plugins/freezeframe/Makefile
	plugins/gain/Makefile
	plugins/gamma/Makefile
	plugins/holo/Makefile
	plugins/huesaturation/Makefile
	plugins/interpolatevideo/Makefile
	plugins/invertaudio/Makefile
	plugins/invertvideo/Makefile
	plugins/irissquare/Makefile
	plugins/ivtc/Makefile
	plugins/loopaudio/Makefile
	plugins/loopvideo/Makefile
	plugins/motion/Makefile
	plugins/motionblur/Makefile
	plugins/normalize/Makefile
	plugins/nullaudio/Makefile
	plugins/nullaudtrans/Makefile
	plugins/nullrtaudio/Makefile
	plugins/nullvideo/Makefile
	plugins/nullrtvideo/Makefile
	plugins/nullvidtrans/Makefile
	plugins/oilpainting/Makefile
	plugins/overlay/Makefile
	plugins/overlayaudio/Makefile
	plugins/pitch/Makefile
	plugins/polar/Makefile
	plugins/reroute/Makefile
	plugins/reframert/Makefile
	plugins/reverb/Makefile
	plugins/reverseaudio/Makefile
	plugins/reversevideo/Makefile
	plugins/rgb601/Makefile
	plugins/rotate/Makefile
	plugins/scale/Makefile
	plugins/denoiseseltempavg/Makefile
	plugins/shapewipe/Makefile
	plugins/sharpen/Makefile
	plugins/shiftinterlace/Makefile
	plugins/slide/Makefile
	plugins/spectrogram/Makefile
	plugins/suv/Makefile
	plugins/suv/data/Makefile
	plugins/bluedottheme/Makefile
	plugins/bluedottheme/data/Makefile
	plugins/defaulttheme/Makefile
	plugins/defaulttheme/data/Makefile
	plugins/swapchannels/Makefile
	plugins/synthesizer/Makefile
	plugins/threshold/Makefile
	plugins/timeavg/Makefile
	plugins/timestretch/Makefile
	plugins/titler/Makefile
	plugins/translate/Makefile
	plugins/unsharp/Makefile
	plugins/videoscope/Makefile
	plugins/wave/Makefile
	plugins/waveform/Makefile
	plugins/whirl/Makefile
	plugins/wipe/Makefile
	plugins/yuv/Makefile
	plugins/downsample/Makefile
	plugins/flash/Makefile
	plugins/gradient/Makefile
	plugins/level/Makefile
	plugins/linearblur/Makefile
	plugins/perspective/Makefile
	plugins/radialblur/Makefile
	plugins/zoomblur/Makefile
	plugins/timefront/Makefile
	plugins/diffkey/Makefile
	image/Makefile)

succeeded=yes
AC_DEFUN([RPT],[if test "x$$1" = "xskip"; then status="not probed"
elif test "x$$1" = "xyes"; then status=found
else status=missing; succeeded=no
fi
echo "AC_HELP_STRING($2,$status)"])

echo
echo "Summary of mandatory components:"
RPT(libpng,libpng)
RPT(libjpeg,libjpeg libraries)
RPT(libjpegh,libjpeg headers)
RPT(libtiff,libtiff libraries)
RPT(libtiffh,libtiff headers)
RPT(freetype2,FreeType 2)
RPT(libxft,libXft library)
RPT(funiconv,iconv function)
RPT(libuuid,libuuid libraries)
RPT(libuuidh,libuuid headers)
RPT(libfftw3,libfftw3 libraries)
RPT(libfftw3h,libfftw3 headers)
RPT(alsa,ALSA subsystem)
mandatory="$succeeded"

echo
echo "Summary of optional components:"
AC_DEFUN([ORPT],[if test "x$$1" = "xno"; then
	echo "$2 was opted out with $3"
else
	if test "x$$1" = "xok"; then component=enabled; else component=disabled; fi
	$4
	echo "$2 is $component"
fi])
ORPT(enable_esd,ESD (Enlightenment Sound Daemon),--disable-esd,RPT(esound,ESD subsystem))
ORPT(enable_opengl,Hardware acceleration using OpenGL 2.0,--disable-opengl,
	RPT(libGL,OpenGL 2.0 libraries)
)
ORPT(enable_ladspa,Support of LADSPA plugins,--disable-ladspa,RPT(ladspa_h,LADSPA header))
RPT(libfdkaac,Fraunhofer FDK AAC)
RPT(libmp3lame,LAME MP3 Encoder)
RPT(libx264,H.264/MPEG-4 AVC)
RPT(libx265,H.265 Video)
RPT(libvorbis,Ogg Vorbis)
RPT(libtheora,Theora)
RPT(librsvg,Scalable Vector Graphics)
RPT(amrnb,AMR-NB)
RPT(amrwb,AMR-WB)
RPT(libvpx,VP8 and VP9)
RPT(libopus,Opus)

if test "x$mandatory" = "xno"; then
	echo
	echo "WARNING: Mandatory components are missing; compilation may fail!"
else
	echo
	echo "Now type"; echo "          make"; echo; echo "to start compilation."
fi
