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
 * Jonathan White
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// mp4_decoder.c //

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#endif

#include "mp4_vars.h"
#include "decore.h"

/**
 *
**/

/***/

void initdecoder (DEC_BUFFERS buffers)
{
  int i, j, cc;

	save_tables(mp4_tables);

  mp4_state->clp = mp4_state->clp_data + 384;
  for (i = -384; i < 640; i++)
    mp4_state->clp[i] = (unsigned char) ( (i < 0) ? 0 : ((i > 255) ? 255 : i) );
 
	/* dc prediction border */
	for (i = 0; i < (2*DEC_MBC+1); i++)
		mp4_state->coeff_pred.dc_store_lum[0][i] = 1024;

	for (i = 1; i < (2*DEC_MBR+1); i++)
		mp4_state->coeff_pred.dc_store_lum[i][0] = 1024;

	for (i = 0; i < (DEC_MBC+1); i++) {
		mp4_state->coeff_pred.dc_store_chr[0][0][i] = 1024;
		mp4_state->coeff_pred.dc_store_chr[1][0][i] = 1024;
	}

	for (i = 1; i < (DEC_MBR+1); i++) {
		mp4_state->coeff_pred.dc_store_chr[0][i][0] = 1024;
		mp4_state->coeff_pred.dc_store_chr[1][i][0] = 1024;
	}

	/* ac prediction border */
	for (i = 0; i < (2*DEC_MBC+1); i++)
		for (j = 0; j < 7; j++)	{
			mp4_state->coeff_pred.ac_left_lum[0][i][j] = 0;
			mp4_state->coeff_pred.ac_top_lum[0][i][j] = 0;
		}

	for (i = 1; i < (2*DEC_MBR+1); i++)
		for (j = 0; j < 7; j++)	{
			mp4_state->coeff_pred.ac_left_lum[i][0][j] = 0;
			mp4_state->coeff_pred.ac_top_lum[i][0][j] = 0;
		}

	/* 
			[Review] too many computation to access to the 
			correct array value, better use two different
			pointer for Cb and Cr components
	*/
	for (i = 0; i < (DEC_MBC+1); i++)
		for (j = 0; j < 7; j++)	{
			mp4_state->coeff_pred.ac_left_chr[0][0][i][j] = 0; 
			mp4_state->coeff_pred.ac_top_chr[0][0][i][j] = 0;
			mp4_state->coeff_pred.ac_left_chr[1][0][i][j] = 0;
			mp4_state->coeff_pred.ac_top_chr[1][0][i][j] = 0;
		}

	for (i = 1; i < (DEC_MBR+1); i++)
		for (j = 0; j < 7; j++)	{
			mp4_state->coeff_pred.ac_left_chr[0][i][0][j] = 0;
			mp4_state->coeff_pred.ac_top_chr[0][i][0][j] = 0;
			mp4_state->coeff_pred.ac_left_chr[1][i][0][j] = 0;
			mp4_state->coeff_pred.ac_top_chr[1][i][0][j] = 0;
		}

	/* mode border */
	for (i = 0; i < mp4_state->mb_width + 1; i++)
		mp4_state->modemap[0][i] = INTRA;
	for (i = 0; i < mp4_state->mb_height + 1; i++) {
		mp4_state->modemap[i][0] = INTRA;
		mp4_state->modemap[i][mp4_state->mb_width+1] = INTRA;
	}

	// edged forward and reference frame
  for (cc = 0; cc < 3; cc++)
  {
    if (cc == 0)
    {
			edged_ref[cc] = (unsigned char *) buffers.mp4_edged_ref_buffers;
			assert(edged_ref[cc]);

			edged_for[cc] = (unsigned char *) buffers.mp4_edged_for_buffers;
			assert(edged_for[cc]);

      frame_ref[cc] = edged_ref[cc] + mp4_state->coded_picture_width * 32 + 32;
      frame_for[cc] = edged_for[cc] + mp4_state->coded_picture_width * 32 + 32;
    } 
    else
    {
			unsigned int offset;

			if (cc == 1) 
				offset = mp4_state->coded_picture_width * mp4_state->coded_picture_height;
			else 
				offset = mp4_state->coded_picture_width * mp4_state->coded_picture_height +
	  					mp4_state->chrom_width * mp4_state->chrom_height;

			edged_ref[cc] = (unsigned char *) buffers.mp4_edged_ref_buffers + offset;
			assert(edged_ref[cc]);

			edged_for[cc] = (unsigned char *) buffers.mp4_edged_for_buffers + offset;
			assert(edged_for[cc]);

      frame_ref[cc] = edged_ref[cc] + mp4_state->chrom_width * 16 + 16;
      frame_for[cc] = edged_for[cc] + mp4_state->chrom_width * 16 + 16;
    }
  }

	// display frame
	for (cc = 0; cc < 3; cc++) 
	{
		unsigned int offset;

		switch (cc) 
		{
		case 0:
			offset = 0;
			break;
		case 1:
			offset = mp4_state->horizontal_size * mp4_state->vertical_size;
			break;
		case 2:
			offset = (mp4_state->horizontal_size * mp4_state->vertical_size) + 
				((mp4_state->horizontal_size * mp4_state->vertical_size) >> 2);
			break;
		}
		
		display_frame[cc] = (unsigned char *) buffers.mp4_display_buffers + offset;
		assert(display_frame[cc]);
	}




}

/***/

void closedecoder ()
{
	/*** REVIEW

	int cc;

	clp -= 384;
	free(clp);

	for (cc = 0; cc < 3; cc++) {
		if(display_frame[cc]) free(display_frame[cc]);
		display_frame[cc] = 0;
		if(edged_ref[cc]) free(edged_ref[cc]);
		edged_ref[cc] = 0;
		if(edged_for[cc]) free(edged_for[cc]);
		edged_for[cc] = 0;
	}

	***/
}
