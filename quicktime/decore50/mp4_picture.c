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
// mp4_picture.c //

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mp4_decoder.h"
#include "mp4_vars.h"
#include "getbits.h"

#include "mp4_header.h"
#include "mp4_mblock.h"

//#include "transferIDCT.h"
#include "postprocess.h"
#include "yuv2rgb.h"
#include "store.h"

/**
 *
**/

void addblock (int comp, int bx, int by, int addflag);
int find_pmv (int block, int comp);

static void make_edge (unsigned char *frame_pic, int width, int height, int edge);

/***/

// Purpose: decode and display a Vop
void get_mp4picture (unsigned char *bmp[], unsigned int stride, int render_flag)
{
	mp4_state->hdr.mba = 0;
	mp4_state->hdr.mb_xpos = 0;
	mp4_state->hdr.mb_ypos = 0;

//printf("get_mp4picture 1\n");
	do {
//printf("get_mp4picture 1.1\n");
		macroblock();
//printf("get_mp4picture 1.2\n");
		mp4_state->hdr.mba++; 
//printf("get_mp4picture 1 %d %d\n", mp4_state->hdr.mba, mp4_state->hdr.mba_size);

	} while ((nextbits_bytealigned(23) != 0) &&
		(mp4_state->hdr.mba < mp4_state->hdr.mba_size));
//printf("get_mp4picture 2\n");

// add edge to decoded frame
	make_edge (frame_ref[0], mp4_state->coded_picture_width, mp4_state->coded_picture_height, 32);
	make_edge (frame_ref[1], mp4_state->chrom_width, mp4_state->chrom_height, 16);
	make_edge (frame_ref[2], mp4_state->chrom_width, mp4_state->chrom_height, 16);
//printf("get_mp4picture 3\n");

	PictureDisplay(bmp, stride, render_flag);
//printf("get_mp4picture 4\n");
// exchange ref and for frames
	{
		int i;
		unsigned char *tmp;
		for (i = 0; i < 3; i++) {
			tmp = frame_ref[i];
			frame_ref[i] = frame_for[i];
			frame_for[i] = tmp;
		}
	}
//printf("get_mp4picture 5\n");
}

/***/

// Purpose: move/add 8x8 block to curr_frame
void addblock (int comp, int bx, int by, int addflag)
{
  int cc, i, iincr;
  unsigned char *rfp;
  short *bp;
  unsigned char *curr[3];

	curr[0] = frame_ref[0];
	curr[1] = frame_ref[1];
	curr[2] = frame_ref[2];

  bp = ld->block;

  cc = (comp < 4) ? 0 : (comp & 1) + 1; /* color component index */

  if (cc == 0) // luminance
  {
		// pixel coordinates
		bx <<= 4;
		by <<= 4;
    // frame DCT coding

		rfp = curr[0] + mp4_state->coded_picture_width * (by + ((comp & 2) << 2)) + bx + ((comp & 1) << 3);
    iincr = mp4_state->coded_picture_width;
  } 
  else // chrominance
  {
    // pixel coordinates
		bx <<= 3;
		by <<= 3;
    // frame DCT coding
		rfp = curr[cc] + mp4_state->chrom_width * by + bx;
    iincr = mp4_state->chrom_width;
  }

  if (addflag)
  {
    for (i = 0; i < 8; i++)
    {
      rfp[0] = mp4_state->clp[bp[0] + rfp[0]];
      rfp[1] = mp4_state->clp[bp[1] + rfp[1]];
      rfp[2] = mp4_state->clp[bp[2] + rfp[2]];
      rfp[3] = mp4_state->clp[bp[3] + rfp[3]];
      rfp[4] = mp4_state->clp[bp[4] + rfp[4]];
      rfp[5] = mp4_state->clp[bp[5] + rfp[5]];
      rfp[6] = mp4_state->clp[bp[6] + rfp[6]];
      rfp[7] = mp4_state->clp[bp[7] + rfp[7]];
      bp += 8;
      rfp += iincr;
    }
  } else
  {
    for (i = 0; i < 8; i++)
    {
      rfp[0] = mp4_state->clp[bp[0]];
      rfp[1] = mp4_state->clp[bp[1]];
      rfp[2] = mp4_state->clp[bp[2]];
      rfp[3] = mp4_state->clp[bp[3]];
      rfp[4] = mp4_state->clp[bp[4]];
      rfp[5] = mp4_state->clp[bp[5]];
      rfp[6] = mp4_state->clp[bp[6]];
      rfp[7] = mp4_state->clp[bp[7]];
      bp += 8;
      rfp += iincr;
    }
  }
}

/***/

