/* 
 *
 *  This file is part of libmpeg3
 *	
 *  libmpeg3 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  libmpeg3 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

/*
 * Discrete Cosine Tansform (DCT) for subband synthesis
 * optimized for machines with no auto-increment. 
 * The performance is highly compiler dependend. Maybe
 * the dct64.c version for 'normal' processor may be faster
 * even for Intel processors.
 */

#include "mpeg3private.h"
#include "mpeg3protos.h"
#include "tables.h"

#include <math.h>

int mpeg3audio_dct64_1(float *out0, float *out1, float *b1, float *b2, float *samples)
{
	register float *costab = mpeg3_pnts[0];

	b1[0x00] = samples[0x00] + samples[0x1F];
	b1[0x01] = samples[0x01] + samples[0x1E];
	b1[0x1F] = (samples[0x00] - samples[0x1F]) * costab[0x0];
	b1[0x1E] = (samples[0x01] - samples[0x1E]) * costab[0x1];

	b1[0x02] = samples[0x02] + samples[0x1D];
	b1[0x03] = samples[0x03] + samples[0x1C];
	b1[0x1D] = (samples[0x02] - samples[0x1D]) * costab[0x2];
	b1[0x1C] = (samples[0x03] - samples[0x1C]) * costab[0x3];

	b1[0x04] = samples[0x04] + samples[0x1B];
	b1[0x05] = samples[0x05] + samples[0x1A];
	b1[0x1B] = (samples[0x04] - samples[0x1B]) * costab[0x4];
	b1[0x1A] = (samples[0x05] - samples[0x1A]) * costab[0x5];

	b1[0x06] = samples[0x06] + samples[0x19];
	b1[0x07] = samples[0x07] + samples[0x18];
	b1[0x19] = (samples[0x06] - samples[0x19]) * costab[0x6];
	b1[0x18] = (samples[0x07] - samples[0x18]) * costab[0x7];

	b1[0x08] = samples[0x08] + samples[0x17];
	b1[0x09] = samples[0x09] + samples[0x16];
	b1[0x17] = (samples[0x08] - samples[0x17]) * costab[0x8];
	b1[0x16] = (samples[0x09] - samples[0x16]) * costab[0x9];

	b1[0x0A] = samples[0x0A] + samples[0x15];
	b1[0x0B] = samples[0x0B] + samples[0x14];
	b1[0x15] = (samples[0x0A] - samples[0x15]) * costab[0xA];
	b1[0x14] = (samples[0x0B] - samples[0x14]) * costab[0xB];

	b1[0x0C] = samples[0x0C] + samples[0x13];
	b1[0x0D] = samples[0x0D] + samples[0x12];
	b1[0x13] = (samples[0x0C] - samples[0x13]) * costab[0xC];
	b1[0x12] = (samples[0x0D] - samples[0x12]) * costab[0xD];

	b1[0x0E] = samples[0x0E] + samples[0x11];
	b1[0x0F] = samples[0x0F] + samples[0x10];
	b1[0x11] = (samples[0x0E] - samples[0x11]) * costab[0xE];
	b1[0x10] = (samples[0x0F] - samples[0x10]) * costab[0xF];

	costab = mpeg3_pnts[1];

	b2[0x00] = b1[0x00] + b1[0x0F]; 
	b2[0x01] = b1[0x01] + b1[0x0E]; 
	b2[0x0F] = (b1[0x00] - b1[0x0F]) * costab[0];
	b2[0x0E] = (b1[0x01] - b1[0x0E]) * costab[1];

	b2[0x02] = b1[0x02] + b1[0x0D]; 
	b2[0x03] = b1[0x03] + b1[0x0C]; 
	b2[0x0D] = (b1[0x02] - b1[0x0D]) * costab[2];
	b2[0x0C] = (b1[0x03] - b1[0x0C]) * costab[3];

	b2[0x04] = b1[0x04] + b1[0x0B]; 
	b2[0x05] = b1[0x05] + b1[0x0A]; 
	b2[0x0B] = (b1[0x04] - b1[0x0B]) * costab[4];
	b2[0x0A] = (b1[0x05] - b1[0x0A]) * costab[5];

	b2[0x06] = b1[0x06] + b1[0x09]; 
	b2[0x07] = b1[0x07] + b1[0x08]; 
	b2[0x09] = (b1[0x06] - b1[0x09]) * costab[6];
	b2[0x08] = (b1[0x07] - b1[0x08]) * costab[7];

	/* */

	b2[0x10] = b1[0x10] + b1[0x1F];
	b2[0x11] = b1[0x11] + b1[0x1E];
	b2[0x1F] = (b1[0x1F] - b1[0x10]) * costab[0];
	b2[0x1E] = (b1[0x1E] - b1[0x11]) * costab[1];

	b2[0x12] = b1[0x12] + b1[0x1D];
	b2[0x13] = b1[0x13] + b1[0x1C];
	b2[0x1D] = (b1[0x1D] - b1[0x12]) * costab[2];
	b2[0x1C] = (b1[0x1C] - b1[0x13]) * costab[3];

	b2[0x14] = b1[0x14] + b1[0x1B];
	b2[0x15] = b1[0x15] + b1[0x1A];
	b2[0x1B] = (b1[0x1B] - b1[0x14]) * costab[4];
	b2[0x1A] = (b1[0x1A] - b1[0x15]) * costab[5];

	b2[0x16] = b1[0x16] + b1[0x19];
	b2[0x17] = b1[0x17] + b1[0x18];
	b2[0x19] = (b1[0x19] - b1[0x16]) * costab[6];
	b2[0x18] = (b1[0x18] - b1[0x17]) * costab[7];

 	costab = mpeg3_pnts[2];

	b1[0x00] = b2[0x00] + b2[0x07];
	b1[0x07] = (b2[0x00] - b2[0x07]) * costab[0];
	b1[0x01] = b2[0x01] + b2[0x06];
	b1[0x06] = (b2[0x01] - b2[0x06]) * costab[1];
	b1[0x02] = b2[0x02] + b2[0x05];
	b1[0x05] = (b2[0x02] - b2[0x05]) * costab[2];
	b1[0x03] = b2[0x03] + b2[0x04];
	b1[0x04] = (b2[0x03] - b2[0x04]) * costab[3];

	b1[0x08] = b2[0x08] + b2[0x0F];
	b1[0x0F] = (b2[0x0F] - b2[0x08]) * costab[0];
	b1[0x09] = b2[0x09] + b2[0x0E];
	b1[0x0E] = (b2[0x0E] - b2[0x09]) * costab[1];
	b1[0x0A] = b2[0x0A] + b2[0x0D];
	b1[0x0D] = (b2[0x0D] - b2[0x0A]) * costab[2];
	b1[0x0B] = b2[0x0B] + b2[0x0C];
	b1[0x0C] = (b2[0x0C] - b2[0x0B]) * costab[3];

	b1[0x10] = b2[0x10] + b2[0x17];
	b1[0x17] = (b2[0x10] - b2[0x17]) * costab[0];
	b1[0x11] = b2[0x11] + b2[0x16];
	b1[0x16] = (b2[0x11] - b2[0x16]) * costab[1];
	b1[0x12] = b2[0x12] + b2[0x15];
	b1[0x15] = (b2[0x12] - b2[0x15]) * costab[2];
	b1[0x13] = b2[0x13] + b2[0x14];
	b1[0x14] = (b2[0x13] - b2[0x14]) * costab[3];

	b1[0x18] = b2[0x18] + b2[0x1F];
	b1[0x1F] = (b2[0x1F] - b2[0x18]) * costab[0];
	b1[0x19] = b2[0x19] + b2[0x1E];
	b1[0x1E] = (b2[0x1E] - b2[0x19]) * costab[1];
	b1[0x1A] = b2[0x1A] + b2[0x1D];
	b1[0x1D] = (b2[0x1D] - b2[0x1A]) * costab[2];
	b1[0x1B] = b2[0x1B] + b2[0x1C];
	b1[0x1C] = (b2[0x1C] - b2[0x1B]) * costab[3];

	{
		register float const cos0 = mpeg3_pnts[3][0];
  		register float const cos1 = mpeg3_pnts[3][1];

		b2[0x00] = b1[0x00] + b1[0x03];
		b2[0x03] = (b1[0x00] - b1[0x03]) * cos0;
		b2[0x01] = b1[0x01] + b1[0x02];
		b2[0x02] = (b1[0x01] - b1[0x02]) * cos1;

		b2[0x04] = b1[0x04] + b1[0x07];
		b2[0x07] = (b1[0x07] - b1[0x04]) * cos0;
		b2[0x05] = b1[0x05] + b1[0x06];
		b2[0x06] = (b1[0x06] - b1[0x05]) * cos1;

		b2[0x08] = b1[0x08] + b1[0x0B];
		b2[0x0B] = (b1[0x08] - b1[0x0B]) * cos0;
		b2[0x09] = b1[0x09] + b1[0x0A];
		b2[0x0A] = (b1[0x09] - b1[0x0A]) * cos1;

		b2[0x0C] = b1[0x0C] + b1[0x0F];
		b2[0x0F] = (b1[0x0F] - b1[0x0C]) * cos0;
		b2[0x0D] = b1[0x0D] + b1[0x0E];
		b2[0x0E] = (b1[0x0E] - b1[0x0D]) * cos1;

		b2[0x10] = b1[0x10] + b1[0x13];
		b2[0x13] = (b1[0x10] - b1[0x13]) * cos0;
		b2[0x11] = b1[0x11] + b1[0x12];
		b2[0x12] = (b1[0x11] - b1[0x12]) * cos1;

		b2[0x14] = b1[0x14] + b1[0x17];
		b2[0x17] = (b1[0x17] - b1[0x14]) * cos0;
		b2[0x15] = b1[0x15] + b1[0x16];
		b2[0x16] = (b1[0x16] - b1[0x15]) * cos1;

		b2[0x18] = b1[0x18] + b1[0x1B];
		b2[0x1B] = (b1[0x18] - b1[0x1B]) * cos0;
		b2[0x19] = b1[0x19] + b1[0x1A];
		b2[0x1A] = (b1[0x19] - b1[0x1A]) * cos1;

		b2[0x1C] = b1[0x1C] + b1[0x1F];
		b2[0x1F] = (b1[0x1F] - b1[0x1C]) * cos0;
		b2[0x1D] = b1[0x1D] + b1[0x1E];
		b2[0x1E] = (b1[0x1E] - b1[0x1D]) * cos1;
 	}

 	{
		register float const cos0 = mpeg3_pnts[4][0];

		b1[0x00] = b2[0x00] + b2[0x01];
		b1[0x01] = (b2[0x00] - b2[0x01]) * cos0;
		b1[0x02] = b2[0x02] + b2[0x03];
		b1[0x03] = (b2[0x03] - b2[0x02]) * cos0;
		b1[0x02] += b1[0x03];

		b1[0x04] = b2[0x04] + b2[0x05];
		b1[0x05] = (b2[0x04] - b2[0x05]) * cos0;
		b1[0x06] = b2[0x06] + b2[0x07];
		b1[0x07] = (b2[0x07] - b2[0x06]) * cos0;
		b1[0x06] += b1[0x07];
		b1[0x04] += b1[0x06];
		b1[0x06] += b1[0x05];
		b1[0x05] += b1[0x07];

		b1[0x08] = b2[0x08] + b2[0x09];
		b1[0x09] = (b2[0x08] - b2[0x09]) * cos0;
		b1[0x0A] = b2[0x0A] + b2[0x0B];
		b1[0x0B] = (b2[0x0B] - b2[0x0A]) * cos0;
		b1[0x0A] += b1[0x0B];

		b1[0x0C] = b2[0x0C] + b2[0x0D];
		b1[0x0D] = (b2[0x0C] - b2[0x0D]) * cos0;
		b1[0x0E] = b2[0x0E] + b2[0x0F];
		b1[0x0F] = (b2[0x0F] - b2[0x0E]) * cos0;
		b1[0x0E] += b1[0x0F];
		b1[0x0C] += b1[0x0E];
		b1[0x0E] += b1[0x0D];
		b1[0x0D] += b1[0x0F];

		b1[0x10] = b2[0x10] + b2[0x11];
		b1[0x11] = (b2[0x10] - b2[0x11]) * cos0;
		b1[0x12] = b2[0x12] + b2[0x13];
		b1[0x13] = (b2[0x13] - b2[0x12]) * cos0;
		b1[0x12] += b1[0x13];

		b1[0x14] = b2[0x14] + b2[0x15];
		b1[0x15] = (b2[0x14] - b2[0x15]) * cos0;
		b1[0x16] = b2[0x16] + b2[0x17];
		b1[0x17] = (b2[0x17] - b2[0x16]) * cos0;
		b1[0x16] += b1[0x17];
		b1[0x14] += b1[0x16];
		b1[0x16] += b1[0x15];
		b1[0x15] += b1[0x17];

		b1[0x18] = b2[0x18] + b2[0x19];
		b1[0x19] = (b2[0x18] - b2[0x19]) * cos0;
		b1[0x1A] = b2[0x1A] + b2[0x1B];
		b1[0x1B] = (b2[0x1B] - b2[0x1A]) * cos0;
		b1[0x1A] += b1[0x1B];

		b1[0x1C] = b2[0x1C] + b2[0x1D];
		b1[0x1D] = (b2[0x1C] - b2[0x1D]) * cos0;
		b1[0x1E] = b2[0x1E] + b2[0x1F];
		b1[0x1F] = (b2[0x1F] - b2[0x1E]) * cos0;
		b1[0x1E] += b1[0x1F];
		b1[0x1C] += b1[0x1E];
		b1[0x1E] += b1[0x1D];
		b1[0x1D] += b1[0x1F];
 	}

	out0[0x10*16] = b1[0x00];
	out0[0x10*12] = b1[0x04];
	out0[0x10* 8] = b1[0x02];
	out0[0x10* 4] = b1[0x06];
	out0[0x10* 0] = b1[0x01];
	out1[0x10* 0] = b1[0x01];
	out1[0x10* 4] = b1[0x05];
	out1[0x10* 8] = b1[0x03];
	out1[0x10*12] = b1[0x07];

	out0[0x10*14] = b1[0x08] + b1[0x0C];
	out0[0x10*10] = b1[0x0C] + b1[0x0a];
	out0[0x10* 6] = b1[0x0A] + b1[0x0E];
	out0[0x10* 2] = b1[0x0E] + b1[0x09];
	out1[0x10* 2] = b1[0x09] + b1[0x0D];
	out1[0x10* 6] = b1[0x0D] + b1[0x0B];
	out1[0x10*10] = b1[0x0B] + b1[0x0F];
	out1[0x10*14] = b1[0x0F];

	{ 
		register float tmp;
		tmp = b1[0x18] + b1[0x1C];
		out0[0x10*15] = tmp + b1[0x10];
		out0[0x10*13] = tmp + b1[0x14];
		tmp = b1[0x1C] + b1[0x1A];
		out0[0x10*11] = tmp + b1[0x14];
		out0[0x10* 9] = tmp + b1[0x12];
		tmp = b1[0x1A] + b1[0x1E];
		out0[0x10* 7] = tmp + b1[0x12];
		out0[0x10* 5] = tmp + b1[0x16];
		tmp = b1[0x1E] + b1[0x19];
		out0[0x10* 3] = tmp + b1[0x16];
		out0[0x10* 1] = tmp + b1[0x11];
		tmp = b1[0x19] + b1[0x1D];
		out1[0x10* 1] = tmp + b1[0x11];
		out1[0x10* 3] = tmp + b1[0x15]; 
		tmp = b1[0x1D] + b1[0x1B];
		out1[0x10* 5] = tmp + b1[0x15];
		out1[0x10* 7] = tmp + b1[0x13];
		tmp = b1[0x1B] + b1[0x1F];
		out1[0x10* 9] = tmp + b1[0x13];
		out1[0x10*11] = tmp + b1[0x17];
		out1[0x10*13] = b1[0x17] + b1[0x1F];
		out1[0x10*15] = b1[0x1F];
	}
	return 0;
}

