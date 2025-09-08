#!/bin/sh

# autogen.sh for Cinelerra-CV official GIT
#
# This script allows you to create a ./configure script
# It is only needed when building from GIT when no ./configure script is provided
#
# use system provided m4 files from $sysaclocal directory
# The files are hopefully provided by development packages
sysaclocal=/usr/share/aclocal

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
for nam in alsa.m4 iconv.m4 lib-link.m4 lib-prefix.m4
do
    ln -s $sysaclocal/$nam m4/$nam
done
touch config.rpath
autoupdate configure.ac
autoreconf --install
intltoolize -f
