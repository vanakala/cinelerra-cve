/* Copyright (C) 1988-1991 Apple Computer, Inc.
 * All Rights Reserved.
 *
 * Warranty Information
 * Even though Apple has reviewed this software, Apple makes no warranty
 * or representation, either express or implied, with respect to this
 * software, its quality, accuracy, merchantability, or fitness for a 
 * particular purpose.  As a result, this software is provided "as is,"
 * and you, its user, are assuming the entire risk as to its quality
 * and accuracy.
 *
 * This code may be used and freely distributed as long as it includes
 * this copyright notice and the warranty information.
 *
 *
 * Motorola processors (Macintosh, Sun, Sparc, MIPS, etc)
 * pack bytes from high to low (they are big-endian).
 * Use the HighLow routines to match the native format
 * of these machines.
 *
 * Intel-like machines (PCs, Sequent)
 * pack bytes from low to high (the are little-endian).
 * Use the LowHigh routines to match the native format
 * of these machines.
 *
 * These routines have been tested on the following machines:
 *	Apple Macintosh, MPW 3.1 C compiler
 *	Apple Macintosh, THINK C compiler
 *	Silicon Graphics IRIS, MIPS compiler
 *	Cray X/MP and Y/MP
 *	Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 *
 * $Id: portableio.c,v 1.1 2003/06/16 20:00:50 herman Exp $
 *
 * $Log: portableio.c,v $
 * Revision 1.1  2003/06/16 20:00:50  herman
 * Initial revision
 *
 * Revision 1.1.1.1  2002/06/21 12:35:26  myrina
 *
 *
 * Revision 1.1.1.1  2001/10/01 03:14:55  heroine
 *
 *
 * Revision 1.1.1.1  2001/10/01 02:51:42  root
 *
 *
 * Revision 1.5  2001/07/15 12:16:31  mikecheng
 * Removed a whooooole heap of unused functions
 *
 * Revision 1.4  2001/07/14 06:17:20  mikecheng
 * More cleanup.  psy.c init and cleanup functions added.
 * Removed 'II_' from in front of all function names.
 * absthr table files now in exe.
 * Started renaming some variables in some sort of scheme (heaps more to come)
 *
 * Revision 1.3  2001/07/04 09:51:36  uid43892
 * everything passed through 'indent *.c *.h'
 *
 * Revision 1.2  2001/07/02 10:35:28  mikecheng
 * Function args changed from K&R -> ansi.
 * Cleaned up the header files.
 * Made compilation pretty strict (-Wall -ansi -pedantic)
 *
 * Revision 1.1.1.1  2001/07/01 06:54:47  mikecheng
 * This is v0.30 of toolame. A fresh start consisting of the dist10 code with
 * all layerI and layerIII removed.
 *
 * Revision 2.6  91/04/30  17:06:02  malcolm
 */

#include <stdio.h>
#include <math.h>
#include "portableio.h"
#include "common.h"

/****************************************************************
 * Big/little-endian independent I/O routines.
 ****************************************************************/
int
Read16BitsHighLow (fp)
     FILE *fp;
{
  int first, second, result;

  first = 0xff & getc (fp);
  second = 0xff & getc (fp);

  result = (first << 8) + second;
#ifndef	THINK_C42
  if (result & 0x8000)
    result = result - 0x10000;
#endif /* THINK_C */
  return (result);
}

int
Read32BitsHighLow (fp)
     FILE *fp;
{
  int first, second, result;

  first = 0xffff & Read16BitsHighLow (fp);
  second = 0xffff & Read16BitsHighLow (fp);

  result = (first << 16) + second;
#ifdef	CRAY
  if (result & 0x80000000)
    result = result - 0x100000000;
#endif
  return (result);
}

defdouble
ReadIeeeExtendedHighLow (fp)
     FILE *fp;
{
  char bits[kExtendedLength];

  ReadBytes (fp, bits, kExtendedLength);
  return ConvertFromIeeeExtended (bits);
}

void
ReadBytes (fp, p, n)
     FILE *fp;
     char *p;
     int n;
{
  while ((!feof (fp) & (n--)) > 0)
    *p++ = getc (fp);
}
