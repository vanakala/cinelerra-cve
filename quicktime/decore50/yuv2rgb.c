/**************************************************************************
 *                                                                        *
 * This code has been developed by John Funnell. This software is an      *
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
 * John Funnell 
 * Andrea Graziani
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// yuv2rgb.c //

#include <memory.h>

#include "portab.h"
#include "yuv2rgb.h"

/**
 *
**/


void (*convert_yuv)(unsigned char *puc_y, int stride_y,
	unsigned char *puc_u, unsigned char *puc_v, int stride_uv,
	unsigned char *bmp[], int width_y, int height_y,
	unsigned int stride_out);

/**
 *
**/

/***

  /  2568      0   3343  \              
 |   2568  -0c92  -1a1e   | / 65536 * 8 
  \  2568   40cf      0  /              

    Y -= 16;
    U -= 128;
    V -= 128;

    R = (0x2568*Y + 0x0000*V + 0x3343*U) / 0x2000;
    G = (0x2568*Y - 0x0c92*V - 0x1a1e*U) / 0x2000;
    B = (0x2568*Y + 0x40cf*V + 0x0000*U) / 0x2000;

    R = R>255 ? 255 : R;
    R = R<0   ?   0 : R;

    G = G>255 ? 255 : G;
    G = G<0   ?   0 : G;

    B = B>255 ? 255 : B;
    B = B<0   ?   0 : B;

***/

#define _S(a)		(a)>255 ? 255 : (a)<0 ? 0 : (a)

#define _R(y,u,v) (0x2568*(y)  			       + 0x3343*(u)) /0x2000
#define _G(y,u,v) (0x2568*(y) - 0x0c92*(v) - 0x1a1e*(u)) /0x2000
#define _B(y,u,v) (0x2568*(y) + 0x40cf*(v))					     /0x2000

struct lookuptable
{
    int32_t m_plY[256];
    int32_t m_plRV[256];
    int32_t m_plGV[256];
    int32_t m_plGU[256];
    int32_t m_plBU[256];
};
static struct lookuptable lut;
void init_yuv2rgb()
{
    int i;
    for(i=0; i<256; i++)
    {
	if(i>=16)
	    if(i>240)
		lut.m_plY[i]=lut.m_plY[240];
	    else
		lut.m_plY[i]=298*(i-16);
	else
	    lut.m_plY[i]=0;
	if((i>=16) && (i<=240))
	{
	    lut.m_plRV[i]=408*(i-128);
	    lut.m_plGV[i]=-208*(i-128);
	    lut.m_plGU[i]=-100*(i-128);
	    lut.m_plBU[i]=517*(i-128);
	}
	else if(i<16)
	{
	    lut.m_plRV[i]=408*(16-128);
	    lut.m_plGV[i]=-208*(16-128);
	    lut.m_plGU[i]=-100*(16-128);
	    lut.m_plBU[i]=517*(16-128);
	}
	else
	{
	    lut.m_plRV[i]=lut.m_plRV[240];
	    lut.m_plGV[i]=lut.m_plGV[240];
	    lut.m_plGU[i]=lut.m_plGU[240];
	    lut.m_plBU[i]=lut.m_plBU[240];
	}
    }
}

/* all stride values are in _pixels_ */

void yuv2rgb_32(uint8_t *puc_y, int stride_y, 
                uint8_t *puc_u, uint8_t *puc_v, int stride_uv, 
                uint8_t *puc_out, int width_y, int height_y,
								unsigned int _stride_out) 
{

	int x, y;
	int stride_diff = 4 * (_stride_out - width_y);

	if (height_y < 0) {
		/* we are flipping our output upside-down */
		height_y  = -height_y;
		puc_y     += (height_y   - 1) * stride_y ;
		puc_u     += (height_y/2 - 1) * stride_uv;
		puc_v     += (height_y/2 - 1) * stride_uv;
		stride_y  = -stride_y;
		stride_uv = -stride_uv;
	}

	for (y=0; y<height_y; y++) 
	{
		for (x=0; x<width_y; x++)
		{
			signed int _r,_g,_b; 
			signed int r, g, b;
			signed int y, u, v;

			y = puc_y[x] - 16;
			u = puc_u[x>>1] - 128;
			v = puc_v[x>>1] - 128;

			_r = _R(y,u,v);
			_g = _G(y,u,v);
			_b = _B(y,u,v);

			r = _S(_r);
			g = _S(_g);
			b = _S(_b);

			puc_out[0] = r;
			puc_out[1] = g;
			puc_out[2] = b;
			puc_out[3] = 0;

			puc_out+=4;
		}

		puc_y   += stride_y;
		if (y%2) {
			puc_u   += stride_uv;
			puc_v   += stride_uv;
		}
		puc_out += stride_diff;
	}
}

