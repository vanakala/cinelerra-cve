/* putvlc.c, generation of variable length codes                            */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#include <stdio.h>

#include "config.h"
#include "global.h"
#include "vlc.h"

/* generate variable length code for DC coefficient (7.2.1) */
static void putDC(slice_engine_t *engine, sVLCtable *tab, int val)
{
	int absval, size;

	absval = (val < 0) ? -val : val;

	if(absval > dctsatlim)
	{
/* should never happen */
    	sprintf(errortext,"DC value out of range (%d)\n",val);
    	error(errortext);
	}

/* compute dct_dc_size */
	size = 0;

	while(absval)
	{
    	absval >>= 1;
    	size++;
	}

//printf("putDC %x %d %d\n", tab[size].code, tab[size].len, size);
//slice_testbits(engine);
/* generate VLC for dct_dc_size (Table B-12 or B-13) */
	slice_putbits(engine, tab[size].code, tab[size].len);
//slice_testbits(engine);

/* append fixed length code (dc_dct_differential) */
	if(size != 0)
	{
    	if(val >= 0)
    		absval = val;
    	else
    		absval = val + (1 << size) - 1;
    	slice_putbits(engine, absval, size);
	}
//slice_testbits(engine);
}

/* generate variable length code for luminance DC coefficient */
void putDClum(slice_engine_t *engine, int val)
{
    putDC(engine, DClumtab, val);
}

/* generate variable length code for chrominance DC coefficient */
void putDCchrom(slice_engine_t *engine, int val)
{
    putDC(engine, DCchromtab, val);
}

/* generate variable length code for other DCT coefficients (7.2.2) */
void putAC(slice_engine_t *engine, int run, int signed_level, int vlcformat)
{
	int level, len;
	VLCtable *ptab;

	level = (signed_level < 0) ? -signed_level : signed_level; /* abs(signed_level) */

/* make sure run and level are valid */
	if(run < 0 || run > 63 || level == 0 || level > dctsatlim)
	{
    	sprintf(errortext,"AC value out of range (run=%d, signed_level=%d)\n",
    		run,signed_level);
    	error(errortext);
	}

	len = 0;

	if(run < 2 && level < 41)
	{
/* vlcformat selects either of Table B-14 / B-15 */
    	if(vlcformat)
    		ptab = &dct_code_tab1a[run][level - 1];
    	else
    		ptab = &dct_code_tab1[run][level - 1];

    	len = ptab->len;
	}
	else 
	if(run < 32 && level < 6)
	{
/* vlcformat selects either of Table B-14 / B-15 */
    	if(vlcformat)
    		ptab = &dct_code_tab2a[run - 2][level - 1];
    	else
    		ptab = &dct_code_tab2[run - 2][level - 1];

    	len = ptab->len;
	}

	if(len != 0) /* a VLC code exists */
	{
    	slice_putbits(engine, ptab->code, len);
    	slice_putbits(engine, signed_level < 0, 1); /* sign */
	}
	else
	{
/* no VLC for this (run, level) combination: use escape coding (7.2.2.3) */
    	slice_putbits(engine, 1l, 6); /* Escape */
    	slice_putbits(engine, run, 6); /* 6 bit code for run */
    	if(mpeg1)
    	{
/* ISO/IEC 11172-2 uses a 8 or 16 bit code */
    		if (signed_level > 127)
        		slice_putbits(engine, 0, 8);
    		if (signed_level < -127)
        		slice_putbits(engine, 128, 8);
    		slice_putbits(engine, signed_level, 8);
    	}
    	else
    	{
/* ISO/IEC 13818-2 uses a 12 bit code, Table B-16 */
    		slice_putbits(engine, signed_level, 12);
    	}
	}
}

/* generate variable length code for first coefficient
 * of a non-intra block (7.2.2.2) */
void putACfirst(slice_engine_t *engine, int run, int val)
{
	if(run == 0 && (val == 1 || val == -1)) /* these are treated differently */
    	slice_putbits(engine, 2 | (val < 0), 2); /* generate '1s' (s=sign), (Table B-14, line 2) */
	else
    	putAC(engine, run, val, 0); /* no difference for all others */
}

/* generate variable length code for macroblock_address_increment (6.3.16) */
void putaddrinc(slice_engine_t *engine, int addrinc)
{
	while(addrinc > 33)
	{
    	slice_putbits(engine, 0x08, 11); /* macroblock_escape */
    	addrinc -= 33;
	}

//printf("putaddrinc %d %d\n", addrinctab[addrinc - 1].code, addrinctab[addrinc - 1].len);
//sleep(1);
	slice_putbits(engine, 
		addrinctab[addrinc - 1].code,
		addrinctab[addrinc - 1].len);
}

/* generate variable length code for macroblock_type (6.3.16.1) */
void putmbtype(slice_engine_t *engine, int pict_type, int mb_type)
{
  	slice_putbits(engine, 
		mbtypetab[pict_type - 1][mb_type].code,
    	mbtypetab[pict_type - 1][mb_type].len);
}

/* generate variable length code for motion_code (6.3.16.3) */
void putmotioncode(slice_engine_t *engine, int motion_code)
{
	int abscode;

	abscode = (motion_code >= 0) ? motion_code : -motion_code; /* abs(motion_code) */
	slice_putbits(engine, 
		motionvectab[abscode].code, 
		motionvectab[abscode].len);
	if(motion_code!=0)
    	slice_putbits(engine, 
			motion_code < 0,
			1); /* sign, 0=positive, 1=negative */
}

/* generate variable length code for dmvector[t] (6.3.16.3), Table B-11 */
void putdmv(slice_engine_t *engine, int dmv)
{
  if(dmv == 0)
	  slice_putbits(engine, 0, 1);
  else if (dmv>0)
      slice_putbits(engine, 2, 2);
  else
      slice_putbits(engine, 3, 2);
}

/* generate variable length code for coded_block_pattern (6.3.16.4)
 *
 * 4:2:2, 4:4:4 not implemented
 */
void putcbp(slice_engine_t *engine, int cbp)
{
    slice_putbits(engine, cbptable[cbp].code, cbptable[cbp].len);
}
