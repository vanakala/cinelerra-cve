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
/// clearblock.c //

#include "portab.h"

/**
 *
**/

// zeros a memory block
//
void clearblock (short *psBlock)
{
  uint32_t* pu32_b;
  int i;

	pu32_b = (uint32_t *) psBlock;

  for (i = 0; i < 4; i++)
  {
    pu32_b[0] = pu32_b[1] = pu32_b[2] = pu32_b[3] = 
		pu32_b[4] = pu32_b[5] = pu32_b[6] = pu32_b[7] = 0;
    pu32_b += 8; // 16 coeff zeroed
  }
}
