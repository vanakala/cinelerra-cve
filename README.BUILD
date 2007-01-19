----------------------------
Library Requirements:
----------------------------

Whilst the official version of Cinelerra (Cinelerra-HV) contain a
custom Makefile, and includes many of the libraries in it, the
community version (Cinelerra-CV) is designed to to customly define the
Makefile specific for your system and use libraries that are / can be
installed into your system as shared libraries.

You need automake version 1.7 to build.  1.4 won't work!
Autoconf 2.57 is also required to build.

The automake version of the cinelerra source tree needs third-party libraries
development files installed. Run 'autoreconf -i' to create configure and
supporting files.

	<x86 CPUs only>
You probably want to enable MMX support.  To do that, run ./configure with
the --enable-mmx option.  NB! If you do that, you may have to use the
--without-pic option, too.  Otherwise, compilation can fail.
	</x86 CPUs only>

For debian user, you can find debian packages of all of these libs on the distribution
itself or at 
	deb ftp://ftp.nerim.net/debian-marillat/ unstable main

(For other architectures/versions of Debian, more info can be found on:
http://hpisi.nerim.net/)

The versions indicated are for information. Cinelerra compiles fine
with these. If you want to use another, don't mail me if it doesn't work.

Some of them are part of every distribution (if they aren't in yours, change
you distrib. Not joking):
        - a52dec
	- alsa libs (>= 0.9)
        - faac
        - faad2
        - fftw
        - lame
        - libavc1394
        - libiec61883
        - libraw1394
        - libsndfile
	- libvorbis (1.0)
	- libogg (1.0)
	- libpng
	- libjpeg	
	- libtiff
	- libesd (esound = 0.2.28)
	- libfreetype (>=2.1.4)
        - mjpegtools
        - OpenEXR
        - x264
	- xlib-dev
	- gettext

Additional requirements for the CVS branch (this source code)
	- automake 1.7
	- autoconf
	- libtool
		
The others are maybe part of your distrib. If they aren't, 
you can find most of them on ftp://ftp.nerim.net/debian-marillat/
and the rest on http://www.kiberpipa.org/~minmax/cinelerra/builds/
(if not, please complain to cinelerra@skolelinux.no!)

----------------------------
Compiler flags:
----------------------------

Note that there are some compiler flags that may optimise the
build for your specific architecture.  Some common examples are:


Pentium-M:

./configure --prefix=/usr --enable-x86 --enable-mmx --enable-freetype2 \ 
            --with-buildinfo=svn/recompile \
 CFLAGS='-O3 -pipe -fomit-frame-pointer -funroll-all-loops -falign-loops=2 \
         -falign-jumps=2 -falign-functions=2 -ffast-math \
         -march=pentium-m -mfpmath=sse,387 -mmmx -msse'

Pentium 4: (include sth D, etc)


AMD 64:


----------------------------
configure flags:
----------------------------

For stamping builds:
  --with-buildinfo=svn
  --with-buildinfo=git
     stamp the About dialog and version info with svn/git version,
     [unsure] flag, and build date
--with-buildinfo=svn/recompile
--with-buildinfo=git/recompile
     as above, but pull the version from svn/git every time make is run.
--with-buildinfo=cust/"SVN r980 SUSE"
     Use a custom string to take the about dialog and version info,
     for pacakgers who use 'make dist' and run configure on the resulting
     tar file.
