# Copyright (C) 1999-2002 Erik de Castro Lopo (erikd AT zip DOT com DOT au).

dnl Require autoconf version
AC_PREREQ(2.54)

AC_INIT
AC_CONFIG_SRCDIR([src/sndfile.c])
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE(libsndfile,1.0.5)
AM_CONFIG_HEADER(src/config.h)

#------------------------------------------------------------------------------------
# This is the version info according to the libtool versioning system.

SHARED_VERSION_INFO="1:5:0"

AC_CHECK_PROG(autogen, autogen, yes, no)

AC_PROG_CC
AC_PROG_INSTALL

AC_PROG_MAKE_SET
AC_PROG_LN_S
AM_PROG_LIBTOOL

AC_HEADER_STDC

AC_CHECK_HEADERS(endian.h)
AC_CHECK_HEADERS(stdint.h)

#====================================================================================
# Couple of initializations here. Fill in real values later.

SHLIB_VERSION_ARG=""

#====================================================================================
# Finished checking, handle options.

AC_ARG_ENABLE(experimental,
		[  --enable-experimental        enable experimental code ],
		ac_arg_experimental=yes, ac_arg_experimental=no)

EXPERIMENTAL_CODE=0
if test x$ac_arg_experimental = xyes ; then
	EXPERIMENTAL_CODE=1
	fi
AC_DEFINE_UNQUOTED([ENABLE_EXPERIMENTAL_CODE],${EXPERIMENTAL_CODE}, [Set to 1 to enable experimental code.])

AC_ARG_ENABLE(gcc-pipe, 
		[  --disable-gcc-pipe           disable gcc -pipe option ],
		ac_arg_gcc_pipe="N", ac_arg_gcc_pipe="Y")

AC_ARG_ENABLE(gcc-opt,  
		[  --disable-gcc-opt            disable gcc optimisations ],
		ac_arg_gcc_opt="N", ac_arg_gcc_opt="Y")

if test x$ac_cv_c_compiler_gnu = xyes ; then
	CFLAGS="$CFLAGS -W -Wall -pedantic -Wstrict-prototypes -Wmissing-prototypes -Waggregate-return -Wcast-align -Wcast-qual -Wnested-externs -Wshadow -Wbad-function-cast"
	# -Wpointer-arith -Wundef -Wmissing-declarations -Winline -Wconversion"
	if test "$ac_arg_gcc_opt" = "N" ; then
		temp_CFLAGS=`echo $CFLAGS | sed "s/O2/O0/"`
		CFLAGS=$temp_CFLAGS
		AC_MSG_WARN([[*** Compiler optimisations switched off. ***]])
		fi

	# Disable -Wall and -pedantic for Apple Darwin/Rhapsody.
	# System headers on these systems are broken.
	case "$target_os" in
		darwin* | rhapsody*)
			temp_CFLAGS=`echo $CFLAGS | sed "s/-Wall -pedantic//"`
			CFLAGS=$temp_CFLAGS
			;;
		linux*)
			SHLIB_VERSION_ARG="-Wl,--version-script=Version_script"
			;;
		*)
			;;
		esac
	if test $ac_arg_gcc_pipe != "N" ; then
	 	CFLAGS="$CFLAGS -pipe"
	 	fi

	AC_DEFINE([COMPILER_IS_GCC],1, [Set to 1 if the compile is GNU GCC.])
	GCC_MAJOR_VERSION=`$CC -dumpversion | sed "s/\..*//"`
	AC_DEFINE_UNQUOTED([GCC_MAJOR_VERSION],${GCC_MAJOR_VERSION}, [Major version of GCC or 3 otherwise.])
	fi

#====================================================================================
# Check types and their sizes.

AC_CHECK_SIZEOF(short,2)
AC_CHECK_SIZEOF(int,4)
AC_CHECK_SIZEOF(long,8)
AC_CHECK_SIZEOF(float,4)
AC_CHECK_SIZEOF(double,4)
AC_CHECK_SIZEOF(void*,8)
AC_CHECK_SIZEOF(size_t,4)

#====================================================================================
# Find an appropriate type for sf_count_t.
# On systems supporting files larger than 2 Gig, sf_count_t must be a 64 bit value.
# Unfortunately there is more than one way of ensuring this so need to do some
# pretty rigourous testing here.

unset ac_cv_sizeof_off_t

AC_CHECK_SIZEOF(off_t,1)	# Fake default value.

if test "x$ac_cv_sizeof_off_t" = "x8" ; then
	# If sizeof (off_t) is 8, no further checking is needed.
	TYPEOF_SF_COUNT_T="off_t"
	SIZEOF_SF_COUNT_T=8
