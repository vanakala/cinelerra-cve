#!/bin/sh

# autogen.sh for Cinelerra Unofficial CVS
# This script allows you to create a ./configure script#
# Please refer to README.BUILD for the needed versions and dependencies

# Automake wrappers
AUTOMAKE=automake
ACLOCAL=aclocal

# Autoconf wrappers
AUTOCONF=autoconf
AUTOHEADER=autoheader

#
# You should not be modifying anything from here
#

# check for recent automake >= 1.7
amversion=`$AUTOMAKE --version | grep "automake (GNU automake)" | cut -b 25-`
echo "checking for automake version... ${amversion:-unknown}"
case "$amversion" in
0.* | 1.[0-6] | 1.[0-6].* | 1.[0-6]-* )
	echo "Error: an old version of automake was detected. You need at least automake 1.7 or a newer version." 1>&2
        echo "If several versions of automake are installed on your system, you can modify the AUTOMAKE wrapper." 1>&2
	echo "For example: AUTOMAKE=/usr/bin/automake-1.7" 1>&2
        exit 1
	;;
esac
# automake ok

# check for recent aclocal >= 1.7
acversion=`$ACLOCAL --version | grep "aclocal (GNU automake)" | cut -b 24-`
echo "checking for aclocal version... ${acversion:-unknown}"
case "$acversion" in
0.* | 1.[0-6] | 1.[0-6].* | 1.[0-6]-* )
	echo "Error: and old version of aclocal was detected. You need at least aclocal 1.7 or a newer version." 1>&2
	echo "usually, automake and aclocal are part of the same Automake package, they should be of the same version." 1>&2
        echo "If several versions of aclocal are installed on your system, you can modify the ACLOCAL wrapper." 1>&2
	echo "For example: ACLOCAL=/usr/bin/aclocal-1.7" 1>&2
        exit 1
	;;
esac
# automake ok

echo "Running aclocal -I m4 ..." &&
$ACLOCAL -I m4 &&

echo "Running libtoolize ..."
libtoolize --force &&

echo "Running autoheader ..." &&
$AUTOHEADER &&

echo "Running automake ..." &&
$AUTOMAKE --foreign --add-missing &&

echo "Running autoconf ..." &&
$AUTOCONF &&

echo "Running aclocal in libsndfile ..." &&
(cd libsndfile && $ACLOCAL) &&

echo "Running libtoolize in libsndfile ..."
(cd libsndfile && libtoolize --force) &&

echo "Running autoheader in libsndfile ..." &&
(cd libsndfile && $AUTOHEADER) &&

echo "Running automake in libsndfile ..." &&
(cd libsndfile && $AUTOMAKE --foreign --add-missing) &&

echo "Running autoconf in libsndfile ..." &&
(cd libsndfile && $AUTOCONF) &&

echo "Finished" 
