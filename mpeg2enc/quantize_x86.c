/* quantize_x86.c Quantization / inverse quantization    
   In compiler (gcc) embdeed assmbley language...
*/

/* Copyright (C) 2000 Andrew Stevens */

/* This program is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */



/* 
 * 3DNow version of
 * Quantisation for non-intra blocks using Test Model 5 quantization
 *
 * this quantizer has a bias of 1/8 stepsize towards zero
 * (except for the DC coefficient)
 *
 *	PRECONDITION: src dst point to *disinct* memory buffers...
 *	              of block_count *adjacent* int16_t[64] arrays...
 *
 * RETURN: 1 If non-zero coefficients left after quantisaion 0 otherwise
 */

#include "config.h"
#include <stdio.h>
#include <math.h>
#include <fenv.h>
#include "global.h"
#include "cpu_accel.h"
#include "simd.h"
#include "attributes.h"
#include "mmx.h"

/* 
 * Quantisation for non-intra blocks 
 *
 * Various versions for various SIMD instruction sets.  Not all of them
 * bother to implement the test model 5 quantisation of the reference source
 * (this has a bias of 1/8 stepsize towards zero - except for the DC coefficient).
 *
 * Actually, as far as I can tell even the reference source doesn't quite do it
 * for non-intra (though it *does* for intra).
 * 
 * Careful analysis of the code also suggests what it actually does is truncate
 * with a modest bias towards 1 (the d>>2 factor)
 *
 *	PRECONDITION: src dst point to *disinct* memory buffers...
 *	              of block_count *adjacent* int16_t[64] arrays...
 *
 *RETURN: A bit-mask of block_count bits indicating non-zero blocks (a 1).
 */
	

/*
 * 3D-Now version: simply truncates to zero, however, the tables have a 2% bias
 * upwards which partly compensates.
 */
 
int quant_non_intra_hv_3dnow(	
	pict_data_s *picture,
	int16_t *src, int16_t *dst,
	int mquant,
	int *nonsat_mquant)
{
	int saturated;
	int satlim = dctsatlim;
	float *i_quant_matf; 
	int   coeff_count = 64*block_count;
	uint32_t nzflag, flags;
	int16_t *psrc, *pdst;
	float *piqf;
	int i;
	uint32_t tmp;

	/* Initialise zero block flags */
	/* Load 1 into mm6 */
	__asm__ ( "movl %0, %%eax\n" 
			  "movd %%eax, %%mm6\n"
			  : :"g" (1) : "eax" );
	/* Load satlim into mm1 */
	movd_m2r( satlim, mm1 );
	punpcklwd_r2r( mm1, mm1 );
	punpckldq_r2r( mm1, mm1 );
restart:
	i_quant_matf = i_inter_q_tblf[mquant];
	flags = 0;
	piqf = i_quant_matf;
	saturated = 0;
	nzflag = 0;
	psrc = src;
	pdst = dst;
	for (i=0; i < coeff_count ; i+=4)
	{

		/* TODO: For maximum efficiency this should be unrolled to allow
		   f.p. and int MMX to be interleaved... 
		*/

		/* Load 4 words, unpack into mm2 and mm3 (with sign extension!)
		 */

		movq_m2r( *(mmx_t *)&psrc[0], mm2 );
		movq_r2r( mm2, mm7 );
		psraw_i2r( 16, mm7 );	/* Replicate sign bits mm2 in mm7 */
		movq_r2r( mm2, mm3 );
		punpcklwd_r2r( mm7, mm2 ); /* Unpack with sign extensions */
		punpckhwd_r2r( mm7, mm3);

		/* Multiply by sixteen... */
		pslld_i2r( 4, mm2 );
		pslld_i2r( 4, mm3 );
		
		/*
		   Load the inverse quantisation factors from the 
		   table in to mm4 and mm5
		   Interleaved with converting mm2 and mm3 to float's
		   to (hopefully) maximise parallelism.
		 */
		movq_m2r( *(mmx_t*)&piqf[0], mm4);
		pi2fd_r2r( mm2, mm2);
		movq_m2r( *(mmx_t*)&piqf[2], mm5);
		pi2fd_r2r( mm3, mm3);

		/* "Divide" by multiplying by inverse quantisation
		 and convert back to integers*/
		pfmul_r2r( mm4, mm2 );
		pf2id_r2r( mm2, mm2);
		pfmul_r2r( mm5, mm3);
		pf2id_r2r( mm3, mm3);


		/* Convert the two pairs of double words into four words */
		packssdw_r2r(  mm3, mm2);


		/* Accumulate saturation... */
		movq_r2r( mm2, mm4 );

		pxor_r2r( mm5, mm5 );	// mm5 = -mm2
		pcmpgtw_r2r( mm1, mm4 ); // mm4 = (mm2 > satlim) 
		psubw_r2r( mm2, mm5 );
		pcmpgtw_r2r( mm1, mm5 ); // mm5 = -mm2 > satlim
		por_r2r( mm5, mm4 );  // mm4 = abs(mm2) > satlim
		movq_r2r( mm4, mm5 );
		psrlq_i2r( 32, mm5);
		por_r2r( mm5, mm4 );

		movd_m2r( saturated, mm5 ); // saturated |= mm4
		por_r2r( mm4, mm5 );
		movd_r2m( mm5, saturated );

		/* Store and accumulate zero-ness */
		movq_r2r( mm2, mm3 );
		movq_r2m( mm2, *(mmx_t*)pdst );
		psrlq_i2r( 32, mm3 );
		por_r2r( mm3, mm2 );
		movd_r2m( mm2, tmp );
		flags |= tmp;

		piqf += 4;
		pdst += 4;
		psrc += 4;

		if( (i & 63) == (63/4)*4 )
		{

			if( saturated )
			{
				int new_mquant = next_larger_quant_hv( picture, mquant );
				if( new_mquant != mquant )
				{
					mquant = new_mquant;
					goto restart;
				}
				else
				{
					return quant_non_intra_hv(picture, src, dst, mquant, 
										   nonsat_mquant);
				}
			}

			nzflag = (nzflag<<1) | !!flags;
			flags = 0;
			piqf = i_quant_matf;
		}
			
	}
	femms();

	//nzflag = (nzflag<<1) | (!!flags);
	return nzflag;
}







