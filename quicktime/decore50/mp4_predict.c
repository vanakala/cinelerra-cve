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
// mp4_predict.c //

#include <math.h>

#include "mp4_vars.h"

#include "mp4_predict.h"

/**
 *
**/

static void rescue_predict();

/*

	B - C
	|   |
	A - x

*/

void dc_recon(int block_num, short * dc_value)
{
	if (mp4_state->hdr.prediction_type == P_VOP) {
		rescue_predict();
	}

	if (block_num < 4)
	{
		int b_xpos = (mp4_state->hdr.mb_xpos << 1) + (block_num & 1);
		int b_ypos = (mp4_state->hdr.mb_ypos << 1) + ((block_num & 2) >> 1);
		int dc_pred;

		// set prediction direction
		if (abs(mp4_state->coeff_pred.dc_store_lum[b_ypos+1-1][b_xpos+1-1] -
			mp4_state->coeff_pred.dc_store_lum[b_ypos+1][b_xpos+1-1]) < // Fa - Fb
			abs(mp4_state->coeff_pred.dc_store_lum[b_ypos+1-1][b_xpos+1-1] -
			mp4_state->coeff_pred.dc_store_lum[b_ypos+1-1][b_xpos+1])) // Fb - Fc
			{
				mp4_state->coeff_pred.predict_dir = TOP;
				dc_pred = mp4_state->coeff_pred.dc_store_lum[b_ypos+1-1][b_xpos+1];
			}
		else
			{
				mp4_state->coeff_pred.predict_dir = LEFT;
				dc_pred = mp4_state->coeff_pred.dc_store_lum[b_ypos+1][b_xpos+1-1];
			}

		(* dc_value) += _div_div(dc_pred, mp4_state->hdr.dc_scaler);
		(* dc_value) *= mp4_state->hdr.dc_scaler;

		// store dc value
		mp4_state->coeff_pred.dc_store_lum[b_ypos+1][b_xpos+1] = (* dc_value);
	}
	else // chrominance blocks
	{
		int b_xpos = mp4_state->hdr.mb_xpos;
		int b_ypos = mp4_state->hdr.mb_ypos;
		int chr_num = block_num - 4;
		int dc_pred;

		// set prediction direction
		if (abs(mp4_state->coeff_pred.dc_store_chr[chr_num][b_ypos+1-1][b_xpos+1-1] -
			mp4_state->coeff_pred.dc_store_chr[chr_num][b_ypos+1][b_xpos+1-1]) < // Fa - Fb
			abs(mp4_state->coeff_pred.dc_store_chr[chr_num][b_ypos+1-1][b_xpos+1-1] -
			mp4_state->coeff_pred.dc_store_chr[chr_num][b_ypos+1-1][b_xpos+1])) // Fb - Fc
			{
				mp4_state->coeff_pred.predict_dir = TOP;
				dc_pred = mp4_state->coeff_pred.dc_store_chr[chr_num][b_ypos+1-1][b_xpos+1];
			}
		else
			{
				mp4_state->coeff_pred.predict_dir = LEFT;
				dc_pred = mp4_state->coeff_pred.dc_store_chr[chr_num][b_ypos+1][b_xpos+1-1];
			}

		(* dc_value) += _div_div(dc_pred, mp4_state->hdr.dc_scaler);
		(* dc_value) *= mp4_state->hdr.dc_scaler;

		// store dc value
		mp4_state->coeff_pred.dc_store_chr[chr_num][b_ypos+1][b_xpos+1] = (* dc_value);
	}
}

/***/

static int saiAcLeftIndex[8] = 
{
	0, 8,16,24,32,40,48,56
};

void ac_recon(int block_num, short * psBlock)
{
	int b_xpos, b_ypos;
	int i;

	if (block_num < 4) {
		b_xpos = (mp4_state->hdr.mb_xpos << 1) + (block_num & 1);
		b_ypos = (mp4_state->hdr.mb_ypos << 1) + ((block_num & 2) >> 1);
	}
	else {
		b_xpos = mp4_state->hdr.mb_xpos;
		b_ypos = mp4_state->hdr.mb_ypos;
	}

	// predict coefficients
	if (mp4_state->hdr.ac_pred_flag) 
	{
		if (block_num < 4) 
		{
			if (mp4_state->coeff_pred.predict_dir == TOP)
			{
				for (i = 1; i < 8; i++) // [Review] index can become more efficient [0..7]
					psBlock[i] += mp4_state->coeff_pred.ac_top_lum[b_ypos+1-1][b_xpos+1][i-1];
			}
			else // left prediction
			{
				for (i = 1; i < 8; i++)
					psBlock[mp4_tables->saiAcLeftIndex[i]] += mp4_state->coeff_pred.ac_left_lum[b_ypos+1][b_xpos+1-1][i-1];
			}
		}
		else
		{
			int chr_num = block_num - 4;

			if (mp4_state->coeff_pred.predict_dir == TOP)
			{
				for (i = 1; i < 8; i++)
					psBlock[i] += mp4_state->coeff_pred.ac_top_chr[chr_num][b_ypos+1-1][b_xpos+1][i-1];
			}
			else // left prediction
			{
				for (i = 1; i < 8; i++)
					psBlock[mp4_tables->saiAcLeftIndex[i]] += mp4_state->coeff_pred.ac_left_chr[chr_num][b_ypos+1][b_xpos+1-1][i-1];
			}
		}
	}
}

