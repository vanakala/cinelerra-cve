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
// mp4_vld.c //

#include <stdio.h>

#include "mp4_vars.h"
#include "getbits.h"
#include "mp4_vld.h"

/**
 * 
**/

/***/

event_t vld_intra_dct() 
{
	event_t event;
	tab_type *tab = (tab_type *) NULL;
	int lmax, rmax;

//printf("vld_intra_dct 1 %02x\n", showbits(12));
	tab = vldTableB16(showbits(12));
	if (!tab) { /* bad code */
		event.run   = 
		event.level = 
		event.last  = -1;
		return event;
	} 

	if (tab->val != ESCAPE) {
		event.run   = (tab->val >>  6) & 63;
		event.level =  tab->val        & 63;
		event.last  = (tab->val >> 12) &  1;
		event.level = getbits(1) ? -event.level : event.level;
	} else {
		/* this value is escaped - see para 7.4.1.3 */
		/* assuming short_video_header == 0 */
		switch (showbits(2)) {
			case 0x0 :  /* Type 1 */
			case 0x1 :  /* Type 1 */
				flushbits(1);
				tab = vldTableB16(showbits(12));  /* use table B-16 */
				if (!tab) { /* bad code */
					event.run   = 
					event.level = 
					event.last  = -1;
					return event;
				}
				event.run   = (tab->val >>  6) & 63;
				event.level =  tab->val        & 63;
				event.last  = (tab->val >> 12) &  1;
				lmax = vldTableB19(event.last, event.run);  /* use table B-19 */
				event.level += lmax;
				event.level =  getbits(1) ? -event.level : event.level;
				break;
			case 0x2 :  /* Type 2 */
				flushbits(2);
				tab = vldTableB16(showbits(12));  /* use table B-16 */
				if (!tab) { /* bad code */
					event.run   = 
					event.level = 
					event.last  = -1;
					break;
				}
				event.run   = (tab->val >>  6) & 63;
				event.level =  tab->val        & 63;
				event.last  = (tab->val >> 12) &  1;
				rmax = vldTableB21(event.last, event.level);  /* use table B-21 */
				event.run = event.run + rmax + 1;
				event.level = getbits(1) ? -event.level : event.level;
				break;
			case 0x3 :  /* Type 3  - fixed length codes */
				flushbits(2);
				event.last  = getbits(1);
				event.run   = getbits(6);  /* table B-18 */ 
				getbits(1); /* marker bit */
				event.level = getbits(12); /* table B-18 */
				/* sign extend level... */
				event.level = (event.level & 0x800) ? (event.level | (-1 ^ 0xfff)) : event.level;
				getbits(1); /* marker bit */
				break;
		}
	}

	return event;
}

/***/

event_t vld_inter_dct() 
{
	event_t event;
	tab_type *tab = (tab_type *) NULL;
	int lmax, rmax;

	tab = vldTableB17(showbits(12));
	if (!tab) { /* bad code */
		event.run   = 
		event.level = 
		event.last  = -1;
		return event;
	} 
	if (tab->val != ESCAPE) {
		event.run   = (tab->val >>  4) & 255;
		event.level =  tab->val        & 15;
		event.last  = (tab->val >> 12) &  1;
		event.level = getbits(1) ? -event.level : event.level;
	} else {
		/* this value is escaped - see para 7.4.1.3 */
		/* assuming short_video_header == 0 */
		int mode = showbits(2);
		switch (mode) {
			case 0x0 :  /* Type 1 */
			case 0x1 :  /* Type 1 */
				flushbits(1);
				tab = vldTableB17(showbits(12));  /* use table B-17 */
				if (!tab) { /* bad code */
					event.run   = 
					event.level = 
					event.last  = -1;
					return event;
				}
				event.run   = (tab->val >>  4) & 255;
				event.level =  tab->val        & 15;
				event.last  = (tab->val >> 12) &  1;
				lmax = vldTableB20(event.last, event.run);  /* use table B-20 */
				event.level += lmax;
				event.level = getbits(1) ? -event.level : event.level;
				break;
			case 0x2 :  /* Type 2 */
				flushbits(2);
				tab = vldTableB17(showbits(12));  /* use table B-16 */
				if (!tab) { /* bad code */
					event.run   = 
					event.level = 
					event.last  = -1;
					break;
				}
				event.run   = (tab->val >>  4) & 255;
				event.level =  tab->val        & 15;
				event.last  = (tab->val >> 12) &  1;
				rmax = vldTableB22(event.last, event.level);  /* use table B-22 */
				event.run = event.run + rmax + 1;
				event.level = getbits(1) ? -event.level : event.level;
				break;
			case 0x3 :  /* Type 3  - fixed length codes */
				flushbits(2);
				event.last  = getbits(1);
				event.run   = getbits(6);  /* table B-18 */ 
				getbits(1); /* marker bit */
				event.level = getbits(12); /* table B-18 */
				/* sign extend level... */
				event.level = (event.level & 0x800) ? (event.level | (-1 ^ 0xfff)) : event.level;
				getbits(1); /* marker bit */
				break;
		}
	}

	return event;
}