/* Disabled in mjpegtools */

#if 0
/*
 * SSE version: simply truncates to zero, however, the tables have a 2% bias
 * upwards which partly compensates.
 */
static int trunc_mxcsr = 0x7f80;
 
int quant_non_intra_hv_sse(	
	pict_data_s *picture,
	int16_t *src, int16_t *dst,
	int mquant,
	int *nonsat_mquant)
{
	int saturated;
	int satlim = dctsatlim;
	float *i_quant_matf; 
	int   coeff_count = 64*block_count;
	uint32_t nzflag, flags;
	int16_t *psrc, *pdst;
	float *piqf;
	int i;
	uint32_t tmp;

	/* Initialise zero block flags */
	/* Load 1 into mm6 */
	__asm__ ( "movl %0, %%eax\n" 
			  "movd %%eax, %%mm6\n"
			  : :"g" (1) : "eax" );
	/* Set up SSE rounding mode */
	__asm__ ( "ldmxcsr (%0)\n" : : "X" (trunc_mxcsr) );

	/* Load satlim into mm1 */
	movd_m2r( satlim, mm1 );
	punpcklwd_r2r( mm1, mm1 );
	punpckldq_r2r( mm1, mm1 );
restart:
	i_quant_matf = i_inter_q_tblf[mquant];
	flags = 0;
	piqf = i_quant_matf;
	saturated = 0;
	nzflag = 0;
	psrc = src;
	pdst = dst;
	for (i=0; i < coeff_count ; i+=4)
	{

		/* Load 4 words, unpack into mm2 and mm3 (with sign extension!)
		 */

		movq_m2r( *(mmx_t *)&psrc[0], mm2 );
		movq_r2r( mm2, mm7 );
		psraw_i2r( 16, mm7 );	/* Replicate sign bits mm2 in mm7 */
		movq_r2r( mm2, mm3 );
		punpcklwd_r2r( mm7, mm2 ); /* Unpack with sign extensions */
		punpckhwd_r2r( mm7, mm3);

		/* Multiply by sixteen... */
		pslld_i2r( 4, mm2 );
		pslld_i2r( 4, mm3 );
		
		/*
		  Convert mm2 and mm3 to float's  in xmm2 and xmm3
		 */
		cvtpi2ps_r2r( mm2, xmm2 );
		cvtpi2ps_r2r( mm3, xmm3 );
		shufps_r2ri(  xmm3, xmm2, 0*1 + 1*4 + 0 * 16 + 1 * 64 );

		/* "Divide" by multiplying by inverse quantisation
		 and convert back to integers*/
		mulps_m2r( *(mmx_t*)&piqf[0], xmm2 );
		cvtps2pi_r2r( xmm2, mm2 );
		shufps_r2ri( xmm2, xmm2, 2*1 + 3*4 + 0 * 16 + 1 * 64 );
		cvtps2pi_r2r( xmm2, mm3 );

		/* Convert the two pairs of double words into four words */
		packssdw_r2r(  mm3, mm2);


		/* Accumulate saturation... */
		movq_r2r( mm2, mm4 );

		pxor_r2r( mm5, mm5 );	// mm5 = -mm2
		pcmpgtw_r2r( mm1, mm4 ); // mm4 = (mm2 > satlim) 
		psubw_r2r( mm2, mm5 );
		pcmpgtw_r2r( mm1, mm5 ); // mm5 = -mm2 > satlim
		por_r2r( mm5, mm4 );  // mm4 = abs(mm2) > satlim
		movq_r2r( mm4, mm5 );
		psrlq_i2r( 32, mm5);
		por_r2r( mm5, mm4 );

		movd_m2r( saturated, mm5 ); // saturated |= mm4
		por_r2r( mm4, mm5 );
		movd_r2m( mm5, saturated );

		/* Store and accumulate zero-ness */
		movq_r2r( mm2, mm3 );
		movq_r2m( mm2, *(mmx_t*)pdst );
		psrlq_i2r( 32, mm3 );
		por_r2r( mm3, mm2 );
		movd_r2m( mm2, tmp );
		flags |= tmp;
				
		piqf += 4;
		pdst += 4;
		psrc += 4;

		if( (i & 63) == (63/4)*4 )
		{

			if( saturated )
			{
				int new_mquant = next_larger_quant_hv( picture, mquant );
				if( new_mquant != mquant )
				{
					mquant = new_mquant;
					goto restart;
				}
				else
				{
					return quant_non_intra_hv(picture, src, dst, mquant, 
										   nonsat_mquant);
				}
			}

			nzflag = (nzflag<<1) | !!flags;
			flags = 0;
			piqf = i_quant_matf;
		}
			
	}
	emms();

	//nzflag = (nzflag<<1) | (!!flags);
	return nzflag;
}