/*
 * the call via dct64 is a trick to force GCC to use
 * (new) registers for the b1,b2 pointer to the bufs[xx] field
 */
int mpeg3audio_dct64(float *a, float *b, float *c)
{
	float bufs[0x40];
	return mpeg3audio_dct64_1(a, b, bufs, bufs + 0x20, c);
}

/*//////////////////////////////////////////////////////////////// */
/* */
/* 9 Point Inverse Discrete Cosine Transform */
/* */
/* This piece of code is Copyright 1997 Mikko Tommila and is freely usable */
/* by anybody. The algorithm itself is of course in the public domain. */
/* */
/* Again derived heuristically from the 9-point WFTA. */
/* */
/* The algorithm is optimized (?) for speed, not for small rounding errors or */
/* good readability. */
/* */
/* 36 additions, 11 multiplications */
/* */
/* Again this is very likely sub-optimal. */
/* */
/* The code is optimized to use a minimum number of temporary variables, */
/* so it should compile quite well even on 8-register Intel x86 processors. */
/* This makes the code quite obfuscated and very difficult to understand. */
/* */
/* References: */
/* [1] S. Winograd: "On Computing the Discrete Fourier Transform", */
/*     Mathematics of Computation, Volume 32, Number 141, January 1978, */
/*     Pages 175-199 */


