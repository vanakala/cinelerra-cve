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
 * Machine-independent I/O routines for IEEE FLOATing-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *	Apple Macintosh, MPW 3.1 C compiler
 *	Apple Macintosh, THINK C compiler
 *	Silicon Graphics IRIS, MIPS compiler
 *	Cray X/MP and Y/MP
 *	Digital Equipment VAX
 *	Sequent Balance (Multiprocesor 386)
 *	NeXT
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * FLOATing-point format, and conversions to and from IEEE single-
 * precision FLOATing-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 *
 * $Id: ieeefloat.c,v 1.1 2003/10/04 00:11:19 herman Exp $
 *
 * $Log: ieeefloat.c,v $
 * Revision 1.1  2003/10/04 00:11:19  herman
 * New file from 1.1.7 (not in 1.1.6) added from Rich's automakified src tree.
 *
 * Revision 1.1  2003/07/29 04:17:52  heroine
 * *** empty log message ***
 *
 * Revision 1.1  1993/06/11  17:45:46  malcolm
 * Initial revision
 *
 */

#include <stdio.h>
#include <math.h>
#include "ieeefloat.h"
#include "common.h"

# define FloatToUnsigned(f)	((unsigned long)(((long)((f) - 2147483648.0)) + 2147483647L + 1))
# define UnsignedToFloat(u)	(((double)((long)((u) - 2147483647L - 1))) + 2147483648.0)

double ConvertFromIeeeExtended (bytes)
     char *bytes;
{
  double f;
  long expon;
  unsigned long hiMant, loMant;

  expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
  hiMant = ((unsigned long) (bytes[2] & 0xFF) << 24)
    | ((unsigned long) (bytes[3] & 0xFF) << 16) |
    ((unsigned long) (bytes[4] & 0xFF) << 8) |
    ((unsigned long) (bytes[5] & 0xFF));
  loMant = ((unsigned long) (bytes[6] & 0xFF) << 24)
    | ((unsigned long) (bytes[7] & 0xFF) << 16) |
    ((unsigned long) (bytes[8] & 0xFF) << 8) |
    ((unsigned long) (bytes[9] & 0xFF));

  if (expon == 0 && hiMant == 0 && loMant == 0) {
    f = 0;
  } else {
    if (expon == 0x7FFF) {	/* Infinity or NaN */
      f = HUGE_VAL;
    } else {
      expon -= 16383;
      f = ldexp (UnsignedToFloat (hiMant), expon -= 31);
      f += ldexp (UnsignedToFloat (loMant), expon -= 32);
    }
  }

  if (bytes[0] & 0x80)
    return -f;
  else
    return f;
}
