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
/// mp4_block.c //

#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include "mp4_vars.h"

#include "getbits.h"
#include "clearblock.h"
#include "mp4_iquant.h"
#include "mp4_predict.h"
#include "mp4_vld.h"
#include "debug.h"
#include "mp4_block.h"

/**
 *
**/

static int getDCdiff();
static void setDCscaler(int block_num);

static int getACdir();

/***/

// fps before -> 90 - 94
// fps after -> 94 - 99
#define CLEARBLOCK(x) bzero(x, 128);

static int getDCsizeLum()
{
	int code;

	// [Ag][note] bad code

	if (showbits(11) == 1) {
		flushbits(11);
		return 12;
	}
	if (showbits(10) == 1) {
      flushbits(10);
      return 11;
	}
	if (showbits(9) == 1) {
      flushbits(9);
      return 10;
	}
	if (showbits(8) == 1) {
		flushbits(8);
		return 9;
	}
	if (showbits(7) == 1) {
		flushbits(7);
		return 8;
	}
	if (showbits(6) == 1) {
		flushbits(6);
		return 7;
	}  
	if (showbits(5) == 1) {
		flushbits(5);
		return 6;
	}
	if (showbits(4) == 1) {
		flushbits(4);
		return 5;
	}

	code = showbits(3);

//printf("getDCsizeLum 1 %x %02x\n", code, showbits(16));
	if (code == 1) {
		flushbits(3);
		return 4;
	} else if (code == 2) {
		flushbits(3);
		return 3;
	} else if (code == 3) {
		flushbits(3);
//printf("getDCsizeLum 2 %x %02x\n", code, showbits(16));
		return 0;
	}

  	code = showbits(2);

  	if (code == 2) {
		flushbits(2);
		return 2;
	} else if (code == 3) {
		flushbits(2);
		return 1;
	}     
//printf("getDCsizeLum 2 %02x\n", showbits(16));

	return 0;
}

static int getDCsizeChr()
{
	// [Ag][note] bad code

	if (showbits(12) == 1) {
		flushbits(12);
		return 12;
	}
	if (showbits(11) == 1) {
		flushbits(11);
		return 11;
	}
	if (showbits(10) == 1) {
		flushbits(10);
		return 10;
	}
	if (showbits(9) == 1) {
		flushbits(9);
		return 9;
	}
	if (showbits(8) == 1) {
		flushbits(8);
		return 8;
	}
	if (showbits(7) == 1) {
		flushbits(7);
		return 7;
	}
	if (showbits(6) == 1) {
		flushbits(6);
		return 6;
	}
	if (showbits(5) == 1) {
		flushbits(5);
		return 5;
	}
	if (showbits(4) == 1) {
		flushbits(4);
		return 4;
	} 
	if (showbits(3) == 1) {
		flushbits(3);
		return 3;
	} 

	return (3 - getbits(2));
}