/*------------------------------------------------------------------*/
/*                                                                  */
/*    Function: Calculation of the inverse MDCT                     */
/*                                                                  */
/*------------------------------------------------------------------*/

int mpeg3audio_dct36(float *inbuf, float *o1, float *o2, float *wintab, float *tsbuf)
{
    float tmp[18];

	{
    	register float *in = inbuf;

    	in[17]+=in[16]; in[16]+=in[15]; in[15]+=in[14];
    	in[14]+=in[13]; in[13]+=in[12]; in[12]+=in[11];
    	in[11]+=in[10]; in[10]+=in[9];  in[9] +=in[8];
    	in[8] +=in[7];  in[7] +=in[6];  in[6] +=in[5];
    	in[5] +=in[4];  in[4] +=in[3];  in[3] +=in[2];
    	in[2] +=in[1];  in[1] +=in[0];

    	in[17]+=in[15]; in[15]+=in[13]; in[13]+=in[11]; in[11]+=in[9];
    	in[9] +=in[7];  in[7] +=in[5];  in[5] +=in[3];  in[3] +=in[1];


    	{
    		float t3;
    		{ 
    			float t0, t1, t2;

    			t0 = mpeg3_COS6_2 * (in[8] + in[16] - in[4]);
    			t1 = mpeg3_COS6_2 * in[12];

    			t3 = in[0];
    			t2 = t3 - t1 - t1;
    			tmp[1] = tmp[7] = t2 - t0;
    			tmp[4]          = t2 + t0 + t0;
    			t3 += t1;

    			t2 = mpeg3_COS6_1 * (in[10] + in[14] - in[2]);
    			tmp[1] -= t2;
    			tmp[7] += t2;
    		}
    		{
    			float t0, t1, t2;

    			t0 = mpeg3_cos9[0] * (in[4] + in[8] );
    			t1 = mpeg3_cos9[1] * (in[8] - in[16]);
    			t2 = mpeg3_cos9[2] * (in[4] + in[16]);

    			tmp[2] = tmp[6] = t3 - t0      - t2;
    			tmp[0] = tmp[8] = t3 + t0 + t1;
    			tmp[3] = tmp[5] = t3      - t1 + t2;
    		}
    	}
    	{
    		float t1, t2, t3;

    		t1 = mpeg3_cos18[0] * (in[2]  + in[10]);
    		t2 = mpeg3_cos18[1] * (in[10] - in[14]);
    		t3 = mpeg3_COS6_1   * in[6];

    		{
        		float t0 = t1 + t2 + t3;
        		tmp[0] += t0;
        		tmp[8] -= t0;
    		}

    		t2 -= t3;
    		t1 -= t3;

    		t3 = mpeg3_cos18[2] * (in[2] + in[14]);

    		t1 += t3;
    		tmp[3] += t1;
    		tmp[5] -= t1;

    		t2 -= t3;
    		tmp[2] += t2;
    		tmp[6] -= t2;
    	}


    	{
    		float t0, t1, t2, t3, t4, t5, t6, t7;

    		t1 = mpeg3_COS6_2 * in[13];
    		t2 = mpeg3_COS6_2 * (in[9] + in[17] - in[5]);

    		t3 = in[1] + t1;
    		t4 = in[1] - t1 - t1;
    		t5 = t4 - t2;

    		t0 = mpeg3_cos9[0] * (in[5] + in[9]);
    		t1 = mpeg3_cos9[1] * (in[9] - in[17]);

    		tmp[13] = (t4 + t2 + t2) * mpeg3_tfcos36[17-13];
    		t2 = mpeg3_cos9[2] * (in[5] + in[17]);

    		t6 = t3 - t0 - t2;
    		t0 += t3 + t1;
    		t3 += t2 - t1;

    		t2 = mpeg3_cos18[0] * (in[3]  + in[11]);
    		t4 = mpeg3_cos18[1] * (in[11] - in[15]);
    		t7 = mpeg3_COS6_1 * in[7];

    		t1 = t2 + t4 + t7;
    		tmp[17] = (t0 + t1) * mpeg3_tfcos36[17-17];
    		tmp[9]  = (t0 - t1) * mpeg3_tfcos36[17-9];
    		t1 = mpeg3_cos18[2] * (in[3] + in[15]);
    		t2 += t1 - t7;

    		tmp[14] = (t3 + t2) * mpeg3_tfcos36[17-14];
    		t0 = mpeg3_COS6_1 * (in[11] + in[15] - in[3]);
    		tmp[12] = (t3 - t2) * mpeg3_tfcos36[17-12];

    		t4 -= t1 + t7;

    		tmp[16] = (t5 - t0) * mpeg3_tfcos36[17-16];
    		tmp[10] = (t5 + t0) * mpeg3_tfcos36[17-10];
    		tmp[15] = (t6 + t4) * mpeg3_tfcos36[17-15];
    		tmp[11] = (t6 - t4) * mpeg3_tfcos36[17-11];
	    }

#define MACRO(v) \
	{ \
    	float tmpval; \
    	tmpval = tmp[(v)] + tmp[17-(v)]; \
    	out2[9+(v)] = tmpval * w[27+(v)]; \
    	out2[8-(v)] = tmpval * w[26-(v)]; \
    	tmpval = tmp[(v)] - tmp[17-(v)]; \
    	ts[SBLIMIT*(8-(v))] = out1[8-(v)] + tmpval * w[8-(v)]; \
    	ts[SBLIMIT*(9+(v))] = out1[9+(v)] + tmpval * w[9+(v)]; \
	}

		{
			register float *out2 = o2;
			register float *w = wintab;
			register float *out1 = o1;
			register float *ts = tsbuf;

			MACRO(0);
			MACRO(1);
			MACRO(2);
			MACRO(3);
			MACRO(4);
			MACRO(5);
			MACRO(6);
			MACRO(7);
			MACRO(8);
		}
	}
	return 0;
}