/***/

// This be done more efficiently 
// ( we spend almost as much time only in here
// as DivX 3.11 spends on all decoding )
void yuv2rgb_24(uint8_t *puc_y, int stride_y, 
                uint8_t *puc_u, uint8_t *puc_v, int stride_uv, 
                uint8_t *puc_out, int width_y, int height_y,
								unsigned int _stride_out) 
{

	int x, y;
	int stride_diff = 6*_stride_out - 3*width_y;

	if (height_y < 0) {
		/* we are flipping our output upside-down */
		height_y  = -height_y;
		puc_y     += (height_y   - 1) * stride_y ;
		puc_u     += (height_y/2 - 1) * stride_uv;
		puc_v     += (height_y/2 - 1) * stride_uv;
		stride_y  = -stride_y;
		stride_uv = -stride_uv;
	}

	for (y=0; y<height_y; y+=2) 
	{
		uint8_t* pY=puc_y;
		uint8_t* pY1=puc_y+stride_y;
		uint8_t* pU=puc_u;
		uint8_t* pV=puc_v;
		uint8_t* pOut2=puc_out+3*_stride_out;
		for (x=0; x<width_y; x+=2)
		{
			int R, G, B;
			int Y;
			unsigned int tmp;
			R=lut.m_plRV[*pU];
			G=lut.m_plGV[*pU];
			pU++;
			G+=lut.m_plGU[*pV];
			B=lut.m_plBU[*pV];
			pV++;
#define PUT_COMPONENT(p,v,i) 	\
    tmp=(unsigned int)(v); 	\
    if(tmp < 0x10000) 		\
	p[i]=tmp>>8; 		\
    else			\
	p[i]=(tmp >> 24) ^ 0xff; 
			Y=lut.m_plY[*pY];
			pY++;
			PUT_COMPONENT(puc_out, R+Y, 0);
			PUT_COMPONENT(puc_out, G+Y, 1);
			PUT_COMPONENT(puc_out, B+Y, 2);
			Y=lut.m_plY[*pY];
			pY++;
			PUT_COMPONENT(puc_out, R+Y, 3);
			PUT_COMPONENT(puc_out, G+Y, 4);
			PUT_COMPONENT(puc_out, B+Y, 5);
			Y=lut.m_plY[*pY1];
			pY1++;
			PUT_COMPONENT(pOut2, R+Y, 0);
			PUT_COMPONENT(pOut2, G+Y, 1);
			PUT_COMPONENT(pOut2, B+Y, 2);
			Y=lut.m_plY[*pY1];
			pY1++;
			PUT_COMPONENT(pOut2, R+Y, 3);
			PUT_COMPONENT(pOut2, G+Y, 4);
			PUT_COMPONENT(pOut2, B+Y, 5);
			puc_out+=6;
			pOut2+=6;
		}

		puc_y   += 2*stride_y;
		puc_u   += stride_uv;
		puc_v   += stride_uv;
		puc_out += stride_diff;
	}
}

/***/

#define _S(a)		(a)>255 ? 255 : (a)<0 ? 0 : (a)

#define _R(y,u,v) (0x2568*(y)  			       + 0x3343*(u)) /0x2000
#define _G(y,u,v) (0x2568*(y) - 0x0c92*(v) - 0x1a1e*(u)) /0x2000
#define _B(y,u,v) (0x2568*(y) + 0x40cf*(v))					     /0x2000

#define _mR	0x7c00
#define _mG 0x03e0
#define _mB 0x001f

#define _Ps555(r,g,b) (((r) << 7) & _mR) | (((g) << 2) & _mG) | (((b) >> 3) & _mB)

