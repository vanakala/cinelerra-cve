#!/bin/sh

# autogen.sh for Cinelerra Unofficial CVS  Oct 14, 2003

echo "Running gettextize ..." &&
gettextize --force &&

# Workaround for the 'missing )' gettextize bug
# Ugly but works
sed 's/AM_GNU_GETTEXT_VERSION(\([.0-9]\+\))\?/AM_GNU_GETTEXT_VERSION(\1)/' < configure.in >configure.in.tmp 
mv -f configure.in.tmp configure.in

echo "Running libtoolize ..."
libtoolize --force &&

echo "Running aclocal -I m4 ..." &&
aclocal -I m4 &&

echo "Running automake ..." &&
automake --foreign --add-missing &&

echo "Running autoconf ..." &&
autoconf &&

echo "Finished" 
