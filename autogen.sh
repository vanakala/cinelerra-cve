#!/bin/sh

# autogen.sh for Cinelerra-CV official GIT
#
# This script allows you to create a ./configure script
# It is only needed when building from GIT when no ./configure script is provided
#
# Make sure that cleans the files created with autotools before run autoreconf.
rm -f aclocal.m4
rm -rf autom4te.cache/
rm -f compile
rm -f config.guess
rm -f config.h.in
rm -f config.sub
rm -f configure
rm -f depcomp
rm -f install-sh
rm -f ltmain.sh
rm -f m4/intltool.m4
rm -f m4/libtool.m4
rm -f m4/ltoptions.m4
rm -f m4/ltsugar.m4
rm -f m4/ltversion.m4
rm -f m4/lt~obsolete.m4
rm -f missing
rm -f po/Makefile.in.in

autoreconf --install
intltoolize