// Purpose: texture decoding of block_num
int block(int block_num, int coded)
{
	int i;
	int dct_dc_size, dct_dc_diff;
	int intraFlag = ((mp4_state->hdr.derived_mb_type == INTRA) || 
		(mp4_state->hdr.derived_mb_type == INTRA_Q)) ? 1 : 0;
	event_t event;

	CLEARBLOCK(ld->block); // clearblock

	if (intraFlag)
	{
		setDCscaler(block_num); // calculate DC scaler

		if (block_num < 4) {
			dct_dc_size = getDCsizeLum();
			if (dct_dc_size != 0) 
				dct_dc_diff = getDCdiff(dct_dc_size);
			else 
				dct_dc_diff = 0;
			if (dct_dc_size > 8)
				getbits1(); // marker bit
		}
		else {
			dct_dc_size = getDCsizeChr();
			if (dct_dc_size != 0)
				dct_dc_diff = getDCdiff(dct_dc_size);
			else 
				dct_dc_diff = 0;
			if (dct_dc_size > 8)
				getbits1(); // marker bit
		}

		ld->block[0] = (short) dct_dc_diff;
	}
	if (intraFlag)
	{
		// dc reconstruction, prediction direction
		dc_recon(block_num, &ld->block[0]);
	}

	if (coded) 
	{
		unsigned int * zigzag; // zigzag scan dir

		if ((intraFlag) && (mp4_state->hdr.ac_pred_flag == 1)) {

			if (mp4_state->coeff_pred.predict_dir == TOP)
				zigzag = mp4_tables->alternate_horizontal_scan;
			else
				zigzag = mp4_tables->alternate_vertical_scan;
		}
		else {
			zigzag = mp4_tables->zig_zag_scan;
		}

		i = intraFlag ? 1 : 0;



		do // event vld
		{
			event = vld_event(intraFlag);
/***
			if (event.run == -1)
			{
				printf("Error: invalid vld code\n");
				exit(201);
			}
***/			
			i+= event.run;
			ld->block[zigzag[i]] = (short) event.level;

//			_Print("Vld Event: Run Level Last %d %d %d\n", event.run, event.level, event.last);

			i++;
		} while (! event.last);
	}

	if (intraFlag)
	{
		// ac reconstruction
		// ac_rescaling(...)
		ac_recon(block_num, &ld->block[0]);
	}

#ifdef _DEBUG_B_ACDC
	if (intraFlag)
	{
		int i;
		_Print("After AcDcRecon:\n");
		_Print("   x ");
		for (i = 1; i < 64; i++) {
			if ((i != 0) && ((i % 8) == 0))
				_Print("\n");
			_Print("%4d ", ld->block[i]);
		}
		_Print("\n");
	}
#endif // _DEBUG_ACDC

	if (mp4_state->hdr.quant_type == 0)
	{
		// inverse quantization
		iquant(ld->block, intraFlag);
	}
	else 
	{
		_Error("Error: MPEG-2 inverse quantization NOT implemented\n");
		exit(110);
	}

#ifdef _DEBUG_B_QUANT
	{
		int i;
		_Print("After IQuant:\n");
		_Print("   x ");
		for (i = 1; i < 64; i++) {
			if ((i != 0) && ((i % 8) == 0))
				_Print("\n");
			_Print("%4d ", ld->block[i]);
		}
		_Print("\n");
	}
#endif // _DEBUG_B_QUANT

	// inverse dct
	idct(ld->block);

	return 1;
}

/***/

int blockIntra(int block_num, int coded)
{
	int i;
	int dct_dc_size, dct_dc_diff;
	event_t event;

//printf("blockIntra 1\n");
	CLEARBLOCK(ld->block); // clearblock
//printf("blockIntra 1\n");







	// dc coeff
	setDCscaler(block_num); // calculate DC scaler






	if (block_num < 4) {
//printf("blockIntra 1 %02x\n", showbits(16));
		dct_dc_size = getDCsizeLum();
//printf("blockIntra 2 %02x\n", showbits(16));
		if (dct_dc_size != 0) 
			dct_dc_diff = getDCdiff(dct_dc_size);
		else 
			dct_dc_diff = 0;
		if (dct_dc_size > 8)
			getbits1(); // marker bit
	}
	else
	{
		dct_dc_size = getDCsizeChr();
		if (dct_dc_size != 0)
			dct_dc_diff = getDCdiff(dct_dc_size);
		else 
			dct_dc_diff = 0;
		if (dct_dc_size > 8)
			getbits1(); // marker bit
	}






//printf("blockIntra 1\n");

	ld->block[0] = (short) dct_dc_diff;
//printf("blockIntra 1\n");

	// dc reconstruction, prediction direction
	dc_recon(block_num, &ld->block[0]);
//printf("blockIntra 1\n");

	if (coded) 
	{
		unsigned int * zigzag; // zigzag scan dir

		if (mp4_state->hdr.ac_pred_flag == 1) {

			if (mp4_state->coeff_pred.predict_dir == TOP)
				zigzag = mp4_tables->alternate_horizontal_scan;
			else
				zigzag = mp4_tables->alternate_vertical_scan;
		}
		else {
			zigzag = mp4_tables->zig_zag_scan;
		}

		i = 1;
//printf("blockIntra 1 %02x\n", showbits(12));
		do // event vld
		{
			event = vld_intra_dct();
/***
			if (event.run == -1)
			{
				printf("Error: invalid vld code\n");
				exit(201);
			}
***/			
			i+= event.run;
			ld->block[zigzag[i]] = (short) event.level;

//			_Print("Vld Event: Run Level Last %d %d %d\n", event.run, event.level, event.last);

			i++;
		} while (! event.last);
	}
//printf("blockIntra 1\n");

	// ac reconstruction
	mp4_state->hdr.intrablock_rescaled = ac_rescaling(block_num, &ld->block[0]);





	if (! mp4_state->hdr.intrablock_rescaled)
	{
		ac_recon(block_num, &ld->block[0]);
	}
//printf("blockIntra 1\n");
	ac_store(block_num, &ld->block[0]);
//printf("blockIntra 1\n");

	if (mp4_state->hdr.quant_type == 0)
	{
		iquant(ld->block, 1);
	}
	else 
	{
		iquant_typefirst(ld->block);
	}
//printf("blockIntra 1\n");

	// inverse dct
	idct(ld->block);
//printf("blockIntra 2\n");

	return 1;
}