else
	# Check for common 64 bit file offset types.
	AC_CHECK_SIZEOF(loff_t,1)	# Fake default value.
	AC_CHECK_SIZEOF(off64_t,1)	# Fake default value.

	TYPEOF_SF_COUNT_T="unknown"
	if test "x$ac_cv_sizeof_loff_t" = "x8" ; then
		TYPEOF_SF_COUNT_T="loff_t"
		SIZEOF_SF_COUNT_T=8
	elif test "x$ac_cv_sizeof_off64_t" = "x8" ; then
		TYPEOF_SF_COUNT_T="off64_t"
		SIZEOF_SF_COUNT_T=8
		fi

	# Save the old sizeof (off_t) value  and then unset it to see if it
	# changes when Large File Support is enabled.

	pre_largefile_sizeof_off_t=$ac_cv_sizeof_off_t
	unset ac_cv_sizeof_off_t

	AC_SYS_EXTRA_LARGEFILE

	if test "x$ac_cv_sys_largefile_CFLAGS" = "xno" ; then
		ac_cv_sys_largefile_CFLAGS=""
		fi
	if test "x$ac_cv_sys_largefile_LDFLAGS" = "xno" ; then
		ac_cv_sys_largefile_LDFLAGS=""
		fi
	if test "x$ac_cv_sys_largefile_LIBS" = "xno" ; then
		ac_cv_sys_largefile_LIBS=""
		fi

	AC_CHECK_SIZEOF(off_t,1)	# Fake default value.

	if test "x$ac_cv_sizeof_off_t" = "x$pre_largefile_sizeof_off_t" ; then
		AC_MSG_WARN([[This machine does not seem to support 64 bit file offsets.]])
		TYPEOF_SF_COUNT_T="off_t"
		SIZEOF_SF_COUNT_T=$ac_cv_sizeof_off_t
	elif test "x$TYPEOF_SF_COUNT_T" = "xunknown" ; then
		echo
		echo "*** The configure process has determined that this system is capable"
		echo "*** of Large File Support but has not been able to find a type which"
		echo "*** is an unambiguous 64 bit file offset."
		echo "*** Please contact the author to help resolve this problem."
		echo
		AC_MSG_ERROR([[Bad file offset type.]])
		fi
	fi

AC_DEFINE_UNQUOTED([TYPEOF_SF_COUNT_T],${TYPEOF_SF_COUNT_T}, [Set to long if unknown.])
AC_SUBST(TYPEOF_SF_COUNT_T)

AC_DEFINE_UNQUOTED([SIZEOF_SF_COUNT_T],${SIZEOF_SF_COUNT_T}, [Set to sizeof (long) if unknown.])
AC_SUBST(SIZEOF_SF_COUNT_T)

#====================================================================================
# Determine endian-ness of target processor.

AC_C_FIND_ENDIAN

AC_DEFINE_UNQUOTED(CPU_IS_BIG_ENDIAN, ${ac_cv_c_big_endian},
	[Target processor is big endian.])
AC_DEFINE_UNQUOTED(CPU_IS_LITTLE_ENDIAN, ${ac_cv_c_little_endian},
	[Target processor is little endian.])

#====================================================================================
# Check for functions.

AC_CHECK_FUNCS(malloc calloc realloc free)
AC_CHECK_FUNCS(open read write lseek fstat ftruncate pread pwrite)
AC_CHECK_FUNCS(snprintf vsnprintf)
AC_CHECK_FUNCS(gmtime gmtime_r)

AC_CHECK_LIB([m],floor)
AC_CHECK_FUNCS(floor ceil fmod)

case "$target_os" in
	darwin* | rhapsody*)
		AC_MSG_WARN([[Not using native lrint() and lrintf() because they are missing on 10.1.]])
		;;
	*)
		AC_C99_FUNC_LRINT
		AC_C99_FUNC_LRINTF

		if test x$ac_cv_c99_lrint = xno || test x$ac_cv_c99_lrintf = xno ; then
			AC_MSG_WARN([[*** Missing C99 standard functions lrint() and lrintf().]])
			AC_MSG_WARN([[*** This may cause benign compiler warnings on some systems (ie Solaris).]])
			fi
		;;
	esac

#====================================================================================
# Find known target OS.

OS_SPECIFIC_INCLUDES=""

