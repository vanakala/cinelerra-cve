----------------------------
Library Requirements:
----------------------------

Whilst the official version of Cinelerra (Cinelerra-HV) contain a
custom Makefile, and includes many of the libraries in it, the
community version (Cinelerra-CV) is designed to to customly define the
Makefile specific for your system and use libraries that are / can be
installed into your system as shared libraries.

You need autoconf 2.57 or later to build.

The versions indicated are for information. Cinelerra compiles fine
with these. If you want to use another, don't mail me if it doesn't work.

Some of them are part of every distribution (if they aren't in yours, change
you distrib. Not joking):
	- freetype2
	- fontconfig
	- libXft
	- fftw
	- alsa libs (>= 0.9)
	- libpng
	- libjpeg
	- libtiff
	- libesd (esound = 0.2.28)
	- OpenEXR
	- xlib-dev
	- gettext
	- ffmpeg libraries > 2.8 (optional, configure may download suitable sources)

Additional requirements for the CVS branch (this source code)
	- automake 1.7
	- autoconf
	- libtool
	- intltoolize


----------------------------
Compiling process
----------------------------

	cd <cinellera-src>
	./autogen.sh
	./configure
	make
	sudo make install

Configure downloads the latest suitable sourses from FFmpeg site
