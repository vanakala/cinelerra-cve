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

#include "basic_prediction.h"

/*

	half_flag[t]

	t = 0		horizontal component
	t = 1		vertical component

*/

// Purpose: specialized basic motion compensation routines
void CopyBlock(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy;

#ifdef BIG_ENDIAN
	char *lpSrc = (char *) Src;
	char *lpDst = (char *) Dst;
	int lpStride = Stride;

	for (dy = 0; dy < 8; dy++) {
		lpDst[0] = lpSrc[0];
		lpDst[1] = lpSrc[1];
		lpDst[2] = lpSrc[2];
		lpDst[3] = lpSrc[3];
		lpDst[4] = lpSrc[4];
		lpDst[5] = lpSrc[5];
		lpDst[6] = lpSrc[6];
		lpDst[7] = lpSrc[7];
		lpSrc += lpStride;
		lpDst += lpStride;
	}
#else
	long *lpSrc = (long *) Src;
	long *lpDst = (long *) Dst;
	int lpStride = Stride >> 2;

	for (dy = 0; dy < 8; dy++) {
		lpDst[0] = lpSrc[0];
		lpDst[1] = lpSrc[1];
		lpSrc += lpStride;
		lpDst += lpStride;
	}
#endif
}
/**/
void CopyBlockHor(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy, dx;

	for (dy = 0; dy < 8; dy++) {
		for (dx = 0; dx < 8; dx++) {
			Dst[dx] = (Src[dx] + Src[dx+1]+1) >> 1; // hor interpolation with rounding
		}
		Src += Stride;
		Dst += Stride;
	}
}
/**/
void CopyBlockVer(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy, dx;

	for (dy = 0; dy < 8; dy++) {
		for (dx = 0; dx < 8; dx++) {
			Dst[dx] = (Src[dx] + Src[dx+Stride] +1) >> 1; // ver interpolation with rounding
		}
		Src += Stride;
		Dst += Stride;
	}
}
/**/
void CopyBlockHorVer(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy, dx;

	for (dy = 0; dy < 8; dy++) {
		for (dx = 0; dx < 8; dx++) {
			Dst[dx] = (Src[dx] + Src[dx+1] + 
								Src[dx+Stride] + Src[dx+Stride+1] +2) >> 2; // horver interpolation with rounding
		}
		Src += Stride;
		Dst += Stride;
	}
}
/**/
void CopyBlockHorRound(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy, dx;



	for (dy = 0; dy < 8; dy++) 
	{


		for (dx = 0; dx < 8; dx++) \
		{
			Dst[dx] = (Src[dx] + Src[dx+1]) >> 1; // hor interpolation with rounding
		}



		Src += Stride;
		Dst += Stride;
	}
}
/**/
void CopyBlockVerRound(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy, dx;

	for (dy = 0; dy < 8; dy++) {
		for (dx = 0; dx < 8; dx++) {
			Dst[dx] = (Src[dx] + Src[dx+Stride]) >> 1; // ver interpolation with rounding
		}
		Src += Stride;
		Dst += Stride;
	}
}
/**/
void CopyBlockHorVerRound(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy, dx;

	for (dy = 0; dy < 8; dy++) {
		for (dx = 0; dx < 8; dx++) {
			Dst[dx] = (Src[dx] + Src[dx+1] + 
								Src[dx+Stride] + Src[dx+Stride+1] +1) >> 2; // horver interpolation with rounding
		}
		Src += Stride;
		Dst += Stride;
	}
}
/** *** **/
void CopyMBlock(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy;

#ifdef BIG_ENDIAN
	char *lpSrc = (char *) Src;
	char *lpDst = (char *) Dst;
	int lpStride = Stride;

	for (dy = 0; dy < 16; dy++) {
		lpDst[0] = lpSrc[0];
		lpDst[1] = lpSrc[1];
		lpDst[2] = lpSrc[2];
		lpDst[3] = lpSrc[3];
		lpDst[4] = lpSrc[4];
		lpDst[5] = lpSrc[5];
		lpDst[6] = lpSrc[6];
		lpDst[7] = lpSrc[7];
		lpDst[8] = lpSrc[8];
		lpDst[9] = lpSrc[9];
		lpDst[10] = lpSrc[10];
		lpDst[11] = lpSrc[11];
		lpDst[12] = lpSrc[12];
		lpDst[13] = lpSrc[13];
		lpDst[14] = lpSrc[14];
		lpDst[15] = lpSrc[15];
		lpSrc += lpStride;
		lpDst += lpStride;
	}
#else
	long *lpSrc = (long *) Src;
	long *lpDst = (long *) Dst;
	int lpStride = Stride >> 2;

	for (dy = 0; dy < 16; dy++) {
		lpDst[0] = lpSrc[0];
		lpDst[1] = lpSrc[1];
		lpDst[2] = lpSrc[2];
		lpDst[3] = lpSrc[3];
		lpSrc += lpStride;
		lpDst += lpStride;
	}
#endif
}
/**/
void CopyMBlockHor(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy, dx;

	for (dy = 0; dy < 16; dy++) {
		for (dx = 0; dx < 16; dx++) {
			Dst[dx] = (Src[dx] + Src[dx+1]+1) >> 1; // hor interpolation with rounding
		}
		Src += Stride;
		Dst += Stride;
	}
}
/**/
void CopyMBlockVer(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy, dx;

	for (dy = 0; dy < 16; dy++) {
		for (dx = 0; dx < 16; dx++) {
			Dst[dx] = (Src[dx] + Src[dx+Stride] +1) >> 1; // ver interpolation with rounding
		}
		Src += Stride;
		Dst += Stride;
	}
}
/**/
void CopyMBlockHorVer(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy, dx;

	for (dy = 0; dy < 16; dy++) {
		for (dx = 0; dx < 16; dx++) {
			Dst[dx] = (Src[dx] + Src[dx+1] + 
								Src[dx+Stride] + Src[dx+Stride+1] +2) >> 2; // horver interpolation with rounding
		}
		Src += Stride;
		Dst += Stride;
	}
}
/**/
void CopyMBlockHorRound(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy, dx;

	for (dy = 0; dy < 16; dy++) {
		for (dx = 0; dx < 16; dx++) {
			Dst[dx] = (Src[dx] + Src[dx+1]) >> 1; // hor interpolation with rounding
		}
		Src += Stride;
		Dst += Stride;
	}
}
/**/
void CopyMBlockVerRound(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy, dx;

	for (dy = 0; dy < 16; dy++) {
		for (dx = 0; dx < 16; dx++) {
			Dst[dx] = (Src[dx] + Src[dx+Stride]) >> 1; // ver interpolation with rounding
		}
		Src += Stride;
		Dst += Stride;
	}
}
/**/
void CopyMBlockHorVerRound(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy, dx;

	for (dy = 0; dy < 16; dy++) {
		for (dx = 0; dx < 16; dx++) {
			Dst[dx] = (Src[dx] + Src[dx+1] + 
								Src[dx+Stride] + Src[dx+Stride+1] +1) >> 2; // horver interpolation with rounding
		}
		Src += Stride;
		Dst += Stride;
	}
}