void addblockIntra (int comp, int bx, int by)
{
  int cc, iincr;
  unsigned char *rfp;
  short *bp;
  unsigned char *curr[3];

	curr[0] = frame_ref[0];
	curr[1] = frame_ref[1];
	curr[2] = frame_ref[2];

  bp = ld->block;

  cc = (comp < 4) ? 0 : (comp & 1) + 1; /* color component index */

  if (cc == 0) // luminance
  {
		// pixel coordinates
		bx <<= 4;
		by <<= 4;
    // frame DCT coding

		rfp = curr[0] + mp4_state->coded_picture_width * (by + ((comp & 2) << 2)) + bx + ((comp & 1) << 3);
    iincr = mp4_state->coded_picture_width;
  } 
  else // chrominance
  {
    // pixel coordinates
		bx <<= 3;
		by <<= 3;
    // frame DCT coding
		rfp = curr[cc] + mp4_state->chrom_width * by + bx;
    iincr = mp4_state->chrom_width;
  }
/***
  for (int i = 0; i < 8; i++)
  {
    rfp[0] = clp[bp[0]];
    rfp[1] = clp[bp[1]];
    rfp[2] = clp[bp[2]];
    rfp[3] = clp[bp[3]];
    rfp[4] = clp[bp[4]];
    rfp[5] = clp[bp[5]];
    rfp[6] = clp[bp[6]];
    rfp[7] = clp[bp[7]];
    bp += 8;
    rfp += iincr;
  }
***/ transferIDCT_copy(bp, rfp, iincr);
}

/***/

void addblockInter (int comp, int bx, int by)
{
  int cc, iincr;
  unsigned char *rfp;
  short *bp;
  unsigned char *curr[3];

	curr[0] = frame_ref[0];
	curr[1] = frame_ref[1];
	curr[2] = frame_ref[2];

  bp = ld->block;

  cc = (comp < 4) ? 0 : (comp & 1) + 1; /* color component index */

  if (cc == 0) // luminance
  {
		// pixel coordinates
		bx <<= 4;
		by <<= 4;
    // frame DCT coding

		rfp = curr[0] + mp4_state->coded_picture_width * (by + ((comp & 2) << 2)) + bx + ((comp & 1) << 3);
    iincr = mp4_state->coded_picture_width;
  } 
  else // chrominance
  {
    // pixel coordinates
		bx <<= 3;
		by <<= 3;
    // frame DCT coding
		rfp = curr[cc] + mp4_state->chrom_width * by + bx;
    iincr = mp4_state->chrom_width;
  }
/***
  for (int i = 0; i < 8; i++)
  {
    rfp[0] = clp[bp[0] + rfp[0]];
    rfp[1] = clp[bp[1] + rfp[1]];
    rfp[2] = clp[bp[2] + rfp[2]];
    rfp[3] = clp[bp[3] + rfp[3]];
    rfp[4] = clp[bp[4] + rfp[4]];
    rfp[5] = clp[bp[5] + rfp[5]];
    rfp[6] = clp[bp[6] + rfp[6]];
    rfp[7] = clp[bp[7] + rfp[7]];
    bp += 8;
    rfp += iincr;
  }
***/ transferIDCT_add(bp, rfp, iincr);
}

/***/

// int x, y: block coord
// int block block num
// int mv comp (0: x, 1: y)
//
// Purpose: compute motion vector prediction
int find_pmv (int block, int comp)
{
  int p1, p2, p3;
  int xin1, xin2, xin3;
  int yin1, yin2, yin3;
  int vec1, vec2, vec3;

	int x = mp4_state->hdr.mb_xpos;
	int y = mp4_state->hdr.mb_ypos;
	
	if ((y == 0) && ((block == 0) || (block == 1)))
	{
		if ((x == 0) && (block == 0))
			return 0;
		else if (block == 1)
			return mp4_state->MV[comp][0][y+1][x+1];
		else // block == 0
			return mp4_state->MV[comp][1][y+1][x+1-1];
	}
	else
	{
		// considerate border (avoid increment inside each single array index)
		x++;
		y++;

		switch (block)
		{
			case 0: 
				vec1 = 1;	yin1 = y;		xin1 = x-1;
				vec2 = 2;	yin2 = y-1;	xin2 = x;
				vec3 = 2;	yin3 = y-1;	xin3 = x+1;
				break;
			case 1:
				vec1 = 0;	yin1 = y;		xin1 = x;
				vec2 = 3;	yin2 = y-1;	xin2 = x;
				vec3 = 2;	yin3 = y-1;	xin3 = x+1;
				break;
			case 2:
				vec1 = 3;	yin1 = y;		xin1 = x-1;
				vec2 = 0;	yin2 = y;	  xin2 = x;
				vec3 = 1;	yin3 = y;		xin3 = x;
				break;
			default: // case 3
				vec1 = 2;	yin1 = y;		xin1 = x;
				vec2 = 0;	yin2 = y;		xin2 = x;
				vec3 = 1;	yin3 = y;		xin3 = x;
				break;
		}
		p1 = mp4_state->MV[comp][vec1][yin1][xin1];
		p2 = mp4_state->MV[comp][vec2][yin2][xin2];
		p3 = mp4_state->MV[comp][vec3][yin3][xin3];

		// return p1 + p2 + p3 - mmax (p1, mmax (p2, p3)) - mmin (p1, mmin (p2, p3));
		return mmin(mmax(p1, p2), mmin(mmax(p2, p3), mmax(p1, p3)));
	}
}