/***/

void ac_store(int block_num, short * psBlock)
{
	int b_xpos, b_ypos;
	int i;

//printf("ac_store 1\n");
	// [Review] This lines of code are repeated frequently
	if (block_num < 4) { 
		b_xpos = (mp4_state->hdr.mb_xpos << 1) + (block_num & 1);
		b_ypos = (mp4_state->hdr.mb_ypos << 1) + ((block_num & 2) >> 1);
	}
	else {
		b_xpos = mp4_state->hdr.mb_xpos;
		b_ypos = mp4_state->hdr.mb_ypos;
	}
//printf("ac_store 1 %d %d %d\n", block_num, b_xpos, b_ypos);

	// store coefficients
	if (block_num < 4)
	{
		for (i = 1; i < 8; i++) {
			mp4_state->coeff_pred.ac_top_lum[b_ypos+1][b_xpos+1][i-1] = psBlock[i];
			mp4_state->coeff_pred.ac_left_lum[b_ypos+1][b_xpos+1][i-1] = psBlock[mp4_tables->saiAcLeftIndex[i]];
		}
	}
	else 
	{
		int chr_num = block_num - 4;


		for (i = 1; i < 8; i++) {
//printf("ac_store 1 %d %d %d\n", chr_num, b_ypos+1, b_xpos+1);
			mp4_state->coeff_pred.ac_top_chr[chr_num][b_ypos+1][b_xpos+1][i-1] = 
				psBlock[i];
			mp4_state->coeff_pred.ac_left_chr[chr_num][b_ypos+1][b_xpos+1][i-1] = 
				psBlock[mp4_tables->saiAcLeftIndex[i]];
		}
	}
//printf("ac_store 2\n");
}


/***/

#define _rescale(predict_quant, current_quant, coeff)	(coeff != 0) ?	\
_div_div((coeff) * (predict_quant), (current_quant))	: 0

int ac_rescaling(int block_num, short * psBlock)
{
	int mb_xpos = mp4_state->hdr.mb_xpos;
	int mb_ypos = mp4_state->hdr.mb_ypos;
	int current_quant = mp4_state->hdr.quantizer;
	int predict_quant = (mp4_state->coeff_pred.predict_dir == TOP) ?
		mp4_state->quant_store[mb_ypos][mb_xpos+1] : mp4_state->quant_store[mb_ypos+1][mb_xpos];
	int b_xpos, b_ypos; // index for stored coeff matrix
	int i;

	if ((! mp4_state->hdr.ac_pred_flag) || (current_quant == predict_quant) || (block_num == 3))
		return 0;

	if ((mb_ypos == 0) && (mp4_state->coeff_pred.predict_dir == TOP))
		return 0;
	if ((mb_xpos == 0) && (mp4_state->coeff_pred.predict_dir == LEFT))
		return 0;
	if ((mb_xpos == 0) && (mb_ypos == 0))
		return 0;

	// [Review] This lines of code are repeated frequently
	if (block_num < 4) {
		b_xpos = (mp4_state->hdr.mb_xpos << 1) + (block_num & 1);
		b_ypos = (mp4_state->hdr.mb_ypos << 1) + ((block_num & 2) >> 1);
	}
	else {
		b_xpos = mp4_state->hdr.mb_xpos;
		b_ypos = mp4_state->hdr.mb_ypos;
	}

	if (mp4_state->coeff_pred.predict_dir == TOP) // rescale only if really needed
	{
		switch (block_num)
		{
		case 0: case 1:
			for (i = 1; i < 8; i++)
				psBlock[i] += _rescale(predict_quant, current_quant, mp4_state->coeff_pred.ac_top_lum[b_ypos][b_xpos+1][i-1]);
			return 1;
			break;
		case 4:
			for (i = 1; i < 8; i++)
				psBlock[i] += _rescale(predict_quant, current_quant, mp4_state->coeff_pred.ac_top_chr[0][b_ypos][b_xpos+1][i-1]);
			return 1;
			break;
		case 5:
			for (i = 1; i < 8; i++)
				psBlock[i] += _rescale(predict_quant, current_quant, mp4_state->coeff_pred.ac_top_chr[1][b_ypos][b_xpos+1][i-1]);
			return 1;
			break;
		}
	}
	else 
	{
		switch (block_num)
		{
		case 0: case 2:
			for (i = 1; i < 8; i++)
				psBlock[mp4_tables->saiAcLeftIndex[i]] += _rescale(predict_quant, current_quant, mp4_state->coeff_pred.ac_left_lum[b_ypos+1][b_xpos][i-1]);
			return 1;
			break;
		case 4:
			for (i = 1; i < 8; i++)
				psBlock[mp4_tables->saiAcLeftIndex[i]] += _rescale(predict_quant, current_quant, mp4_state->coeff_pred.ac_left_chr[0][b_ypos+1][b_xpos][i-1]);
			return 1;
			break;
		case 5:
			for (i = 1; i < 8; i++)
				psBlock[mp4_tables->saiAcLeftIndex[i]] += _rescale(predict_quant, current_quant, mp4_state->coeff_pred.ac_left_chr[1][b_ypos+1][b_xpos][i-1]);
			return 1;
			break;
		}
	}

	return 0;
}

