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
// global.h //

/**
 *	This file is not longer needed !!!
**/

/* GLOBAL is defined in only one file */

#include <stdio.h>

#ifndef GLOBAL
#define EXTERN extern
#else
#define EXTERN
#endif


/**
 *	mp4 stuff
**/

#include "mp4_header.h"

EXTERN int	juice_flag, output_flag;
EXTERN char * outputname;
EXTERN int  flag_invert;
EXTERN int	post_flag, pp_options;
EXTERN mp4_header mp4_hdr;
EXTERN struct _base
{
  // bit input
  int infile;
  unsigned char rdbfr[2051];
  unsigned char *rdptr;
  unsigned char inbfr[16];
  int incnt;
  int bitcnt;
  // block data
  short block[6][64];
} base, *ld;
EXTERN int MV[2][6][MBR+1][MBC+2];
EXTERN int modemap[MBR+1][MBC+2];
EXTERN int quant_store[MBR+1][MBC+1]; // [Review]
EXTERN struct _ac_dc
{
	int dc_store_lum[2*MBR+1][2*MBC+1];
	int ac_left_lum[2*MBR+1][2*MBC+1][7];
	int ac_top_lum[2*MBR+1][2*MBC+1][7];

	int dc_store_chr[2][MBR+1][MBC+1];
	int ac_left_chr[2][MBR+1][MBC+1][7];
	int ac_top_chr[2][MBR+1][MBC+1][7];

	int predict_dir;

} ac_dc;

EXTERN unsigned char	*edged_ref[3],
						*edged_for[3],
						*frame_ref[3],
						*frame_for[3],
						*display_frame[3];