/***/

void make_edge (unsigned char *frame_pic,
                int edged_width, int edged_height, int edge)
{
  int j;

	int width = edged_width - (2*edge);
	int height = edged_height - (2*edge);
	
	unsigned char *p_border;
	unsigned char *p_border_top, *p_border_bottom;
	unsigned char *p_border_top_ref, *p_border_bottom_ref;

	// left and right edges
	p_border = frame_pic;

	for (j = 0; j < height; j++)
	{
		unsigned char border_left = *(p_border);
		unsigned char border_right = *(p_border + (width-1));

		memset((p_border - edge), border_left, edge);
		memset((p_border + width), border_right, edge);

		p_border += edged_width;
	}

	// top and bottom edges
	p_border_top_ref = frame_pic;
	p_border_bottom_ref = frame_pic + (edged_width * (height -1));
	p_border_top = p_border_top_ref - (edge * edged_width);
	p_border_bottom = p_border_bottom_ref + edged_width;

	for (j = 0; j < edge; j++)
	{
		memcpy(p_border_top, p_border_top_ref, width);
		memcpy(p_border_bottom, p_border_bottom_ref, width);

		p_border_top += edged_width;
		p_border_bottom += edged_width;
	}

  // corners
	{
		unsigned char * p_left_corner_top = frame_pic - edge - (edge * edged_width);
		unsigned char * p_right_corner_top = p_left_corner_top + edge + width;
		unsigned char * p_left_corner_bottom = frame_pic + (edged_width * height) - edge;
		unsigned char * p_right_corner_bottom = p_left_corner_bottom + edge + width;

		char left_corner_top = *(frame_pic);
		char right_corner_top = *(frame_pic + (width-1));
		char left_corner_bottom = *(frame_pic + (edged_width * (height-1)));
		char right_corner_bottom = *(frame_pic + (edged_width * (height-1)) + (width-1));

		for (j = 0; j < edge; j++)
		{
			memset(p_left_corner_top, left_corner_top, edge);
			memset(p_right_corner_top, right_corner_top, edge);
			memset(p_left_corner_bottom, left_corner_bottom, edge);
			memset(p_right_corner_bottom, right_corner_bottom, edge);

			p_left_corner_top += edged_width;
			p_right_corner_top += edged_width;
			p_left_corner_bottom += edged_width;
			p_right_corner_bottom += edged_width;
		}
	}
}

/***/

// Purpose: manages a one frame buffer for re-ordering frames prior to 
//  displaying or writing to a file
void PictureDisplay(unsigned char *bmp[], unsigned int stride, int render_flag)
{ 
#ifdef _DECORE
	if (render_flag) 
	{
		if (mp4_state->post_flag)
		{
//printf("PictureDisplay 1\n");
			postprocess(frame_ref, mp4_state->coded_picture_width,
				display_frame, mp4_state->horizontal_size, 
				mp4_state->horizontal_size,  mp4_state->vertical_size, 
				&mp4_state->quant_store[1][1], (DEC_MBC+1), mp4_state->pp_options);
			mp4_state->convert_yuv(display_frame[0], mp4_state->hdr.width,
				display_frame[1], display_frame[2], (mp4_state->hdr.width>>1),
				bmp, mp4_state->hdr.width, mp4_state->flag_invert * mp4_state->hdr.height,
				stride);
//printf("PictureDisplay 2\n");
		}
		else
		{
//printf("PictureDisplay 3 %p\n", mp4_state->convert_yuv);
			mp4_state->convert_yuv(frame_ref[0], 
				mp4_state->coded_picture_width,
				frame_ref[1], 
				frame_ref[2], 
				(mp4_state->coded_picture_width>>1),
				bmp, 
				mp4_state->hdr.width, 
				mp4_state->flag_invert * mp4_state->hdr.height,
				stride);
//printf("PictureDisplay 4\n");
		}
	}
#else
	// output on a file
	storeframe (frame_ref, mp4_state->coded_picture_width, mp4_state->vertical_size);
#endif
}