#endif

/*
 * The ordinary MMX version.  Due to the limited dynamic range afforded by working
 * with 16-bit int's it (a) has to jump through some gory fudge-factor hoops
 * (b) give up in tough cases and fall back on the reference code. Fortunately, the
 * latter happens *very* rarely.
 *
 * TODO Replace the inefficient block-by-block call to the assembler by a sweep
 * through the whole lot...
 */
																							     											     
int quant_non_intra_hv_mmx(
	pict_data_s *picture,
	int16_t *src, int16_t *dst,
	int mquant,
	int *nonsat_mquant)
{

	int nzflag;
	int clipvalue  = dctsatlim;
	int flags = 0;
	int saturated = 0;
	uint16_t *quant_mat = inter_q;
	int comp;
	uint16_t *i_quant_mat = i_inter_q;
	int imquant;
	int16_t *psrc, *pdst;

	/* MMX routine does not work right for MQ=2 ... (no unsigned mult) */
	if( mquant == 2 )
	{
		return quant_non_intra_hv(picture, src, dst, mquant, nonsat_mquant);
	}
	/* If available use the fast MMX quantiser.  It returns
	   flags to signal if coefficients are outside its limited range or
	   saturation would occur with the specified quantisation
	   factor
	   Top 16 bits - non zero quantised coefficient present
	   Bits 8-15   - Saturation occurred
	   Bits 0-7    - Coefficient out of range.
	*/

	nzflag = 0;
	pdst = dst;
	psrc = src;
	comp = 0; 
	do
	{
		imquant = (IQUANT_SCALE/mquant);
		flags = quantize_ni_mmx( pdst, psrc, quant_mat, i_quant_mat, 
										imquant, mquant, clipvalue );
		nzflag = (nzflag << 1) |( !!(flags & 0xffff0000));
  
		/* If we're saturating simply bump up quantization and start
			from scratch...  if we can't avoid saturation by
			quantising then we're hosed and we fall back to
			saturation using the old C code.  */
  
		if( (flags & 0xff00) != 0 )
		{
			int new_mquant = next_larger_quant_hv( picture, mquant );
			if( new_mquant != mquant )
			{
				mquant = new_mquant;
			}
			else
			{
				saturated = 1;
				break;
			}

			comp = 0; 
			nzflag = 0;
			pdst = dst;
			psrc = src;
		}
		else
		{
			++comp;
			pdst += 64;
			psrc +=64;
		}
		/* Fall back to 32-bit(or better - if some hero(ine) made this work on
			non 32-bit int machines ;-)) if out of dynamic range for MMX...
		*/
	}
	while( comp < block_count  && (flags & 0xff) == 0  );


	/* Coefficient out of range or can't avoid saturation:
	fall back to the original 32-bit int version: this is rare */
	if(  (flags & 0xff) != 0 || saturated)
	{
		return quant_non_intra_hv(picture, src, dst, mquant, nonsat_mquant);
	}

	*nonsat_mquant = mquant;
	return nzflag;
}


void iquant1_intra(int16_t *src, int16_t *dst, int dc_prec, int mquant)
{
  int i, val;
  uint16_t *quant_mat = intra_q;

  dst[0] = src[0] << (3-dc_prec);
  for (i=1; i<64; i++)
  {
    val = (int)(src[i]*quant_mat[i]*mquant)/16;

    /* mismatch control */
    if ((val&1)==0 && val!=0)
      val+= (val>0) ? -1 : 1;

    /* saturation */
    dst[i] = (val>2047) ? 2047 : ((val<-2048) ? -2048 : val);
  }
}
