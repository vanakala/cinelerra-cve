#!/bin/sh

# autogen.sh for Cinelerra Unofficial CVS  Oct 14, 2003

echo "Running gettextize ..." &&
gettextize --force &&

echo "Running libtoolize ..."
libtoolize --force &&

echo "Running aclocal -I m4 ..." &&
aclocal -I m4 &&

echo "Running automake ..." &&
automake --foreign --add-missing &&

echo "Running autoconf ..." &&
autoconf &&

echo "Finished" 