/***/

#define _IsIntra(mb_y, mb_x) ((mp4_state->modemap[(mb_y)+1][(mb_x)+1] == INTRA) || \
	(mp4_state->modemap[(mb_y)+1][(mb_x)+1] == INTRA_Q))

static void rescue_predict() 
{
	int mb_xpos = mp4_state->hdr.mb_xpos;
	int mb_ypos = mp4_state->hdr.mb_ypos;
	int i;

	if (! _IsIntra(mb_ypos-1, mb_xpos-1)) {
		// rescue -A- DC value
		mp4_state->coeff_pred.dc_store_lum[2*mb_ypos+1-1][2*mb_xpos+1-1] = 1024;
		mp4_state->coeff_pred.dc_store_chr[0][mb_ypos+1-1][mb_xpos+1-1] = 1024;
		mp4_state->coeff_pred.dc_store_chr[1][mb_ypos+1-1][mb_xpos+1-1] = 1024;
	}
	// left
	if (! _IsIntra(mb_ypos, mb_xpos-1)) {
		// rescue -B- DC values
		mp4_state->coeff_pred.dc_store_lum[2*mb_ypos+1][2*mb_xpos+1-1] = 1024;
		mp4_state->coeff_pred.dc_store_lum[2*mb_ypos+1+1][2*mb_xpos+1-1] = 1024;
		mp4_state->coeff_pred.dc_store_chr[0][mb_ypos+1][mb_xpos+1-1] = 1024;
		mp4_state->coeff_pred.dc_store_chr[1][mb_ypos+1][mb_xpos+1-1] = 1024;
		//  rescue -B- AC values
		for(i = 0; i < 7; i++) {
			mp4_state->coeff_pred.ac_left_lum[2*mb_ypos+1][2*mb_xpos+1-1][i] = 0;
			mp4_state->coeff_pred.ac_left_lum[2*mb_ypos+1+1][2*mb_xpos+1-1][i] = 0;
			mp4_state->coeff_pred.ac_left_chr[0][mb_ypos+1][mb_xpos+1-1][i] = 0;
			mp4_state->coeff_pred.ac_left_chr[1][mb_ypos+1][mb_xpos+1-1][i] = 0;
		}
	}
	// top
	if (! _IsIntra(mb_ypos-1, mb_xpos)) {
		// rescue -C- DC values
		mp4_state->coeff_pred.dc_store_lum[2*mb_ypos+1-1][2*mb_xpos+1] = 1024;
		mp4_state->coeff_pred.dc_store_lum[2*mb_ypos+1-1][2*mb_xpos+1+1] = 1024;
		mp4_state->coeff_pred.dc_store_chr[0][mb_ypos+1-1][mb_xpos+1] = 1024;
		mp4_state->coeff_pred.dc_store_chr[1][mb_ypos+1-1][mb_xpos+1] = 1024;
		// rescue -C- AC values
		for(i = 0; i < 7; i++) {
			mp4_state->coeff_pred.ac_top_lum[2*mb_ypos+1-1][2*mb_xpos+1][i] = 0;
			mp4_state->coeff_pred.ac_top_lum[2*mb_ypos+1-1][2*mb_xpos+1+1][i] = 0;
			mp4_state->coeff_pred.ac_top_chr[0][mb_ypos+1-1][mb_xpos+1][i] = 0;
			mp4_state->coeff_pred.ac_top_chr[1][mb_ypos+1-1][mb_xpos+1][i] = 0;
		}
	}
}
