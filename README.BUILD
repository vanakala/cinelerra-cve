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
	- alsa libs (>= 0.9)
	- libvorbis (1.0)
	- libogg (1.0)
	- libpng
	- libjpeg	
	- libtiff
	- libesd (esound = 0.2.28)
	- libfreetype (>=2.1.4)
	- xlib-dev
		
The others are maybe part of your distrib. If they aren't, you can find them
on http://lpnotfr.free.fr/cinelerra-libs
	- libdv (0.99)
	- xvid (cvs 20020822)
	- liba52 (0.7.4)
	- libsndfile - both 0.x.x and 1.x.x versions should work
	- libaudiofile (0.2.3)
	- libraw1394 (0.9.0)
	- libavc1394 (0.4.1)
	- liblame-dev
	- libavcodec
	- libavcodec-dev
	- libuuid
	- nasm (for mmx optimization)
