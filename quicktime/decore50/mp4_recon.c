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
// mp4_recon.c //


#include "mp4_vars.h"

#include "basic_prediction.h"
#include "timer.h"
/**
 *
**/

void recon_comp (unsigned char *src, unsigned char *dst, 
								int lx, int w, int h, int x, 
								int y, int dx, int dy, int chroma)
{
  int xint, xh, yint, yh;
  unsigned char *s, *d;

  xint = dx >> 1;
  xh = dx & 1;
  yint = dy >> 1;
  yh = dy & 1;

  /* origins */
  s = src + lx * (y + yint) + x + xint;
  d = dst + lx * y + x;

	{
		int mc_driver = ((w!=8)<<3) | (mp4_state->hdr.rounding_type<<2) | (yh<<1) | (xh);





//printf("recon_comp 1 %x\n", mc_driver);
		switch (mc_driver)
		{
		// block
			 // no round
			case 0: case 4:
 			  CopyBlock(s, d, lx);
			  break;
			case 1:
				CopyBlockHor(s, d, lx);
				break;
			case 2:
				CopyBlockVer(s, d, lx);
				break;
			case 3:
				CopyBlockHorVer(s, d, lx);
				break;
				// round
			case 5:
				CopyBlockHorRound(s, d, lx);
				break;
			case 6:
				CopyBlockVerRound(s, d, lx);
				break;
			case 7:
				CopyBlockHorVerRound(s, d, lx);
				break;
		// macroblock
			// no round
			case 8: case 12:
				CopyMBlock(s, d, lx);
				break;
			case 9:
				CopyMBlockHor(s, d, lx);
				break;
			case 10:
				CopyMBlockVer(s, d, lx);
				break;
			case 11:
				CopyMBlockHorVer(s, d, lx);
				break;
			// round
			case 13:
				CopyMBlockHorRound(s, d, lx);
				break;
			case 14:
				CopyMBlockVerRound(s, d, lx);
				break;
			case 15:
				CopyMBlockHorVerRound(s, d, lx);
				break;
		}
//printf("recon_comp 2\n");
	}
}


/***/

void divx_reconstruct (int bx, int by, int mode)
{
  int w, h, lx, dx, dy, xp, yp, comp, sum;
  int x, y, px, py;
  unsigned char * src[3];
  start_timer();
	
  x = bx + 1;
  y = by + 1;


//printf("divx_reconstruct 1\n");
  lx = mp4_state->coded_picture_width;

  src[0] = frame_for[0];
  src[1] = frame_for[1];
  src[2] = frame_for[2];

//printf("divx_reconstruct 1\n");
	w = 8;
	h = 8;

//printf("divx_reconstruct 1\n");
	// Luma
	px = bx << 4;
	py = by << 4;
	if (mode == INTER4V)
	{
		for (comp = 0; comp < 4; comp++)
		{
			dx = mp4_state->MV[0][comp][y][x];
			dy = mp4_state->MV[1][comp][y][x];
			
			xp = px + ((comp & 1) << 3);
			yp = py + ((comp & 2) << 2);
//printf("divx_reconstruct 1.1\n");

			recon_comp (src[0], frame_ref[0], lx, w, h, xp, yp, dx, dy, 0);
//printf("divx_reconstruct 1.2\n");
		}
	} else
	{
		dx = mp4_state->MV[0][0][y][x];
		dy = mp4_state->MV[1][0][y][x];

//printf("divx_reconstruct 1.3\n");
		recon_comp (src[0], frame_ref[0], lx, w << 1, h << 1, px, py, dx, dy, 0);
//printf("divx_reconstruct 1.4\n");
	}
	
//printf("divx_reconstruct 1\n");
	// Chr
	px = bx << 3;
	py = by << 3;
	if (mode == INTER4V)
	{
		sum = mp4_state->MV[0][0][y][x] + mp4_state->MV[0][1][y][x] + 
					mp4_state->MV[0][2][y][x] + mp4_state->MV[0][3][y][x];
		if (sum == 0) 
			dx = 0;
		else
			dx = sign (sum) * (mp4_tables->roundtab[abs (sum) % 16] + (abs (sum) / 16) * 2);
		
		sum = mp4_state->MV[1][0][y][x] + mp4_state->MV[1][1][y][x] + 
					mp4_state->MV[1][2][y][x] + mp4_state->MV[1][3][y][x];
		if (sum == 0)
			dy = 0;
		else
			dy = sign (sum) * (mp4_tables->roundtab[abs (sum) % 16] + (abs (sum) / 16) * 2);
		
	} else
	{
		dx = mp4_state->MV[0][0][y][x];
		dy = mp4_state->MV[1][0][y][x];

		/* chroma rounding */
		dx = (dx % 4 == 0 ? dx >> 1 : (dx >> 1) | 1);
		dy = (dy % 4 == 0 ? dy >> 1 : (dy >> 1) | 1);
	}
	lx >>= 1;

//printf("divx_reconstruct 1\n");
	recon_comp (src[1], frame_ref[1], lx, w, h, px, py, dx, dy, 1);
	recon_comp (src[2], frame_ref[2], lx, w, h, px, py, dx, dy, 2);

//printf("divx_reconstruct 1\n");
	stop_recon_timer();
//printf("divx_reconstruct 2\n");
}

/***/

