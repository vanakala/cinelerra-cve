/**************************************************************************
 *                                                                        *
 * This code has been developed by Andrea Graziani. This software is an   *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php                        *
 *                                                                        *
 **************************************************************************/
/**
*  Copyright (C) 2001 - Project Mayo
 *
 * Andrea Graziani (Ag)
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// debug.c //

#include <stdarg.h>
#include <stdio.h>

#include "mp4_vars.h"

#include "debug.h"

/**
 *
**/

#ifndef _DECORE
#ifdef _DEBUG

static int siPrintPic_start = 0;
static int siPrintPic_end = 0;
static int siPrintMba_start = 0;
static int siPrintMba_end = 0;

/***/

void _SetPrintCond(int picnum_start, int picnum_end, 
									 int mba_start, int mba_end)
{
	siPrintPic_start = picnum_start;
	siPrintPic_end = picnum_end;
	siPrintMba_start = mba_start;
	siPrintMba_end = mba_end;
}

/***
extern FILE *debug_file;

void _Print(const char * format, ...)
{
	if ((mp4_hdr.picnum >= siPrintPic_start) &&
		(mp4_hdr.picnum <= siPrintPic_end)) 
	{
		if ((mp4_hdr.mba >= siPrintMba_start) &&
			(mp4_hdr.mba <= siPrintMba_end)) 
		{
			va_list arglist;
			va_start(arglist, format);
			fprintf(debug_file, format, arglist);
			va_end(arglist);
			fflush(debug_file);
		}
	}
}
***/

void _Print(const char * format, ...)
{
	if ((mp4_state->hdr.picnum >= siPrintPic_start) &&
		(mp4_state->hdr.picnum <= siPrintPic_end)) 
	{
		if ((mp4_state->hdr.mba >= siPrintMba_start) &&
			(mp4_state->hdr.mba <= siPrintMba_end)) 
		{
			va_list arglist;
			va_start(arglist, format);
			vprintf(format, arglist);
			va_end(arglist);
		}
	}
}

void _Break(int picnum, int mba)
{
	if ((mp4_state->hdr.picnum == picnum) && (mp4_state->hdr.mba == mba)) {
		int iBreak = 0;
		// exit(0);
	}
}

void _Error(const char * format, ...)
{
	va_list arglist;
	va_start(arglist, format);
	vprintf(format, arglist);
	va_end(arglist);
}

#endif
#endif
