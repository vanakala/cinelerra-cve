#include "sizes.h"

#ifdef RIGHT_SHIFT_IS_UNSIGNED
#define SHIFT_TEMPS	long shift_temp;
#define RIGHT_SHIFT(x, shft) \
	((shift_temp = (x)) < 0 ? \
	 (shift_temp >> (shft)) | ((~((long) 0)) << (32-(shft))) : \
	 (shift_temp >> (shft)))
#else
#define SHIFT_TEMPS
#define RIGHT_SHIFT(x, shft) ((x) >> (shft))
#endif

#define MAXJSAMPLE 255
#define RANGE_MASK  (MAXJSAMPLE * 4 + 3)
#define CONST_SCALE (ONE << CONST_BITS)
#define ONE	((long)1)
#define CONST_BITS 13
#define PASS1_BITS  2
#define DCTSIZE1 8
#define DCTSIZE2 64
#define MULTIPLY(var, const) ((var) * (const))
#define FIX(x)	((long)((x) * CONST_SCALE + 0.5))
#define DESCALE(x,n)  RIGHT_SHIFT((x) + (ONE << ((n) - 1)), n)

int quicktime_rev_dct(int16_t *data, unsigned char *outptr, unsigned char *rnglimit)
{
	long tmp0, tmp1, tmp2, tmp3;
	long tmp10, tmp11, tmp12, tmp13;
	long z1, z2, z3, z4, z5;
	long d0, d1, d2, d3, d4, d5, d6, d7;
	register int16_t *dataptr;
	int rowctr;
	SHIFT_TEMPS

/* Pass 1: process rows. */
/* Note results are scaled up by sqrt(8) compared to a true IDCT; */
/* furthermore, we scale the results by 2**PASS1_BITS. */

	dataptr = data;

	for(rowctr = DCTSIZE1 - 1; rowctr >= 0; rowctr--) 
	{
/* Due to quantization, we will usually find that many of the input
 * coefficients are zero, especially the AC terms.  We can exploit this
 * by short-circuiting the IDCT calculation for any row in which all
 * the AC terms are zero.  In that case each output is equal to the
 * DC coefficient (with scale factor as needed).
 * With typical images and quantization tables, half or more of the
 * row DCT calculations can be simplified this way.
 */

    	register int *idataptr = (int*)dataptr;
    	d0 = dataptr[0];
    	d1 = dataptr[1];
    	if((d1 == 0) && (idataptr[1] | idataptr[2] | idataptr[3]) == 0) 
		{
/* AC terms all zero */
    		if(d0) 
			{
/* Compute a 32 bit value to assign. */
				int16_t dcval = (int16_t) (d0 << PASS1_BITS);
				register int v = (dcval & 0xffff) | ((dcval << 16) & 0xffff0000);

				idataptr[0] = v;
				idataptr[1] = v;
				idataptr[2] = v;
				idataptr[3] = v;
    		}

/* advance pointer to next row */
    		dataptr += DCTSIZE1;	
    		continue;
    	}
    	d2 = dataptr[2];
    	d3 = dataptr[3];
    	d4 = dataptr[4];
    	d5 = dataptr[5];
    	d6 = dataptr[6];
    	d7 = dataptr[7];

    	/* Even part: reverse the even part of the forward DCT. */
    	/* The rotator is sqrt(2)*c(-6). */
    	if(d6)
		{
			if(d4) 
			{
	    		if(d2)
				{
					if(d0)
					{
/* d0 != 0, d2 != 0, d4 != 0, d6 != 0 */
				    	z1 = MULTIPLY(d2 + d6, FIX(0.541196100));
		    			tmp2 = z1 + MULTIPLY(d6, -FIX(1.847759065));
		    			tmp3 = z1 + MULTIPLY(d2, FIX(0.765366865));

		    			tmp0 = (d0 + d4) << CONST_BITS;
		    			tmp1 = (d0 - d4) << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp1 + tmp2;
		    			tmp12 = tmp1 - tmp2;
					} 
					else 
					{
/* d*0 == 0, d2 != 0, d4 != 0, d6 != 0 */
				    	z1 = MULTIPLY(d2 + d6, FIX(0.541196100));
				    	tmp2 = z1 + MULTIPLY(d6, -FIX(1.847759065));
				    	tmp3 = z1 + MULTIPLY(d2, FIX(0.765366865));

		    			tmp0 = d4 << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp2 - tmp0;
		    			tmp12 = -(tmp0 + tmp2);
					}
		    	} 
				else 
				{
					if(d0) 
					{
/* d0 != 0, d2 == 0, d4 != 0, d6 != 0 */
		    			tmp2 = MULTIPLY(d6, -FIX(1.306562965));
		    			tmp3 = MULTIPLY(d6, FIX(0.541196100));

		    			tmp0 = (d0 + d4) << CONST_BITS;
		    			tmp1 = (d0 - d4) << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp1 + tmp2;
		    			tmp12 = tmp1 - tmp2;
					} 
					else 
					{
/* d0 == 0, d2 == 0, d4 != 0, d6 != 0 */
		    			tmp2 = MULTIPLY(d6, -FIX(1.306562965));
		    			tmp3 = MULTIPLY(d6, FIX(0.541196100));

		    			tmp0 = d4 << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp2 - tmp0;
		    			tmp12 = -(tmp0 + tmp2);
					}
	    		}
			} 
			else 
			{
	    		if (d2) 
				{
					if (d0) 
					{
/* d0 != 0, d2 != 0, d4 == 0, d6 != 0 */
		    			z1 = MULTIPLY(d2 + d6, FIX(0.541196100));
		    			tmp2 = z1 + MULTIPLY(d6, -FIX(1.847759065));
		    			tmp3 = z1 + MULTIPLY(d2, FIX(0.765366865));

		    			tmp0 = d0 << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp0 + tmp2;
		    			tmp12 = tmp0 - tmp2;
					} 
					else 
					{
/* d0 == 0, d2 != 0, d4 == 0, d6 != 0 */
		    			z1 = MULTIPLY(d2 + d6, FIX(0.541196100));
		    			tmp2 = z1 + MULTIPLY(d6, -FIX(1.847759065));
		    			tmp3 = z1 + MULTIPLY(d2, FIX(0.765366865));

		    			tmp10 = tmp3;
		    			tmp13 = -tmp3;
		    			tmp11 = tmp2;
		    			tmp12 = -tmp2;
					}
	    		} 
				else 
				{
					if(d0) 
					{
/* d0 != 0, d2 == 0, d4 == 0, d6 != 0 */
		    			tmp2 = MULTIPLY(d6, -FIX(1.306562965));
		    			tmp3 = MULTIPLY(d6, FIX(0.541196100));

		    			tmp0 = d0 << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp0 + tmp2;
		    			tmp12 = tmp0 - tmp2;
					} 
					else 
					{
/* d0 == 0, d2 == 0, d4 == 0, d6 != 0 */
		    			tmp2 = MULTIPLY(d6, -FIX(1.306562965));
		    			tmp3 = MULTIPLY(d6, FIX(0.541196100));

		    			tmp10 = tmp3;
		    			tmp13 = -tmp3;
		    			tmp11 = tmp2;
		    			tmp12 = -tmp2;
					}
	    		}
			}
    	} 
		else 
		{
			if(d4) 
			{
	    		if(d2) 
				{
					if(d0) 
					{
/* d*0 != 0, d2 != 0, d4 != 0, d6 == 0 */
		    			tmp2 = MULTIPLY(d2, FIX(0.541196100));
		    			tmp3 = MULTIPLY(d2, FIX(1.306562965));

		    			tmp0 = (d0 + d4) << CONST_BITS;
		    			tmp1 = (d0 - d4) << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp1 + tmp2;
		    			tmp12 = tmp1 - tmp2;
					} 
					else 
					{
/* d0 == 0, d2 != 0, d4 != 0, d6 == 0 */
		    			tmp2 = MULTIPLY(d2, FIX(0.541196100));
		    			tmp3 = MULTIPLY(d2, FIX(1.306562965));

		    			tmp0 = d4 << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp2 - tmp0;
		    			tmp12 = -(tmp0 + tmp2);
					}
	    		} 
				else 
				{
					if(d0) 
					{
/* d0 != 0, d2 == 0, d4 != 0, d6 == 0 */
		    			tmp10 = tmp13 = (d0 + d4) << CONST_BITS;
		    			tmp11 = tmp12 = (d0 - d4) << CONST_BITS;
					} 
					else 
					{
/* d0 == 0, d2 == 0, d4 != 0, d6 == 0 */
		    			tmp10 = tmp13 = d4 << CONST_BITS;
		    			tmp11 = tmp12 = -tmp10;
					}
		    	}
			} 
			else 
			{
	    		if(d2) 
				{
					if(d0) 
					{
/* d0 != 0, d2 != 0, d4 == 0, d6 == 0 */
						tmp2 = MULTIPLY(d2, FIX(0.541196100));
						tmp3 = MULTIPLY(d2, FIX(1.306562965));

						tmp0 = d0 << CONST_BITS;

						tmp10 = tmp0 + tmp3;
						tmp13 = tmp0 - tmp3;
						tmp11 = tmp0 + tmp2;
						tmp12 = tmp0 - tmp2;
					} 
					else 
					{
/* d0 == 0, d2 != 0, d4 == 0, d6 == 0 */
						tmp2 = MULTIPLY(d2, FIX(0.541196100));
						tmp3 = MULTIPLY(d2, FIX(1.306562965));

						tmp10 = tmp3;
						tmp13 = -tmp3;
						tmp11 = tmp2;
						tmp12 = -tmp2;
					}
		    	} 
				else 
				{
					if(d0) 
					{
/* d0 != 0, d2 == 0, d4 == 0, d6 == 0 */
		    			tmp10 = tmp13 = tmp11 = tmp12 = d0 << CONST_BITS;
					} 
					else 
					{
/* d0 == 0, d2 == 0, d4 == 0, d6 == 0 */
		    			tmp10 = tmp13 = tmp11 = tmp12 = 0;
					}
		    	}
			}
    	}


/* Odd part per figure 8; the matrix is unitary and hence its
 * transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively.
 */

    	if(d7) 
		{
			if(d5) 
			{
	    		if(d3) 
				{
					if(d1) 
					{
/* d1 != 0, d3 != 0, d5 != 0, d7 != 0 */
		    			z1 = d7 + d1;
		    			z2 = d5 + d3;
		    			z3 = d7 + d3;
		    			z4 = d5 + d1;
		    			z5 = MULTIPLY(z3 + z4, FIX(1.175875602));

		    			tmp0 = MULTIPLY(d7, FIX(0.298631336)); 
		    			tmp1 = MULTIPLY(d5, FIX(2.053119869));
		    			tmp2 = MULTIPLY(d3, FIX(3.072711026));
		    			tmp3 = MULTIPLY(d1, FIX(1.501321110));
		    			z1 = MULTIPLY(z1, -FIX(0.899976223));
		    			z2 = MULTIPLY(z2, -FIX(2.562915447));
		    			z3 = MULTIPLY(z3, -FIX(1.961570560));
		    			z4 = MULTIPLY(z4, -FIX(0.390180644));

		    			z3 += z5;
		    			z4 += z5;

		    			tmp0 += z1 + z3;
		    			tmp1 += z2 + z4;
		    			tmp2 += z2 + z3;
		    			tmp3 += z1 + z4;
					}
					else 
					{
/* d1 == 0, d3 != 0, d5 != 0, d7 != 0 */
		    			z1 = d7;
		    			z2 = d5 + d3;
		    			z3 = d7 + d3;
		    			z5 = MULTIPLY(z3 + d5, FIX(1.175875602));

		    			tmp0 = MULTIPLY(d7, FIX(0.298631336)); 
		    			tmp1 = MULTIPLY(d5, FIX(2.053119869));
		    			tmp2 = MULTIPLY(d3, FIX(3.072711026));
		    			z1 = MULTIPLY(d7, -FIX(0.899976223));
		    			z2 = MULTIPLY(z2, -FIX(2.562915447));
		    			z3 = MULTIPLY(z3, -FIX(1.961570560));
		    			z4 = MULTIPLY(d5, -FIX(0.390180644));

		    			z3 += z5;
		    			z4 += z5;

		    			tmp0 += z1 + z3;
		    			tmp1 += z2 + z4;
		    			tmp2 += z2 + z3;
		    			tmp3 = z1 + z4;
					}
	    		} 
				else 
				{
					if(d1) 
					{
/* d1 != 0, d3 == 0, d5 != 0, d7 != 0 */
		    			z1 = d7 + d1;
		    			z2 = d5;
		    			z3 = d7;
		    			z4 = d5 + d1;
		    			z5 = MULTIPLY(z3 + z4, FIX(1.175875602));

		    			tmp0 = MULTIPLY(d7, FIX(0.298631336)); 
		    			tmp1 = MULTIPLY(d5, FIX(2.053119869));
		    			tmp3 = MULTIPLY(d1, FIX(1.501321110));
		    			z1 = MULTIPLY(z1, -FIX(0.899976223));
		    			z2 = MULTIPLY(d5, -FIX(2.562915447));
		    			z3 = MULTIPLY(d7, -FIX(1.961570560));
		    			z4 = MULTIPLY(z4, -FIX(0.390180644));

		    			z3 += z5;
		    			z4 += z5;

		    			tmp0 += z1 + z3;
		    			tmp1 += z2 + z4;
		    			tmp2 = z2 + z3;
		    			tmp3 += z1 + z4;
					} 
					else 
					{
/* d1 == 0, d3 == 0, d5 != 0, d7 != 0 */
		    			tmp0 = MULTIPLY(d7, -FIX(0.601344887)); 
		    			z1 = MULTIPLY(d7, -FIX(0.899976223));
		    			z3 = MULTIPLY(d7, -FIX(1.961570560));
		    			tmp1 = MULTIPLY(d5, -FIX(0.509795578));
		    			z2 = MULTIPLY(d5, -FIX(2.562915447));
		    			z4 = MULTIPLY(d5, -FIX(0.390180644));
		    			z5 = MULTIPLY(d5 + d7, FIX(1.175875602));

		    			z3 += z5;
		    			z4 += z5;

		    			tmp0 += z3;
		    			tmp1 += z4;
		    			tmp2 = z2 + z3;
		    			tmp3 = z1 + z4;
					}
	    		}
			} 
			else 
			{
	    		if(d3) 
				{
					if(d1) 
					{
/* d1 != 0, d3 != 0, d5 == 0, d7 != 0 */
		    			z1 = d7 + d1;
		    			z3 = d7 + d3;
		    			z5 = MULTIPLY(z3 + d1, FIX(1.175875602));

		    			tmp0 = MULTIPLY(d7, FIX(0.298631336)); 
		    			tmp2 = MULTIPLY(d3, FIX(3.072711026));
		    			tmp3 = MULTIPLY(d1, FIX(1.501321110));
		    			z1 = MULTIPLY(z1, -FIX(0.899976223));
		    			z2 = MULTIPLY(d3, -FIX(2.562915447));
		    			z3 = MULTIPLY(z3, -FIX(1.961570560));
		    			z4 = MULTIPLY(d1, -FIX(0.390180644));

		    			z3 += z5;
		    			z4 += z5;

		    			tmp0 += z1 + z3;
		    			tmp1 = z2 + z4;
		    			tmp2 += z2 + z3;
		    			tmp3 += z1 + z4;
					} 
					else 
					{
/* d1 == 0, d3 != 0, d5 == 0, d7 != 0 */
		    			z3 = d7 + d3;

		    			tmp0 = MULTIPLY(d7, -FIX(0.601344887)); 
		    			z1 = MULTIPLY(d7, -FIX(0.899976223));
		    			tmp2 = MULTIPLY(d3, FIX(0.509795579));
		    			z2 = MULTIPLY(d3, -FIX(2.562915447));
		    			z5 = MULTIPLY(z3, FIX(1.175875602));
		    			z3 = MULTIPLY(z3, -FIX(0.785694958));

		    			tmp0 += z3;
		    			tmp1 = z2 + z5;
		    			tmp2 += z3;
		    			tmp3 = z1 + z5;
					}
	   			} 
				else 
				{
					if(d1) 
					{
/* d1 != 0, d3 == 0, d5 == 0, d7 != 0 */
		    			z1 = d7 + d1;
		    			z5 = MULTIPLY(z1, FIX(1.175875602));

		    			z1 = MULTIPLY(z1, FIX(0.275899379));
		    			z3 = MULTIPLY(d7, -FIX(1.961570560));
		    			tmp0 = MULTIPLY(d7, -FIX(1.662939224)); 
		    			z4 = MULTIPLY(d1, -FIX(0.390180644));
		    			tmp3 = MULTIPLY(d1, FIX(1.111140466));

		    			tmp0 += z1;
		    			tmp1 = z4 + z5;
		    			tmp2 = z3 + z5;
		    			tmp3 += z1;
					} 
					else 
					{
/* d1 == 0, d3 == 0, d5 == 0, d7 != 0 */
		    			tmp0 = MULTIPLY(d7, -FIX(1.387039845));
		    			tmp1 = MULTIPLY(d7, FIX(1.175875602));
		    			tmp2 = MULTIPLY(d7, -FIX(0.785694958));
		    			tmp3 = MULTIPLY(d7, FIX(0.275899379));
					}
		    	}
			}
    	} 
		else 
		{
			if(d5) 
			{
	    		if(d3) 
				{
					if(d1) 
					{
/* d1 != 0, d3 != 0, d5 != 0, d7 == 0 */
		    			z2 = d5 + d3;
		    			z4 = d5 + d1;
		    			z5 = MULTIPLY(d3 + z4, FIX(1.175875602));

		    			tmp1 = MULTIPLY(d5, FIX(2.053119869));
		    			tmp2 = MULTIPLY(d3, FIX(3.072711026));
		    			tmp3 = MULTIPLY(d1, FIX(1.501321110));
		    			z1 = MULTIPLY(d1, -FIX(0.899976223));
		    			z2 = MULTIPLY(z2, -FIX(2.562915447));
		    			z3 = MULTIPLY(d3, -FIX(1.961570560));
		    			z4 = MULTIPLY(z4, -FIX(0.390180644));

		    			z3 += z5;
		    			z4 += z5;

		    			tmp0 = z1 + z3;
		    			tmp1 += z2 + z4;
		    			tmp2 += z2 + z3;
		    			tmp3 += z1 + z4;
					} 
					else 
					{
/* d1 == 0, d3 != 0, d5 != 0, d7 == 0 */
		    			z2 = d5 + d3;

		    			z5 = MULTIPLY(z2, FIX(1.175875602));
		    			tmp1 = MULTIPLY(d5, FIX(1.662939225));
		    			z4 = MULTIPLY(d5, -FIX(0.390180644));
		    			z2 = MULTIPLY(z2, -FIX(1.387039845));
		    			tmp2 = MULTIPLY(d3, FIX(1.111140466));
		    			z3 = MULTIPLY(d3, -FIX(1.961570560));

		    			tmp0 = z3 + z5;
		    			tmp1 += z2;
		    			tmp2 += z2;
		    			tmp3 = z4 + z5;
					}
		    	} 
				else 
				{
					if(d1) 
					{
/* d1 != 0, d3 == 0, d5 != 0, d7 == 0 */
		    			z4 = d5 + d1;

		    			z5 = MULTIPLY(z4, FIX(1.175875602));
		    			z1 = MULTIPLY(d1, -FIX(0.899976223));
		    			tmp3 = MULTIPLY(d1, FIX(0.601344887));
		    			tmp1 = MULTIPLY(d5, -FIX(0.509795578));
		    			z2 = MULTIPLY(d5, -FIX(2.562915447));
		    			z4 = MULTIPLY(z4, FIX(0.785694958));

		    			tmp0 = z1 + z5;
		    			tmp1 += z4;
		    			tmp2 = z2 + z5;
		    			tmp3 += z4;
					} 
					else 
					{
/* d1 == 0, d3 == 0, d5 != 0, d7 == 0 */
		    			tmp0 = MULTIPLY(d5, FIX(1.175875602));
		    			tmp1 = MULTIPLY(d5, FIX(0.275899380));
		    			tmp2 = MULTIPLY(d5, -FIX(1.387039845));
		    			tmp3 = MULTIPLY(d5, FIX(0.785694958));
					}
		    	}
			} 
			else 
			{
	    		if(d3) 
				{
					if(d1) 
					{
/* d1 != 0, d3 != 0, d5 == 0, d7 == 0 */
		    			z5 = d1 + d3;
		    			tmp3 = MULTIPLY(d1, FIX(0.211164243));
		    			tmp2 = MULTIPLY(d3, -FIX(1.451774981));
		    			z1 = MULTIPLY(d1, FIX(1.061594337));
		    			z2 = MULTIPLY(d3, -FIX(2.172734803));
		    			z4 = MULTIPLY(z5, FIX(0.785694958));
		    			z5 = MULTIPLY(z5, FIX(1.175875602));

		    			tmp0 = z1 - z4;
		    			tmp1 = z2 + z4;
		    			tmp2 += z5;
		    			tmp3 += z5;
					} 
					else 
					{
/* d1 == 0, d3 != 0, d5 == 0, d7 == 0 */
		    			tmp0 = MULTIPLY(d3, -FIX(0.785694958));
		    			tmp1 = MULTIPLY(d3, -FIX(1.387039845));
		    			tmp2 = MULTIPLY(d3, -FIX(0.275899379));
		    			tmp3 = MULTIPLY(d3, FIX(1.175875602));
					}
		    	} 
				else 
				{
					if(d1) 
					{
/* d1 != 0, d3 == 0, d5 == 0, d7 == 0 */
		    			tmp0 = MULTIPLY(d1, FIX(0.275899379));
		    			tmp1 = MULTIPLY(d1, FIX(0.785694958));
		    			tmp2 = MULTIPLY(d1, FIX(1.175875602));
		    			tmp3 = MULTIPLY(d1, FIX(1.387039845));
					} 
					else 
					{
/* d1 == 0, d3 == 0, d5 == 0, d7 == 0 */
		    			tmp0 = tmp1 = tmp2 = tmp3 = 0;
					}
		    	}
			}
    	}

/* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */

    	dataptr[0] = (int16_t) DESCALE(tmp10 + tmp3, CONST_BITS - PASS1_BITS);
    	dataptr[7] = (int16_t) DESCALE(tmp10 - tmp3, CONST_BITS - PASS1_BITS);
    	dataptr[1] = (int16_t) DESCALE(tmp11 + tmp2, CONST_BITS - PASS1_BITS);
    	dataptr[6] = (int16_t) DESCALE(tmp11 - tmp2, CONST_BITS - PASS1_BITS);
    	dataptr[2] = (int16_t) DESCALE(tmp12 + tmp1, CONST_BITS - PASS1_BITS);
    	dataptr[5] = (int16_t) DESCALE(tmp12 - tmp1, CONST_BITS - PASS1_BITS);
    	dataptr[3] = (int16_t) DESCALE(tmp13 + tmp0, CONST_BITS - PASS1_BITS);
    	dataptr[4] = (int16_t) DESCALE(tmp13 - tmp0, CONST_BITS - PASS1_BITS);

    	dataptr += DCTSIZE1;		/* advance pointer to next row */
	}

/* Pass 2: process columns. */
/* Note that we must descale the results by a factor of 8 == 2**3, */
/* and also undo the PASS1_BITS scaling. */

	dataptr = data;
	for(rowctr = DCTSIZE1 - 1; rowctr >= 0; rowctr--) 
	{
/* Columns of zeroes can be exploited in the same way as we did with rows.
 * However, the row calculation has created many nonzero AC terms, so the
 * simplification applies less often (typically 5% to 10% of the time).
 * On machines with very fast multiplication, it's possible that the
 * test takes more time than it's worth.  In that case this section
 * may be commented out.
 */

    	d0 = dataptr[DCTSIZE1 * 0];
    	d1 = dataptr[DCTSIZE1 * 1];
    	d2 = dataptr[DCTSIZE1 * 2];
    	d3 = dataptr[DCTSIZE1 * 3];
    	d4 = dataptr[DCTSIZE1 * 4];
    	d5 = dataptr[DCTSIZE1 * 5];
    	d6 = dataptr[DCTSIZE1 * 6];
    	d7 = dataptr[DCTSIZE1 * 7];

/* Even part: reverse the even part of the forward DCT. */
/* The rotator is sqrt(2)*c(-6). */
    	if(d6) 
		{
			if(d4) 
			{
	    		if(d2) 
				{
					if(d0) 
					{
/* d0 != 0, d2 != 0, d4 != 0, d6 != 0 */
		    			z1 = MULTIPLY(d2 + d6, FIX(0.541196100));
		    			tmp2 = z1 + MULTIPLY(d6, -FIX(1.847759065));
		    			tmp3 = z1 + MULTIPLY(d2, FIX(0.765366865));

		    			tmp0 = (d0 + d4) << CONST_BITS;
		    			tmp1 = (d0 - d4) << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp1 + tmp2;
		    			tmp12 = tmp1 - tmp2;
					} else {
/* d0 == 0, d2 != 0, d4 != 0, d6 != 0 */
		    			z1 = MULTIPLY(d2 + d6, FIX(0.541196100));
		    			tmp2 = z1 + MULTIPLY(d6, -FIX(1.847759065));
		    			tmp3 = z1 + MULTIPLY(d2, FIX(0.765366865));

		    			tmp0 = d4 << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp2 - tmp0;
		    			tmp12 = -(tmp0 + tmp2);
					}
	    		} 
				else 
				{
					if(d0) 
					{
/* d0 != 0, d2 == 0, d4 != 0, d6 != 0 */
		    			tmp2 = MULTIPLY(d6, -FIX(1.306562965));
		    			tmp3 = MULTIPLY(d6, FIX(0.541196100));

		    			tmp0 = (d0 + d4) << CONST_BITS;
		    			tmp1 = (d0 - d4) << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp1 + tmp2;
		    			tmp12 = tmp1 - tmp2;
					} 
					else 
					{
/* d0 == 0, d2 == 0, d4 != 0, d6 != 0 */
		    			tmp2 = MULTIPLY(d6, -FIX(1.306562965));
		    			tmp3 = MULTIPLY(d6, FIX(0.541196100));

		    			tmp0 = d4 << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp2 - tmp0;
		    			tmp12 = -(tmp0 + tmp2);
					}
	    		}
			} 
			else 
			{
	    		if(d2) 
				{
					if(d0) 
					{
/* d0 != 0, d2 != 0, d4 == 0, d6 != 0 */
		    			z1 = MULTIPLY(d2 + d6, FIX(0.541196100));
		    			tmp2 = z1 + MULTIPLY(d6, -FIX(1.847759065));
		    			tmp3 = z1 + MULTIPLY(d2, FIX(0.765366865));

		    			tmp0 = d0 << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp0 + tmp2;
		    			tmp12 = tmp0 - tmp2;
					} 
					else 
					{
/* d0 == 0, d2 != 0, d4 == 0, d6 != 0 */
		    			z1 = MULTIPLY(d2 + d6, FIX(0.541196100));
		    			tmp2 = z1 + MULTIPLY(d6, -FIX(1.847759065));
		    			tmp3 = z1 + MULTIPLY(d2, FIX(0.765366865));

		    			tmp10 = tmp3;
		    			tmp13 = -tmp3;
		    			tmp11 = tmp2;
		    			tmp12 = -tmp2;
					}
	    		} 
				else 
				{
					if(d0) 
					{
/* d0 != 0, d2 == 0, d4 == 0, d6 != 0 */
		    			tmp2 = MULTIPLY(d6, -FIX(1.306562965));
		    			tmp3 = MULTIPLY(d6, FIX(0.541196100));

		    			tmp0 = d0 << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp0 + tmp2;
		    			tmp12 = tmp0 - tmp2;
					} 
					else 
					{
/* d0 == 0, d2 == 0, d4 == 0, d6 != 0 */
		    			tmp2 = MULTIPLY(d6, -FIX(1.306562965));
		    			tmp3 = MULTIPLY(d6, FIX(0.541196100));

		    			tmp10 = tmp3;
		    			tmp13 = -tmp3;
		    			tmp11 = tmp2;
		    			tmp12 = -tmp2;
					}
	    		}
			}
    	} 
		else 
		{
			if(d4) 
			{
	    		if(d2) 
				{
					if(d0) 
					{
/* d0 != 0, d2 != 0, d4 != 0, d6 == 0 */
		    			tmp2 = MULTIPLY(d2, FIX(0.541196100));
		    			tmp3 = MULTIPLY(d2, FIX(1.306562965));

		    			tmp0 = (d0 + d4) << CONST_BITS;
		    			tmp1 = (d0 - d4) << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp1 + tmp2;
		    			tmp12 = tmp1 - tmp2;
					} 
					else 
					{
/* d0 == 0, d2 != 0, d4 != 0, d6 == 0 */
		    			tmp2 = MULTIPLY(d2, FIX(0.541196100));
		    			tmp3 = MULTIPLY(d2, FIX(1.306562965));

		    			tmp0 = d4 << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp2 - tmp0;
		    			tmp12 = -(tmp0 + tmp2);
					}
	    		} 
				else 
				{
					if(d0) 
					{
/* d0 != 0, d2 == 0, d4 != 0, d6 == 0 */
		    			tmp10 = tmp13 = (d0 + d4) << CONST_BITS;
		    			tmp11 = tmp12 = (d0 - d4) << CONST_BITS;
					}
					else 
					{
/* d0 == 0, d2 == 0, d4 != 0, d6 == 0 */
		    			tmp10 = tmp13 = d4 << CONST_BITS;
		    			tmp11 = tmp12 = -tmp10;
					}
	    		}
			} 
			else 
			{
	    		if(d2) 
				{
					if(d0) 
					{
/* d0 != 0, d2 != 0, d4 == 0, d6 == 0 */
		    			tmp2 = MULTIPLY(d2, FIX(0.541196100));
		    			tmp3 = MULTIPLY(d2, FIX(1.306562965));

		    			tmp0 = d0 << CONST_BITS;

		    			tmp10 = tmp0 + tmp3;
		    			tmp13 = tmp0 - tmp3;
		    			tmp11 = tmp0 + tmp2;
		    			tmp12 = tmp0 - tmp2;
					} 
					else 
					{
/* d0 == 0, d2 != 0, d4 == 0, d6 == 0 */
		    			tmp2 = MULTIPLY(d2, FIX(0.541196100));
		    			tmp3 = MULTIPLY(d2, FIX(1.306562965));

		    			tmp10 = tmp3;
		    			tmp13 = -tmp3;
		    			tmp11 = tmp2;
		    			tmp12 = -tmp2;
					}
	    		} 
				else 
				{
					if(d0) 
					{
/* d0 != 0, d2 == 0, d4 == 0, d6 == 0 */
		    			tmp10 = tmp13 = tmp11 = tmp12 = d0 << CONST_BITS;
					} 
					else 
					{
/* d0 == 0, d2 == 0, d4 == 0, d6 == 0 */
		    			tmp10 = tmp13 = tmp11 = tmp12 = 0;
					}
	    		}
			}
    	}

/* Odd part per figure 8; the matrix is unitary and hence its
 * transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively.
 */
    	if(d7) 
		{
			if(d5) 
			{
	    		if(d3) 
				{
					if(d1) 
					{
/* d1 != 0, d3 != 0, d5 != 0, d7 != 0 */
		    			z1 = d7 + d1;
		    			z2 = d5 + d3;
		    			z3 = d7 + d3;
		    			z4 = d5 + d1;
		    			z5 = MULTIPLY(z3 + z4, FIX(1.175875602));

		    			tmp0 = MULTIPLY(d7, FIX(0.298631336)); 
		    			tmp1 = MULTIPLY(d5, FIX(2.053119869));
		    			tmp2 = MULTIPLY(d3, FIX(3.072711026));
		    			tmp3 = MULTIPLY(d1, FIX(1.501321110));
		    			z1 = MULTIPLY(z1, -FIX(0.899976223));
		    			z2 = MULTIPLY(z2, -FIX(2.562915447));
		    			z3 = MULTIPLY(z3, -FIX(1.961570560));
		    			z4 = MULTIPLY(z4, -FIX(0.390180644));

		    			z3 += z5;
		    			z4 += z5;

		    			tmp0 += z1 + z3;
		    			tmp1 += z2 + z4;
		    			tmp2 += z2 + z3;
		    			tmp3 += z1 + z4;
					} 
					else 
					{
/* d1 == 0, d3 != 0, d5 != 0, d7 != 0 */
		    			z1 = d7;
		    			z2 = d5 + d3;
		    			z3 = d7 + d3;
		    			z5 = MULTIPLY(z3 + d5, FIX(1.175875602));

		    			tmp0 = MULTIPLY(d7, FIX(0.298631336)); 
		    			tmp1 = MULTIPLY(d5, FIX(2.053119869));
		    			tmp2 = MULTIPLY(d3, FIX(3.072711026));
		    			z1 = MULTIPLY(d7, -FIX(0.899976223));
		    			z2 = MULTIPLY(z2, -FIX(2.562915447));
		    			z3 = MULTIPLY(z3, -FIX(1.961570560));
		    			z4 = MULTIPLY(d5, -FIX(0.390180644));

		    			z3 += z5;
		    			z4 += z5;

		    			tmp0 += z1 + z3;
		    			tmp1 += z2 + z4;
		    			tmp2 += z2 + z3;
		    			tmp3 = z1 + z4;
					}
	    		} 
				else 
				{
					if(d1) 
					{
/* d1 != 0, d3 == 0, d5 != 0, d7 != 0 */
		    			z1 = d7 + d1;
		    			z2 = d5;
		    			z3 = d7;
		    			z4 = d5 + d1;
		    			z5 = MULTIPLY(z3 + z4, FIX(1.175875602));

		    			tmp0 = MULTIPLY(d7, FIX(0.298631336)); 
		    			tmp1 = MULTIPLY(d5, FIX(2.053119869));
		    			tmp3 = MULTIPLY(d1, FIX(1.501321110));
		    			z1 = MULTIPLY(z1, -FIX(0.899976223));
		    			z2 = MULTIPLY(d5, -FIX(2.562915447));
		    			z3 = MULTIPLY(d7, -FIX(1.961570560));
		    			z4 = MULTIPLY(z4, -FIX(0.390180644));

		    			z3 += z5;
		    			z4 += z5;

		    			tmp0 += z1 + z3;
		    			tmp1 += z2 + z4;
		    			tmp2 = z2 + z3;
		    			tmp3 += z1 + z4;
					} 
					else 
					{
/* d1 == 0, d3 == 0, d5 != 0, d7 != 0 */
		    			tmp0 = MULTIPLY(d7, -FIX(0.601344887)); 
		    			z1 = MULTIPLY(d7, -FIX(0.899976223));
		    			z3 = MULTIPLY(d7, -FIX(1.961570560));
		    			tmp1 = MULTIPLY(d5, -FIX(0.509795578));
		    			z2 = MULTIPLY(d5, -FIX(2.562915447));
		    			z4 = MULTIPLY(d5, -FIX(0.390180644));
		    			z5 = MULTIPLY(d5 + d7, FIX(1.175875602));

		    			z3 += z5;
		    			z4 += z5;

		    			tmp0 += z3;
		    			tmp1 += z4;
		    			tmp2 = z2 + z3;
		    			tmp3 = z1 + z4;
					}
	    		}
			} 
			else 
			{
	    		if(d3) 
				{
					if(d1) 
					{
/* d1 != 0, d3 != 0, d5 == 0, d7 != 0 */
		    			z1 = d7 + d1;
		    			z3 = d7 + d3;
		    			z5 = MULTIPLY(z3 + d1, FIX(1.175875602));

		    			tmp0 = MULTIPLY(d7, FIX(0.298631336)); 
		    			tmp2 = MULTIPLY(d3, FIX(3.072711026));
		    			tmp3 = MULTIPLY(d1, FIX(1.501321110));
		    			z1 = MULTIPLY(z1, -FIX(0.899976223));
		    			z2 = MULTIPLY(d3, -FIX(2.562915447));
		    			z3 = MULTIPLY(z3, -FIX(1.961570560));
		    			z4 = MULTIPLY(d1, -FIX(0.390180644));

		    			z3 += z5;
		    			z4 += z5;

		    			tmp0 += z1 + z3;
		    			tmp1 = z2 + z4;
		    			tmp2 += z2 + z3;
		    			tmp3 += z1 + z4;
					}
					else 
					{
/* d1 == 0, d3 != 0, d5 == 0, d7 != 0 */
		    			z3 = d7 + d3;

		    			tmp0 = MULTIPLY(d7, -FIX(0.601344887)); 
		    			z1 = MULTIPLY(d7, -FIX(0.899976223));
		    			tmp2 = MULTIPLY(d3, FIX(0.509795579));
		    			z2 = MULTIPLY(d3, -FIX(2.562915447));
		    			z5 = MULTIPLY(z3, FIX(1.175875602));
		    			z3 = MULTIPLY(z3, -FIX(0.785694958));

		    			tmp0 += z3;
		    			tmp1 = z2 + z5;
		    			tmp2 += z3;
		    			tmp3 = z1 + z5;
					}
	    		} 
				else 
				{
					if(d1) 
					{
/* d1 != 0, d3 == 0, d5 == 0, d7 != 0 */
		    			z1 = d7 + d1;
		    			z5 = MULTIPLY(z1, FIX(1.175875602));

		    			z1 = MULTIPLY(z1, FIX(0.275899379));
		    			z3 = MULTIPLY(d7, -FIX(1.961570560));
		    			tmp0 = MULTIPLY(d7, -FIX(1.662939224)); 
		    			z4 = MULTIPLY(d1, -FIX(0.390180644));
		    			tmp3 = MULTIPLY(d1, FIX(1.111140466));

		    			tmp0 += z1;
		    			tmp1 = z4 + z5;
		    			tmp2 = z3 + z5;
		    			tmp3 += z1;
					} 
					else 
					{
/* d1 == 0, d3 == 0, d5 == 0, d7 != 0 */
		    			tmp0 = MULTIPLY(d7, -FIX(1.387039845));
		    			tmp1 = MULTIPLY(d7, FIX(1.175875602));
		    			tmp2 = MULTIPLY(d7, -FIX(0.785694958));
		    			tmp3 = MULTIPLY(d7, FIX(0.275899379));
					}
	    		}
			}
    	} 
		else 
		{
			if(d5)
			{
	    		if(d3) 
				{
					if(d1) 
					{
/* d1 != 0, d3 != 0, d5 != 0, d7 == 0 */
		    			z2 = d5 + d3;
		    			z4 = d5 + d1;
		    			z5 = MULTIPLY(d3 + z4, FIX(1.175875602));

		    			tmp1 = MULTIPLY(d5, FIX(2.053119869));
		    			tmp2 = MULTIPLY(d3, FIX(3.072711026));
		    			tmp3 = MULTIPLY(d1, FIX(1.501321110));
		    			z1 = MULTIPLY(d1, -FIX(0.899976223));
		    			z2 = MULTIPLY(z2, -FIX(2.562915447));
		    			z3 = MULTIPLY(d3, -FIX(1.961570560));
		    			z4 = MULTIPLY(z4, -FIX(0.390180644));

		    			z3 += z5;
		    			z4 += z5;

		    			tmp0 = z1 + z3;
		    			tmp1 += z2 + z4;
		    			tmp2 += z2 + z3;
		    			tmp3 += z1 + z4;
					} 
					else 
					{
/* d1 == 0, d3 != 0, d5 != 0, d7 == 0 */
		    			z2 = d5 + d3;

		    			z5 = MULTIPLY(z2, FIX(1.175875602));
		    			tmp1 = MULTIPLY(d5, FIX(1.662939225));
		    			z4 = MULTIPLY(d5, -FIX(0.390180644));
		    			z2 = MULTIPLY(z2, -FIX(1.387039845));
		    			tmp2 = MULTIPLY(d3, FIX(1.111140466));
		    			z3 = MULTIPLY(d3, -FIX(1.961570560));

		    			tmp0 = z3 + z5;
		    			tmp1 += z2;
		    			tmp2 += z2;
		    			tmp3 = z4 + z5;
					}
	    		} 
				else 
				{
					if(d1) 
					{
/* d1 != 0, d3 == 0, d5 != 0, d7 == 0 */
		    			z4 = d5 + d1;

		    			z5 = MULTIPLY(z4, FIX(1.175875602));
		    			z1 = MULTIPLY(d1, -FIX(0.899976223));
		    			tmp3 = MULTIPLY(d1, FIX(0.601344887));
		    			tmp1 = MULTIPLY(d5, -FIX(0.509795578));
		    			z2 = MULTIPLY(d5, -FIX(2.562915447));
		    			z4 = MULTIPLY(z4, FIX(0.785694958));

		    			tmp0 = z1 + z5;
		    			tmp1 += z4;
		    			tmp2 = z2 + z5;
		    			tmp3 += z4;
					} 
					else 
					{
/* d1 == 0, d3 == 0, d5 != 0, d7 == 0 */
		    			tmp0 = MULTIPLY(d5, FIX(1.175875602));
		    			tmp1 = MULTIPLY(d5, FIX(0.275899380));
		    			tmp2 = MULTIPLY(d5, -FIX(1.387039845));
		    			tmp3 = MULTIPLY(d5, FIX(0.785694958));
					}
				}
			} 
			else 
			{
	    		if(d3) 
				{
					if(d1) 
					{
/* d1 != 0, d3 != 0, d5 == 0, d7 == 0 */
		    			z5 = d1 + d3;
		    			tmp3 = MULTIPLY(d1, FIX(0.211164243));
		    			tmp2 = MULTIPLY(d3, -FIX(1.451774981));
		    			z1 = MULTIPLY(d1, FIX(1.061594337));
		    			z2 = MULTIPLY(d3, -FIX(2.172734803));
		    			z4 = MULTIPLY(z5, FIX(0.785694958));
		    			z5 = MULTIPLY(z5, FIX(1.175875602));

		    			tmp0 = z1 - z4;
		    			tmp1 = z2 + z4;
		    			tmp2 += z5;
		    			tmp3 += z5;
					} 
					else 
					{
/* d1 == 0, d3 != 0, d5 == 0, d7 == 0 */
		    			tmp0 = MULTIPLY(d3, -FIX(0.785694958));
		    			tmp1 = MULTIPLY(d3, -FIX(1.387039845));
		    			tmp2 = MULTIPLY(d3, -FIX(0.275899379));
		    			tmp3 = MULTIPLY(d3, FIX(1.175875602));
					}
	    		} 
				else 
				{
					if(d1) 
					{
/* d1 != 0, d3 == 0, d5 == 0, d7 == 0 */
		    			tmp0 = MULTIPLY(d1, FIX(0.275899379));
		    			tmp1 = MULTIPLY(d1, FIX(0.785694958));
		    			tmp2 = MULTIPLY(d1, FIX(1.175875602));
		    			tmp3 = MULTIPLY(d1, FIX(1.387039845));
					} 
					else 
					{
/* d1 == 0, d3 == 0, d5 == 0, d7 == 0 */
		    			tmp0 = tmp1 = tmp2 = tmp3 = 0;
					}
	    		}
			}
    	}

/* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */


		outptr[DCTSIZE1 * 0] = rnglimit[(int)DESCALE(tmp10 + tmp3, CONST_BITS + PASS1_BITS + 3) & RANGE_MASK];
		outptr[DCTSIZE1 * 7] = rnglimit[(int)DESCALE(tmp10 - tmp3, CONST_BITS + PASS1_BITS + 3) & RANGE_MASK];
		outptr[DCTSIZE1 * 1] = rnglimit[(int)DESCALE(tmp11 + tmp2, CONST_BITS + PASS1_BITS + 3) & RANGE_MASK];
		outptr[DCTSIZE1 * 6] = rnglimit[(int)DESCALE(tmp11 - tmp2, CONST_BITS + PASS1_BITS + 3) & RANGE_MASK];
		outptr[DCTSIZE1 * 2] = rnglimit[(int)DESCALE(tmp12 + tmp1, CONST_BITS + PASS1_BITS + 3) & RANGE_MASK];
		outptr[DCTSIZE1 * 5] = rnglimit[(int)DESCALE(tmp12 - tmp1, CONST_BITS + PASS1_BITS + 3) & RANGE_MASK];
		outptr[DCTSIZE1 * 3] = rnglimit[(int)DESCALE(tmp13 + tmp0, CONST_BITS + PASS1_BITS + 3) & RANGE_MASK];
		outptr[DCTSIZE1 * 4] = rnglimit[(int)DESCALE(tmp13 - tmp0, CONST_BITS + PASS1_BITS + 3) & RANGE_MASK];

		dataptr++;			/* advance pointer to next column */
		outptr++;
	}
}
