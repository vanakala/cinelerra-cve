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
// mp4_mblock.c //

#include <math.h>
#include <assert.h>

#include "mp4_vars.h"

#include "getbits.h"
#include "mp4_block.h"
#include "mp4_mblock.h"
#include "debug.h"

/**
 *
**/

static int getMCBPC();
static int getCBPY();
static int setMV(int block_num);
static int getMVdata();

/***/

extern int find_pmv(int block_num, int type);
extern void addblock (int comp, int bx, int by, int addflag);
extern void addblockIntra (int comp, int bx, int by);
extern void addblockInter (int comp, int bx, int by);

/***/

int macroblock()
{
	int j;
	int intraFlag, interFlag;

//	_Print("-Macroblock %d\n", mp4_state->hdr.mba);
//	_Break(93, 426);
//printf("macroblock 1\n");

	if (mp4_state->hdr.prediction_type != I_VOP)
		mp4_state->hdr.not_coded = getbits(1);
//printf("macroblock 2\n");

	// coded macroblock or I-VOP
	if (! mp4_state->hdr.not_coded || mp4_state->hdr.prediction_type == I_VOP) 
	{

//printf("macroblock 3\n");
//printf("macroblock: 1 %02x\n", showbits(16));
		mp4_state->hdr.mcbpc = getMCBPC(); // mcbpc
//printf("macroblock: 2 %02x\n", showbits(16));




		mp4_state->hdr.derived_mb_type = mp4_state->hdr.mcbpc & 7;
		mp4_state->hdr.cbpc = (mp4_state->hdr.mcbpc >> 4) & 3;
//printf("macroblock 4\n");

		mp4_state->modemap[mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] = 
			mp4_state->hdr.derived_mb_type; // [Review] used only in P-VOPs
		intraFlag = ((mp4_state->hdr.derived_mb_type == INTRA) || 
			(mp4_state->hdr.derived_mb_type == INTRA_Q)) ? 1 : 0;
		interFlag = (! intraFlag);
//printf("macroblock 5\n");

		if (intraFlag)
			mp4_state->hdr.ac_pred_flag = getbits(1);
		if (mp4_state->hdr.derived_mb_type != STUFFING) {

			mp4_state->hdr.cbpy = getCBPY(); // cbpy
			mp4_state->hdr.cbp = (mp4_state->hdr.cbpy << 2) | mp4_state->hdr.cbpc;
		}
		else
			return 1;
//printf("macroblock 6\n");

//		_Print("Mb type: %d\n", mp4_state->hdr.derived_mb_type);
//		_Print("Pattern Chr: %d\n", mp4_state->hdr.cbpc);
//		_Print("Pattern Luma: %d\n", mp4_state->hdr.cbpy);

		if (mp4_state->hdr.derived_mb_type == INTER_Q ||
			mp4_state->hdr.derived_mb_type == INTRA_Q) {

			mp4_state->hdr.dquant = getbits(2);
			mp4_state->hdr.quantizer += DQtab[mp4_state->hdr.dquant];
			if (mp4_state->hdr.quantizer > 31)
				mp4_state->hdr.quantizer = 31;
			else if (mp4_state->hdr.quantizer < 1)
				mp4_state->hdr.quantizer = 1;
		}
		if (mp4_state->hdr.derived_mb_type == INTER ||
			mp4_state->hdr.derived_mb_type == INTER_Q) {

			setMV(-1); // mv
		}
		else if (mp4_state->hdr.derived_mb_type == INTER4V) {

			for (j = 0; j < 4; j++) {

				setMV(j); // mv
			}
		}
		else { // intra
			if (mp4_state->hdr.prediction_type == P_VOP) {
				int i;
				for (i = 0; i < 4; i++) {
					mp4_state->MV[0][i][mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] = 0;
					mp4_state->MV[1][i][mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] = 0;
				}
			}
		}
//printf("macroblock 7\n");

//printf("macroblock 3 %02x\n", showbits(16));
		// motion compensation
		if (interFlag) 
		{
//printf("macroblock 8\n");
			divx_reconstruct(mp4_state->hdr.mb_xpos, mp4_state->hdr.mb_ypos, mp4_state->hdr.derived_mb_type);
//printf("macroblock 9\n");

			// texture decoding add
			for (j = 0; j < 6; j++) {
				int coded = mp4_state->hdr.cbp & (1 << (5 - j));

				if (coded) { 
					blockInter(j, (coded != 0));
					addblockInter(j, mp4_state->hdr.mb_xpos, mp4_state->hdr.mb_ypos);
				}
			}
//printf("macroblock 10\n");
		}
		else 
		{
			// texture decoding add
//printf("macroblock 11\n");
			for (j = 0; j < 6; j++) {
				int coded = mp4_state->hdr.cbp & (1 << (5 - j));

//printf("macroblock 11.1 %02x\n", showbits(16));
				blockIntra(j, (coded != 0));
//printf("macroblock 11.2 %02x\n", showbits(16));
				addblockIntra(j, mp4_state->hdr.mb_xpos, mp4_state->hdr.mb_ypos);
//printf("macroblock 11.3 %02x\n", showbits(16));
			}
//printf("macroblock 12\n");
		}
//printf("macroblock 8 %02x\n", showbits(16));
	}

	// not coded macroblock
	else {

//printf("macroblock 9\n");
		mp4_state->MV[0][0][mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] = mp4_state->MV[0][1][mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] =
		mp4_state->MV[0][2][mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] = mp4_state->MV[0][3][mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] = 0;
		mp4_state->MV[1][0][mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] = mp4_state->MV[1][1][mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] =
		mp4_state->MV[1][2][mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] = mp4_state->MV[1][3][mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] = 0;

		mp4_state->modemap[mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] = NOT_CODED; // [Review] used only in P-VOPs

		divx_reconstruct(mp4_state->hdr.mb_xpos, mp4_state->hdr.mb_ypos, mp4_state->hdr.derived_mb_type);
//printf("macroblock 10\n");
	}

	mp4_state->quant_store[mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] = mp4_state->hdr.quantizer;
//printf("macroblock 11\n");

	if (mp4_state->hdr.mb_xpos < (mp4_state->mb_width-1)) {
		mp4_state->hdr.mb_xpos++;
	}
	else {
		mp4_state->hdr.mb_ypos++;
		mp4_state->hdr.mb_xpos = 0;
	}
//printf("macroblock 12\n");

	return 1;
}

