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
 * Andrea Graziani
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// store.c //

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "mp4_vars.h"

#include "debug.h"
#include "store.h"

/**
 *
**/

static void store_yuv (char *name, unsigned char *src, int offset, int incr, int width, int height);
static void putbyte (int c);

/**/

FILE * outfile;
int create_flag = 1;
const char mode_create[] = "wb";
const char mode_open[] = "ab";

/***/

// Purpose: store a frame in yuv format
void storeframe (unsigned char *src[], int width, int height)
{
	int offset = 0;
	int hor_size = mp4_state->horizontal_size;

  store_yuv (mp4_state->outputname, src[0], offset, width, hor_size, height);

  offset >>= 1;
  width >>= 1;
  height >>= 1;
	hor_size >>= 1;

  store_yuv (mp4_state->outputname, src[1], offset, width, hor_size, height);
  store_yuv (mp4_state->outputname, src[2], offset, width, hor_size, height);
}

/***/

static void store_yuv (char *name, unsigned char *src, int offset, 
                        int incr, int width, int height)
{
  int i;
  unsigned char *p;
	const char * mode = create_flag ? mode_create : mode_open;


	if (create_flag)
		create_flag = 0;

  if ((outfile = fopen (name, mode)) == NULL)
  {
    _Print ("Error: Couldn't append to %s\n", name);
    exit(0);
  }

  for (i = 0; i < height; i++)
  {
    p = src + offset + incr * i;
		fwrite(p, width, 1, outfile);
  }

  fclose (outfile);
}