#define _Ps565(r,g,b) ( ((r & 0xF8) >> 3) | (((g & 0xF8) << 3)) | (((b & 0xF8) << 8)) )

void yuv2rgb_555(uint8_t *puc_y, int stride_y, 
                uint8_t *puc_u, uint8_t *puc_v, int stride_uv, 
                uint8_t *puc_out, int width_y, int height_y,
								unsigned int _stride_out) 
{

	int x, y;
	unsigned short *pus_out;
	int stride_diff = _stride_out - width_y; // expressed in short

	if (height_y < 0) {
		/* we are flipping our output upside-down */
		height_y  = -height_y;
		puc_y     += (height_y   - 1) * stride_y ;
		puc_u     += (height_y/2 - 1) * stride_uv;
		puc_v     += (height_y/2 - 1) * stride_uv;
		stride_y  = -stride_y;
		stride_uv = -stride_uv;
	}

	pus_out = (unsigned short *) puc_out;

	for (y=0; y<height_y; y++) 
	{
		for (x=0; x<width_y; x++)
		{
			signed int _r,_g,_b; 
			signed int r, g, b;
			signed int y, u, v;

			y = puc_y[x] - 16;
			u = puc_u[x>>1] - 128;
			v = puc_v[x>>1] - 128;

			_r = _R(y,u,v);
			_g = _G(y,u,v);
			_b = _B(y,u,v);

			r = _S(_r);
			g = _S(_g);
			b = _S(_b);

			pus_out[0] = _Ps555(b,g,r);

			pus_out++;
		}

		puc_y   += stride_y;
		if (y%2) {
			puc_u   += stride_uv;
			puc_v   += stride_uv;
		}
		pus_out += stride_diff;
	}
}

/***/

void yuv2rgb_565(uint8_t *puc_y, int stride_y, 
                uint8_t *puc_u, uint8_t *puc_v, int stride_uv, 
                uint8_t *puc_out, int width_y, int height_y,
								unsigned int _stride_out) 
{

	int x, y;
	unsigned short *pus_out;
	int stride_diff = _stride_out - width_y; // expressed in short

	if (height_y < 0) {
		/* we are flipping our output upside-down */
		height_y  = -height_y;
		puc_y     += (height_y   - 1) * stride_y ;
		puc_u     += (height_y/2 - 1) * stride_uv;
		puc_v     += (height_y/2 - 1) * stride_uv;
		stride_y  = -stride_y;
		stride_uv = -stride_uv;
	}
	pus_out = (unsigned short *) puc_out;

	for (y=0; y<height_y; y++) 
	{
		for (x=0; x<width_y; x++)
		{
			signed int _r,_g,_b; 
			signed int r, g, b;
			signed int y, u, v;

			y = puc_y[x] - 16;
			u = puc_u[x>>1] - 128;
			v = puc_v[x>>1] - 128;

			_r = _R(y,u,v);
			_g = _G(y,u,v);
			_b = _B(y,u,v);

			r = _S(_r);
			g = _S(_g);
			b = _S(_b);

			pus_out[0] = (unsigned short) _Ps565(r,g,b);

			pus_out++;
		}

		puc_y   += stride_y;
		if (y%2) {
			puc_u   += stride_uv;
			puc_v   += stride_uv;
		}
		pus_out += stride_diff;
	}
}

/***/

// conversion from 4:2:0 to yuv2, 16 bit yuv output
//
void yuy2_out(uint8_t *puc_y, int stride_y, 
  uint8_t *puc_u, uint8_t *puc_v, int stride_uv, 
  uint8_t *puc_out, int width_y, int height_y,
	unsigned int stride_out) 
{ 
	int y;
	uint8_t* puc_out2;
	unsigned int stride_diff = 4 * stride_out - 2 * width_y; // expressed in bytes

	if (height_y < 0) {
		/* we are flipping our output upside-down */
		height_y  = -height_y;
		puc_y     += (height_y   - 1) * stride_y ;
		puc_u     += (height_y/2 - 1) * stride_uv;
		puc_v     += (height_y/2 - 1) * stride_uv;
		stride_y  = -stride_y;
		stride_uv = -stride_uv;
	}
	puc_out2 = puc_out + 2 * stride_out;
	for (y=height_y/2; y; y--) {
		register uint8_t *py, *py2, *pu, *pv;
		register int x;
		uint32_t tmp;

		py = puc_y;
		py2 = puc_y + stride_y;
		pu = puc_u;
		pv = puc_v;
		for (x=width_y/2; x; x--) {
			tmp = *(py++);
			tmp |= *(pu++) << 8;
			tmp |= *(py++) << 16;
			tmp |= *(pv++) << 24;
			*(uint32_t*)puc_out=tmp;
			puc_out += 4;

			tmp &= 0xFF00FF00;
			tmp |= *(py2++);
			tmp |= *(py2++) << 16;
			*(uint32_t*)puc_out2=tmp;
			puc_out2 += 4;
		}

		puc_y += 2*stride_y;
		puc_u += stride_uv;
		puc_v += stride_uv;

		puc_out += stride_diff;
		puc_out2 += stride_diff;
	}

}