/***/

int blockInter(int block_num, int coded)
{
	event_t event;
	unsigned int * zigzag = mp4_tables->zig_zag_scan; // zigzag scan dir
	int i;
	
	CLEARBLOCK(ld->block); // clearblock

	// inverse quant type
	if (mp4_state->hdr.quant_type == 0) 
	{
		int q_scale = mp4_state->hdr.quantizer;
		int q_2scale = q_scale << 1;
		int q_add = (q_scale & 1) ? q_scale : (q_scale - 1);
			
		i = 0;
		do // event vld
		{
			event = vld_inter_dct();

			/***
			if (event.run == -1)
			{
			printf("Error: invalid vld code\n");
			exit(201);
			}
			***/			
			i+= event.run;
			if (event.level > 0) {
				ld->block[zigzag[i]] = (q_2scale * event.level) + q_add;
			}
			else {
				ld->block[zigzag[i]] = (q_2scale * event.level) - q_add;
			}
			
			// _Print("Vld Event: Run Level Last %d %d %d\n", event.run, event.level, event.last);
			
			i++;
		} while (! event.last);
	}
	else 
	{
		int k, m = 0;
		i = 0;

		// event vld
		do 
		{
			event = vld_inter_dct();

			i+= event.run;
	
			k = (event.level > 0) ? 1 : -1;

			assert(ld->block[zigzag[i]] < 2047);
			assert(ld->block[zigzag[i]] > -2048);

			ld->block[zigzag[i]] = ((2 * event.level + k) * mp4_state->hdr.quantizer * 
				mp4_tables->nonintra_quant_matrix[zigzag[i]]) >> 4;

			assert(ld->block[zigzag[i]] < 2047);
			assert(ld->block[zigzag[i]] > -2048);

			m ^= ld->block[zigzag[i]];
			
			// _Print("Vld Event: Run Level Last %d %d %d\n", event.run, event.level, event.last);
			
			i++;
		} while (! event.last);

		if (!(m%2)) ld->block[63] ^= 1;
	}

	// inverse dct
	idct(ld->block);
		
	return 1;
}

/***/

/***/

static int getDCdiff(int dct_dc_size)
{
	int code = getbits(dct_dc_size);
	int msb = code >> (dct_dc_size - 1);

	if (msb == 0) {
		return (-1 * (code^((int) pow(2.0,(double) dct_dc_size) - 1)));
	}
  else { 
		return code;
	}
}

/***/

static void setDCscaler(int block_num) 
{
	int type = (block_num < 4) ? 0 : 1;
	int quant = mp4_state->hdr.quantizer;

	if (type == 0) {
		if (quant > 0 && quant < 5) 
			mp4_state->hdr.dc_scaler = 8;
		else if (quant > 4 && quant < 9) 
			mp4_state->hdr.dc_scaler = (2 * quant);
		else if (quant > 8 && quant < 25) 
			mp4_state->hdr.dc_scaler = (quant + 8);
		else 
			mp4_state->hdr.dc_scaler = (2 * quant - 16);
	}
  else {
		if (quant > 0 && quant < 5) 
			mp4_state->hdr.dc_scaler = 8;
		else if (quant > 4 && quant < 25) 
			mp4_state->hdr.dc_scaler = ((quant + 13) / 2);
		else 
			mp4_state->hdr.dc_scaler = (quant - 6);
	}
}

/***/