/*
 * new DCT12
 */
int mpeg3audio_dct12(float *in,float *rawout1,float *rawout2,register float *wi,register float *ts)
{
#define DCT12_PART1 \
            in5 = in[5*3]; \
    in5 += (in4 = in[4*3]); \
    in4 += (in3 = in[3*3]); \
    in3 += (in2 = in[2*3]); \
    in2 += (in1 = in[1*3]); \
    in1 += (in0 = in[0*3]); \
                            \
    in5 += in3; in3 += in1; \
                            \
    in2 *= mpeg3_COS6_1; \
    in3 *= mpeg3_COS6_1; \

#define DCT12_PART2 \
	in0 += in4 * mpeg3_COS6_2; \
                    	 \
	in4 = in0 + in2;     \
	in0 -= in2;          \
                    	 \
	in1 += in5 * mpeg3_COS6_2; \
                    	 \
	in5 = (in1 + in3) * mpeg3_tfcos12[0]; \
	in1 = (in1 - in3) * mpeg3_tfcos12[2]; \
                    	\
	in3 = in4 + in5;    \
	in4 -= in5;         \
                    	\
	in2 = in0 + in1;    \
	in0 -= in1;


	{
    	float in0,in1,in2,in3,in4,in5;
    	register float *out1 = rawout1;
    	ts[SBLIMIT*0] = out1[0]; ts[SBLIMIT*1] = out1[1]; ts[SBLIMIT*2] = out1[2];
    	ts[SBLIMIT*3] = out1[3]; ts[SBLIMIT*4] = out1[4]; ts[SBLIMIT*5] = out1[5];

    	DCT12_PART1

    	{
    		float tmp0,tmp1 = (in0 - in4);
    		{
        		float tmp2 = (in1 - in5) * mpeg3_tfcos12[1];
        		tmp0 = tmp1 + tmp2;
        		tmp1 -= tmp2;
    		}
    		ts[(17-1)*SBLIMIT] = out1[17-1] + tmp0 * wi[11-1];
    		ts[(12+1)*SBLIMIT] = out1[12+1] + tmp0 * wi[6+1];
    		ts[(6 +1)*SBLIMIT] = out1[6 +1] + tmp1 * wi[1];
    		ts[(11-1)*SBLIMIT] = out1[11-1] + tmp1 * wi[5-1];
    	}

    	DCT12_PART2

    	ts[(17-0)*SBLIMIT] = out1[17-0] + in2 * wi[11-0];
    	ts[(12+0)*SBLIMIT] = out1[12+0] + in2 * wi[6+0];
    	ts[(12+2)*SBLIMIT] = out1[12+2] + in3 * wi[6+2];
    	ts[(17-2)*SBLIMIT] = out1[17-2] + in3 * wi[11-2];

    	ts[(6+0)*SBLIMIT]  = out1[6+0] + in0 * wi[0];
    	ts[(11-0)*SBLIMIT] = out1[11-0] + in0 * wi[5-0];
    	ts[(6+2)*SBLIMIT]  = out1[6+2] + in4 * wi[2];
    	ts[(11-2)*SBLIMIT] = out1[11-2] + in4 * wi[5-2];
    }

	in++;

	{
    	 float in0,in1,in2,in3,in4,in5;
    	 register float *out2 = rawout2;

    	 DCT12_PART1

    	 {
    		 float tmp0,tmp1 = (in0 - in4);
    		 {
        		 float tmp2 = (in1 - in5) * mpeg3_tfcos12[1];
        		 tmp0 = tmp1 + tmp2;
        		 tmp1 -= tmp2;
    		 }
    		 out2[5-1] = tmp0 * wi[11-1];
    		 out2[0+1] = tmp0 * wi[6+1];
    		 ts[(12+1)*SBLIMIT] += tmp1 * wi[1];
    		 ts[(17-1)*SBLIMIT] += tmp1 * wi[5-1];
    	 }

    	 DCT12_PART2

    	 out2[5-0] = in2 * wi[11-0];
    	 out2[0+0] = in2 * wi[6+0];
    	 out2[0+2] = in3 * wi[6+2];
    	 out2[5-2] = in3 * wi[11-2];

    	 ts[(12+0)*SBLIMIT] += in0 * wi[0];
    	 ts[(17-0)*SBLIMIT] += in0 * wi[5-0];
    	 ts[(12+2)*SBLIMIT] += in4 * wi[2];
    	 ts[(17-2)*SBLIMIT] += in4 * wi[5-2];
	}

    in++; 

	{
    	float in0,in1,in2,in3,in4,in5;
    	register float *out2 = rawout2;
    	out2[12]=out2[13]=out2[14]=out2[15]=out2[16]=out2[17]=0.0;

    	DCT12_PART1

    	{
    		float tmp0,tmp1 = (in0 - in4);
    		{
        		float tmp2 = (in1 - in5) * mpeg3_tfcos12[1];
        		tmp0 = tmp1 + tmp2;
        		tmp1 -= tmp2;
    		}
    		out2[11-1] = tmp0 * wi[11-1];
    		out2[6 +1] = tmp0 * wi[6+1];
    		out2[0+1] += tmp1 * wi[1];
    		out2[5-1] += tmp1 * wi[5-1];
    	}

    	DCT12_PART2

    	out2[11-0] = in2 * wi[11-0];
    	out2[6 +0] = in2 * wi[6+0];
    	out2[6 +2] = in3 * wi[6+2];
    	out2[11-2] = in3 * wi[11-2];

    	out2[0+0] += in0 * wi[0];
    	out2[5-0] += in0 * wi[5-0];
    	out2[0+2] += in4 * wi[2];
    	out2[5-2] += in4 * wi[5-2];
	}
	return 0;
}
