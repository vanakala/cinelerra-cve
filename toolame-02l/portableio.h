#ifndef PORTABLEIO_H__
#define PORTABLEIO_H__
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
 * Machine-independent I/O routines for 8-, 16-, 24-, and 32-bit integers.
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
 * FLOATing-point format, and conversions to and from IEEE single-
 * precision FLOATing-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 *
 * $Id: portableio.h,v 1.1 2003/10/04 00:11:19 herman Exp $
 *
 * $Log: portableio.h,v $
 * Revision 1.1  2003/10/04 00:11:19  herman
 * New file from 1.1.7 (not in 1.1.6) added from Rich's automakified src tree.
 *
 * Revision 1.1  2003/07/29 04:17:52  heroine
 * *** empty log message ***
 *
 * Revision 2.6  91/04/30  17:06:02  malcolm
 */

#include	<stdio.h>
#include	"ieeefloat.h"

int ReadByte (FILE * fp);
int Read16BitsLowHigh (FILE * fp);
int Read16BitsHighLow (FILE * fp);
void Write8Bits (FILE * fp, int i);
void Write16BitsLowHigh (FILE * fp, int i);
void Write16BitsHighLow (FILE * fp, int i);
int Read24BitsHighLow (FILE * fp);
int Read32Bits (FILE * fp);
int Read32BitsHighLow (FILE * fp);
void Write32Bits (FILE * fp, int i);
void Write32BitsLowHigh (FILE * fp, int i);
void Write32BitsHighLow (FILE * fp, int i);
void ReadBytes (FILE * fp, char *p, int n);
void ReadBytesSwapped (FILE * fp, char *p, int n);
void WriteBytes (FILE * fp, char *p, int n);
void WriteBytesSwapped (FILE * fp, char *p, int n);
double ReadIeeeFloatHighLow (FILE * fp);
double ReadIeeeFloatLowHigh (FILE * fp);
double ReadIeeeDoubleHighLow (FILE * fp);
double ReadIeeeDoubleLowHigh (FILE * fp);
double ReadIeeeExtendedHighLow (FILE * fp);
double ReadIeeeExtendedLowHigh (FILE * fp);
void WriteIeeeFloatLowHigh (FILE * fp, double num);
void WriteIeeeFloatHighLow (FILE * fp, double num);
void WriteIeeeDoubleLowHigh (FILE * fp, double num);
void WriteIeeeDoubleHighLow (FILE * fp, double num);
void WriteIeeeExtendedLowHigh (FILE * fp, double num);
void WriteIeeeExtendedHighLow (FILE * fp, double num);

#define	Read32BitsLowHigh(f)	Read32Bits(f)
#define WriteString(f,s)	fwrite(s,strlen(s),sizeof(char),f)
#endif
