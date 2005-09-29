#!/bin/sh

# Clean symlinks created with previous versions of automake
rm -f install-sh
rm -f missing
rm -f compile

# autogen.sh for Cinelerra Unofficial CVS
# 
# This script allows you to create a ./configure script
# It is only needed when building from CVS when no ./configure script is provided
#
# WARNING: Recent versions of automake and autoconf are needed.
# You will need at least automake 1.7 and autoconf 2.57.
#
# If the script is not able to locate the needed versions,
# use variables to define absolute paths to the executables.
#
# The available variables are:
# AUTOMAKE=/path/to/automake 
# ACLOCAL=/path/to/aclocal
# AUTOCONF=/path/to/autoconf
# AUTOHEADER=/path/to/autoheader
#
# Example (needed for Debian SID):
#
# export AUTOMAKE=/usr/bin/automake-1.7 
# export ACLOCAL=/usr/bin/aclocal-1.7
# sh autogen.sh

echo "User defined paths to the preferred autoconf and automake versions."
echo "Read the script if you would like to modify them."

if test "$AUTOMAKE" = ""; then
  AUTOMAKE=automake
fi
echo 'AUTOMAKE'"=$AUTOMAKE"

if test "$ACLOCAL" = ""; then
  ACLOCAL=aclocal
fi
echo 'ACLOCAL'=$ACLOCAL

if test "$AUTOCONF" = ""; then
  AUTOCONF=autoconf
fi
echo 'AUTOCONF'=$AUTOCONF

if test "$AUTOHEADER" = ""; then
  AUTOHEADER=autoheader
fi
echo 'AUTOHEADER'=$AUTOHEADER

#
# You should not be modifying anything from here
#

echo "Now building the ./configure script..."

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

echo "Finished" 