case "$target_os" in
	darwin* | rhapsody*)
		OS_SPECIFIC_INCLUDES="-fpascal-strings -I/Developer/Headers/FlatCarbon"
		OS_SPECIFIC_LINKS="-framework CoreAudio"
		;;
	*)
		OS_SPECIFIC_INCLUDES=""
		OS_SPECIFIC_LINKS=""
		;;
	esac

#====================================================================================
# Test for sanity when cross-compiling.

if test $cross_compiling = yes ; then
	AC_MSG_WARN([[******************************************************************]])
	AC_MSG_WARN([[*** We are cross-compiling, so have to assume sizeof (short) == 2 ]])
	AC_MSG_WARN([[*** and sizeof (int) == 4. If this is not the case there is no    ]])
	AC_MSG_WARN([[*** chance of this working. Please contact the mantainer.         ]])
	AC_MSG_WARN([[******************************************************************]])
	fi

if test $ac_cv_sizeof_short != 2 ; then
	AC_MSG_WARN([[******************************************************************]])
	AC_MSG_WARN([[*** sizeof (short) != 2.                                          ]])
	AC_MSG_WARN([[******************************************************************]])
	fi

if test $ac_cv_sizeof_int != 4 ; then
	AC_MSG_WARN([[******************************************************************]])
	AC_MSG_WARN([[*** sizeof (int) != 4                                             ]])
	AC_MSG_WARN([[******************************************************************]])
	fi

if test $ac_cv_sizeof_float != 4 ; then
	AC_MSG_WARN([[******************************************************************]])
	AC_MSG_WARN([[*** sizeof (float) != 4.                                          ]])
	AC_MSG_WARN([[******************************************************************]])
	fi

if test $ac_cv_sizeof_double != 8 ; then
	AC_MSG_WARN([[******************************************************************]])
	AC_MSG_WARN([[*** sizeof (double) != 8.                                         ]])
	AC_MSG_WARN([[******************************************************************]])
	fi

if test x"$ac_cv_prog_autogen" = "xno" ; then
	AC_MSG_WARN([[Touching files in directory tests/.]])
	touch tests/*.[ch]
	fi

#====================================================================================
# Now use the information from the checking stage.

AC_DEFINE([OS_IS_WIN32],0, [Set to 1 if compiling for Win32])

if test x"$CFLAGS" = x ; then
	echo "Error in configure script. CFLAGS has been screwed up."
	exit
	fi

# if test x$ac_cv_c_compiler_gnu = xyes ; then
# 	CFLAGS="$CFLAGS -Werror"
# 	fi

AC_SUBST(SHLIB_VERSION_ARG)
AC_SUBST(SHARED_VERSION_INFO)
AC_SUBST(OS_SPECIFIC_INCLUDES)
AC_SUBST(OS_SPECIFIC_LINKS)
AC_SUBST(ENABLE_EXPERIMENTAL_CODE)

AC_SUBST(COMPILER_IS_GCC)
AC_SUBST(GCC_MAJOR_VERSION)

AC_CONFIG_FILES([Makefile src/sndfile.h src/Version_script \
			src/Makefile src/GSM610/Makefile src/G72x/Makefile \
			man/Makefile examples/Makefile tests/Makefile doc/Makefile \
			Win32/Makefile MacOS/Makefile Octave/Makefile \
			libsndfile.spec sndfile.pc])
AC_OUTPUT

#====================================================================================

AC_MSG_RESULT([
-=-=-=-=-=-=-=-=-=-= Configuration Complete =-=-=-=-=-=-=-=-=-=-

  Configuration summary :

    Version : ..................... ${VERSION}
    Experimental code : ........... ${ac_arg_experimental}
])

if test x$ac_cv_c_compiler_gnu = xyes ; then
	echo -e "  Tools :\n"
	echo "    Compiler is GCC : ............. ${ac_cv_c_compiler_gnu}"
	echo "    GCC major version : ........... ${GCC_MAJOR_VERSION}"
	if test $GCC_MAJOR_VERSION -lt 3 ; then
		echo -e "\n    ** This compiler version allows applications to write "
		echo "    ** to static strings within the library."
		echo "    ** Compile with GCC version 3.X to avoid this problem."
		fi
	fi

AC_MSG_RESULT([
  Installation directories :

    Library directory : ........... ${prefix}/lib
    Program directory : ........... ${prefix}/bin
    Pkgconfig directory : ......... ${prefix}/lib/pkgconfig
])

if test x$prefix != "x/usr" ; then
	echo "Compiling some other packages against libsndfile may require "
	echo -e "the addition of \"${prefix}/lib/pkgconfig\" to the "
	echo -e "PKG_CONFIG_PATH environment variable.\n"
	fi

