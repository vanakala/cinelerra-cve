#!/bin/sh

# autogen.sh for Cinelerra Unofficial CVS

# check for recent automake >= 1.7
amversion=`automake --version | grep "automake (GNU automake)" | cut -b 25-`
echo "checking for automake version... ${amversion:-unknown}"
case "$amversion" in
0.* | 1.[0-6] | 1.[0-6].* )
	echo "You are using a version of automake older than 1.7. That will not work." 1>&2
	echo "See README.BUILD for further instructions." 1>&2
	exit 1
	;;
esac
# automake ok

echo "Running libtoolize ..."
libtoolize --force &&

echo "Running autoheader ..." &&
autoheader &&

echo "Running aclocal -I m4 ..." &&
aclocal -I m4 &&

echo "Running automake ..." &&
automake --foreign --add-missing &&

echo "Running autoconf ..." &&
autoconf &&

echo "Running libtoolize in libsndfile ..."
(cd libsndfile && libtoolize --force) &&

echo "Running autoheader in libsndfile ..." &&
(cd libsndfile && autoheader) &&

echo "Running aclocal in libsndfile ..." &&
(cd libsndfile && aclocal) &&

echo "Running automake in libsndfile ..." &&
(cd libsndfile && automake --foreign --add-missing) &&

echo "Running autoconf in libsndfile ..." &&
(cd libsndfile && autoconf) &&

echo "Finished" 