/***/

static int getMCBPC()
{
	if (mp4_state->hdr.prediction_type == I_VOP)
	{
		int code = showbits(9);

		if (code == 1) {
			flushbits(9); // stuffing
			return 0;
		}
		else if (code < 8) {
			return -1;
		}

		code >>= 3;
		if (code >= 32) {
			flushbits(1);
			return 3;
		}
		
		flushbits(mp4_tables->MCBPCtabIntra[code].len);
		return mp4_tables->MCBPCtabIntra[code].val;
	}
	else
	{
		int code = showbits(9);

		if (code == 1) {
			flushbits(9); // stuffing
			return 0;
		}
		else if (code == 0)	{
			return -1;
		}
		
		if (code >= 256)
		{
			flushbits(1);
			return 0;
		}
		
		flushbits(mp4_tables->MCBPCtabInter[code].len);
		return mp4_tables->MCBPCtabInter[code].val;
	}
}

/***/

static int getCBPY()
{
	int cbpy;
	int code = showbits(6);

	if (code < 2) {
		return -1;
	}
			  
	if (code >= 48) {
		flushbits(2);
		cbpy = 15;
	} else {
		flushbits(mp4_tables->CBPYtab[code].len);
		cbpy = mp4_tables->CBPYtab[code].val;
	}

	if (!((mp4_state->hdr.derived_mb_type == 3) ||
		(mp4_state->hdr.derived_mb_type == 4)))
		  cbpy = 15 - cbpy;

  return cbpy;
}

/***/

static int setMV(int block_num)
{
	int hor_mv_data, ver_mv_data, hor_mv_res, ver_mv_res;
	int scale_fac = 1 << (mp4_state->hdr.fcode_for - 1);
	int high = (32 * scale_fac) - 1;
	int low = ((-32) * scale_fac);
	int range = (64 * scale_fac);

	int mvd_x, mvd_y, pmv_x, pmv_y, mv_x, mv_y;

/***

	[hor_mv_data]
	if ((vop_fcode_forward != 1) && (hor_mv_data != 0))
		[hor_mv_residual]

***/	

  hor_mv_data = getMVdata(); // mv data

	if ((scale_fac == 1) || (hor_mv_data == 0))
		mvd_x = hor_mv_data;
	else {
		hor_mv_res = getbits(mp4_state->hdr.fcode_for-1); // mv residual
		mvd_x = ((abs(hor_mv_data) - 1) * scale_fac) + hor_mv_res + 1;
		if (hor_mv_data < 0)
			mvd_x = - mvd_x;
	}

  ver_mv_data = getMVdata(); 

	if ((scale_fac == 1) || (ver_mv_data == 0))
		mvd_y = ver_mv_data;
	else {
		ver_mv_res = getbits(mp4_state->hdr.fcode_for-1);
		mvd_y = ((abs(ver_mv_data) - 1) * scale_fac) + ver_mv_res + 1;
		if (ver_mv_data < 0)
			mvd_y = - mvd_y;
	}

	if (block_num == -1) {
		pmv_x = find_pmv(0, 0);
		pmv_y = find_pmv(0, 1);
	}
	else {
		pmv_x = find_pmv(block_num, 0);
		pmv_y = find_pmv(block_num, 1);
	}

//	_Print("Hor MotV Pred: %d\n", pmv_x);
//	_Print("Ver MorV Pred: %d\n", pmv_y);

	mv_x = pmv_x + mvd_x;

	if (mv_x < low)
		mv_x += range;
	if (mv_x > high)
		mv_x -= range;

	mv_y = pmv_y + mvd_y;

	if (mv_y < low)
		mv_y += range;
	if (mv_y > high)
		mv_y -= range;

	// put [mv_x, mv_y] in MV struct
	if (block_num == -1) {
		int i;
		for (i = 0; i < 4; i++) {
			mp4_state->MV[0][i][mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] = mv_x;
			mp4_state->MV[1][i][mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] = mv_y;
		}
	}
	else {
		mp4_state->MV[0][block_num][mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] = mv_x;
		mp4_state->MV[1][block_num][mp4_state->hdr.mb_ypos+1][mp4_state->hdr.mb_xpos+1] = mv_y;
	}

//  _Print("Hor MotV: %d\n", mv_x);
//	_Print("Ver MotV: %d\n", mv_y);

  return 1;
}

/***/

static int getMVdata()
{
	int code;

	if (getbits(1)) {
		return 0; // hor_mv_data == 0
  }
	
	code = showbits(12);

	if (code >= 512)
  {
		code = (code >> 8) - 2;
		flushbits(mp4_tables->MVtab0[code].len);
		return mp4_tables->MVtab0[code].val;
  }
	
	if (code >= 128)
  {
		code = (code >> 2) - 32;
		flushbits(mp4_tables->MVtab1[code].len);
		return mp4_tables->MVtab1[code].val;
  }

	code -= 4; 

//	assert(code >= 0);
	if(code <= 0) return 0;

	flushbits(mp4_tables->MVtab2[code].len);
	return mp4_tables->MVtab2[code].val;
}

/***/