/***/

event_t vld_event(int intraFlag) 
{
	if (intraFlag) {
		return vld_intra_dct();
	} else {
		return vld_inter_dct();
	}
}

/***/

/* Table B-19 -- ESCL(a), LMAX values of intra macroblocks */
int vldTableB19(int last, int run) {
	if (!last){ /* LAST == 0 */
		if        (run ==  0) {
			return 27;
		} else if (run ==  1) {
			return 10;
		} else if (run ==  2) {
			return  5;
		} else if (run ==  3) {
			return  4;
		} else if (run <=  7) {
			return  3;
		} else if (run <=  9) {
			return  2;
		} else if (run <= 14) {
			return  1;
		} else { /* illegal? */
			return  0; 
		}
	} else {    /* LAST == 1 */
		if        (run ==  0) {
			return  8;
		} else if (run ==  1) {
			return  3;
		} else if (run <=  6) {
			return  2;
		} else if (run <= 20) {
			return  1;
		} else { /* illegal? */
			return  0; 
		}		
	}
}

/***/

/* Table B-20 -- ESCL(b), LMAX values of inter macroblocks */
int vldTableB20(int last, int run) {
	if (!last){ /* LAST == 0 */
		if        (run ==  0) {
			return 12;
		} else if (run ==  1) {
			return  6;
		} else if (run ==  2) {
			return  4;
		} else if (run <=  6) {
			return  3;
		} else if (run <= 10) {
			return  2;
		} else if (run <= 26) {
			return  1;
		} else { /* illegal? */
			return  0; 
		}
	} else {    /* LAST == 1 */
		if        (run ==  0) {
			return  3;
		} else if (run ==  1) {
			return  2;
		} else if (run <= 40) {
			return  1;
		} else { /* illegal? */
			return  0; 
		}		
	}
}

/***/

/* Table B-21 -- ESCR(a), RMAX values of intra macroblocks */
int vldTableB21(int last, int level) {
	if (!last){ /* LAST == 0 */
		if        (level ==  1) {
			return 14;
		} else if (level ==  2) {
			return  9;
		} else if (level ==  3) {
			return  7;
		} else if (level ==  4) {
			return  3;
		} else if (level ==  5) {
			return  2;
		} else if (level <= 10) {
			return  1;
		} else if (level <= 27) {
			return  0;
		} else { /* illegal? */
			return  0; 
		}
	} else {    /* LAST == 1 */
		if        (level ==  1) {
			return  20;
		} else if (level ==  2) {
			return  6;
		} else if (level ==  3) {
			return  1;
		} else if (level <=  8) {
			return  0;
		} else { /* illegal? */
			return  0; 
		}		
	}
}

/***/

/* Table B-22 -- ESCR(b), RMAX values of inter macroblocks */
int vldTableB22(int last, int level) {
	if (!last){ /* LAST == 0 */
		if        (level ==  1) {
			return 26;
		} else if (level ==  2) {
			return 10;
		} else if (level ==  3) {
			return  6;
		} else if (level ==  4) {
			return  2;
		} else if (level <=  6) {
			return  1;
		} else if (level <= 12) {
			return  0;
		} else { /* illegal? */
			return  0; 
		}
	} else {    /* LAST == 1 */
		if        (level ==  1) {
			return  40;
		} else if (level ==  2) {
			return  1;
		} else if (level ==  3) {
			return  0;
		} else { /* illegal? */
			return  0; 
		}		
	}
}

/***/

tab_type *vldTableB16(int code) {
	tab_type *tab;

	if (code >= 512) {
		tab = &(mp4_tables->tableB16_1[(code >> 5) - 16]);
	} else if (code >= 128) {
		tab = &(mp4_tables->tableB16_2[(code >> 2) - 32]);
	} else if (code >= 8) {
		tab = &(mp4_tables->tableB16_3[(code >> 0) - 8]);
	} else {
		/* invalid Huffman code */
		return (tab_type *) NULL;
	}
	flushbits(tab->len);
	return tab;
}

/***/

tab_type *vldTableB17(int code) {
	tab_type *tab;

	if (code >= 512) {
		tab = &(mp4_tables->tableB17_1[(code >> 5) - 16]);
	} else if (code >= 128) {
		tab = &(mp4_tables->tableB17_2[(code >> 2) - 32]);
	} else if (code >= 8) {
		tab = &(mp4_tables->tableB17_3[(code >> 0) - 8]);
	} else {
		/* invalid Huffman code */
		return (tab_type *) NULL;
	}
	flushbits(tab->len);
	return tab;
}

/***/
