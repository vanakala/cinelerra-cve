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
// mp4_iquant.c //

#include "mp4_vars.h"

#include "mp4_predict.h"
#include "mp4_iquant.h"

/**
 *	inverse quantization for intra blocks
**/

#define _iquant_h263(coeff, q_2scale, q_add) \
if ((coeff) > 0) \
{	\
	(coeff) = ((q_2scale) * (coeff)) + (q_add);	\
}	\
else \
if ((coeff) < 0)\
{	\
	(coeff) *= -1;	\
	(coeff) = ((q_2scale) * (coeff)) + (q_add);	\
	(coeff) *= -1; \
}	\

/**
 *
**/

// iquant type h.263, not optimized
// intraFlag is 0 or 1
__inline void iquant (short * psblock, int intraFlag)
{
	int i;
	int q_scale = mp4_state->hdr.quantizer;
	int q_2scale = q_scale << 1;
	int q_add = (q_scale & 1) ? q_scale : (q_scale - 1);
/*
 * static int count = 0;
 * count++;
 * if(!(count % 1000))
 * {
 * 	printf("iquant %d\n", count);
 * }
 */

	if(intraFlag)
		for (i = 1; i < 64; i++)
		{
			_iquant_h263(psblock[i], q_2scale, q_add);
		}
	else
		for (i = 0; i < 64; i++)
		{
			_iquant_h263(psblock[i], q_2scale, q_add);
		}
}


void iquant_typefirst (short * psblock)
{
	int i;
/*
 * static int count = 0;
 * count++;
 * if(!(count % 1000))
 * {
 * 	printf("iquant_typefirst %d\n", count);
 * }
 */

	for (i = 1; i < 64; i++)
	{
		if (psblock[i] != 0) {
			psblock[i] = (psblock[i] * 2 *	mp4_state->hdr.quantizer * 
				mp4_tables->intra_quant_matrix[mp4_tables->zig_zag_scan[i]]) >> 4;
		}
	}
}


