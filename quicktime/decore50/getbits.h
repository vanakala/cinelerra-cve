/**************************************************************************
 *                                                                        *
 * This code has been developed by Eugene Kuznetsov. This software is an  *
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
 * Eugene Kuznetsov
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/

#ifndef _DECORE_GETBITS_H
#define _DECORE_GETBITS_H

void initbits (unsigned char * stream, int length);
void fillbfr (void);

/***/

//#if defined(LINUX)
#if 0

// 486+ specific instruction
// anybody want to use decore on 386?
#define _SWAP(a,b) b=*(int*)a; \
	__asm__ ( "bswapl %0\n" : "=g" (b) : "0" (b) )



#elif defined(WIN32)



#define _SWAP(a,b) \
	b=*(int*)a; __asm mov eax,b __asm bswap eax __asm mov b, eax


#else


#define _SWAP(a,b) (b=((a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3]))


#endif

#ifdef WIN32
#include <io.h>
#endif

/***/







#if 1


static void refill(int bits)
{
	while(64 - ld->pos < bits)
	{
		ld->buf <<= 8;
		ld->buf |= *ld->head++;
		ld->pos -= 8;
	}
}






static __inline unsigned int showbits (int bits)
{
	uint32_t result;
	refill(bits);



	result = (uint64_t)(0xffffffffffffffffLL >> (64 - bits)) &
		(uint64_t)(ld->buf >> (64 - bits - ld->pos));

//printf("showbits %d %x\n", bits, result);
	return result;
}


static __inline void flushbits (int bits)
{
	refill(bits);
	ld->pos += bits;
//printf("flushbits %d\n", bits);
}


static __inline unsigned int getbits (int bits)
{
	uint32_t result;
	refill(bits);
	result = (uint64_t)(0xffffffffffffffffLL >> (64 - bits)) &
		(uint64_t)(ld->buf >> (64 - bits - ld->pos));
	ld->pos += bits;
//printf("getbits %d %x\n", bits, result);
	return result;
}


static __inline unsigned char getbits1 ()
{
	uint32_t result;
	return getbits(1);


	refill(1);

	result = 0x1 & (ld->buf >> (63 - ld->pos));
	ld->pos++;

	return result;
}


// Purpose: look nbit forward for an alignement
static int __inline bytealigned(int nbit) 
{
	return (((ld->pos + nbit) % 8) == 0);
}

static int __inline nextbits_bytealigned(int nbit)
{
	int code;
	int skipcnt = 0;

	if (bytealigned(skipcnt))
	{
		// stuffing bits
		if (showbits(8) == 127) {
			skipcnt += 8;
		}
	}
	else
	{
		// bytealign
		while (! bytealigned(skipcnt)) {
			skipcnt += 1;
		}
	}

	code = showbits(nbit + skipcnt);
	return ((code << skipcnt) >> skipcnt);
}














#else









static __inline unsigned int showbits (int n)
{
	int rbit = 32 - ld->bitcnt;
	unsigned int b;
	_SWAP(ld->rdptr, b);
//	return ((b & msk[rbit]) >> (rbit-n));
// Is it platform independent???
//printf("showbits %d %x\n", n, (b & (0xFFFFFFFFU >> (ld->bitcnt))) >> (rbit-n));

	return (b & (0xFFFFFFFFU >> (ld->bitcnt))) >> (rbit-n);
}

static __inline unsigned int showbits_r (int n)
{
	int rbit = 32 - ld->bitcnt;
	unsigned int b;
	_SWAP(ld->rdptr, b);
//	return ((b & msk[rbit]) >> (rbit-n));
// Is it platform independent???
//printf("showbits %d %x\n", n, (b & (0xFFFFFFFFU >> (ld->bitcnt))) >> (rbit-n));

	return (b & (0xFFFFFFFFU >> (ld->bitcnt))) >> (rbit-n);
}

static __inline void flushbits (int n)
{
	ld->bitcnt += n;
	if (ld->bitcnt >= 8) {
		ld->rdptr += ld->bitcnt / 8;
		ld->bitcnt = ld->bitcnt % 8;
	}
//printf("flushbits %d\n", n);
}

static __inline void flushbits_r (int n)
{
	ld->bitcnt += n;
	if (ld->bitcnt >= 8) {
		ld->rdptr += ld->bitcnt / 8;
		ld->bitcnt = ld->bitcnt % 8;
	}
//printf("flushbits %d\n", n);
}


static __inline unsigned int getbits (int n)
{
  unsigned int l;

  l = showbits_r (n);
  flushbits_r (n);
//printf("getbits %d %x\n", n, l);

  return l;
}












/***/

// Purpose: look nbit forward for an alignement
static int __inline bytealigned(int nbit) 
{
	return (((ld->bitcnt + nbit) % 8) == 0);
}

/***/

static int __inline nextbits_bytealigned(int nbit)
{
	int code;
	int skipcnt = 0;

	if (bytealigned(skipcnt))
	{
		// stuffing bits
		if (showbits(8) == 127) {
			skipcnt += 8;
		}
	}
	else
	{
		// bytealign
		while (! bytealigned(skipcnt)) {
			skipcnt += 1;
		}
	}

	code = showbits(nbit + skipcnt);
	return ((code << skipcnt) >> skipcnt);
}


/* return next bit (could be made faster than getbits(1)) */


static unsigned char getbits1 ()
{
  return getbits (1);
}





#endif





/***/

static int __inline nextbits(int nbit)
{
	return showbits(nbit);
}









#endif /* _DECORE_GETBITS_H */