/*** YUV 4:2:0 -> UYVY ***/

void uyvy_out(uint8_t *puc_y, int stride_y, 
  uint8_t *puc_u, uint8_t *puc_v, int stride_uv, 
  uint8_t *puc_out, int width_y, int height_y,
	unsigned int stride_out) 
{ 
	int y;
	uint8_t* puc_out2;
	unsigned int stride_diff = 4 * stride_out - 2 * width_y; // expressed in bytes

	if (height_y < 0) {
		/* we are flipping our output upside-down */
		height_y  = -height_y;
		puc_y     += (height_y   - 1) * stride_y ;
		puc_u     += (height_y/2 - 1) * stride_uv;
		puc_v     += (height_y/2 - 1) * stride_uv;
		stride_y  = -stride_y;
		stride_uv = -stride_uv;
	}
	puc_out2 = puc_out + 2 * stride_out;
	for (y=height_y/2; y; y--) {
		register uint8_t *py, *py2, *pu, *pv;
		register int x;
		uint32_t tmp;

		py = puc_y;
		py2 = puc_y + stride_y;
		pu = puc_u;
		pv = puc_v;
		for (x=width_y/2; x; x--) {
			tmp = *(pu++);
			tmp |= *(py++) << 8;
			tmp |= *(pv++) << 16;
			tmp |= *(py++) << 24;
			*(uint32_t*)puc_out=tmp;
			puc_out += 4;

			tmp &= 0x00FF00FF;
			tmp |= *(py2++) << 8;
			tmp |= *(py2++) << 24;
			*(uint32_t*)puc_out2=tmp;
			puc_out2 += 4;
		}

		puc_y += 2*stride_y;
		puc_u += stride_uv;
		puc_v += stride_uv;

		puc_out += stride_diff;
		puc_out2 += stride_diff;
	}

}


// conversion from 4:2:0 to yv2, 12 bit (just remove the edges)
//
void yuv12_out(uint8_t *puc_y, int stride_y, 
  uint8_t *puc_u, uint8_t *puc_v, int stride_uv, 
  uint8_t *puc_out[], int width_y, int height_y,
	unsigned int stride_out) 
{ 
	int i;

	unsigned char * pauc_out[3];

	if (height_y < 0) {
		// we are flipping our output upside-down
		height_y = -height_y;
		puc_y     += (height_y   - 1) * stride_y ;
		puc_u     += (height_y/2 - 1) * stride_uv;
		puc_v     += (height_y/2 - 1) * stride_uv;
		stride_y  = -stride_y;
		stride_uv = -stride_uv;
	}

	pauc_out[0] = puc_out[0];
	pauc_out[1] = puc_out[1];
	pauc_out[2] = puc_out[2];

//printf("yuv12_out 1 %d %d\n", stride_out, height_y);
	for (i=0; i<height_y; i++) {
		
		memcpy(pauc_out[0], puc_y, width_y);

		pauc_out[0] += stride_out;
		puc_y += stride_y;
	}

	for (i=0; i<height_y/2; i++) {
		
		memcpy(pauc_out[1], puc_u, width_y/2); // U and V buffer must be inverted
		memcpy(pauc_out[2], puc_v, width_y/2);

		pauc_out[1] += stride_out/2;
		pauc_out[2] += stride_out/2;
		puc_u += stride_uv;
		puc_v += stride_uv;
	}
//printf("yuv12_out 2\n");
}

