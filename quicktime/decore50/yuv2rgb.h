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
 * http://www.projectmayo.com/opendivx/license.php .                      *
 *                                                                        *
 **************************************************************************/
/**
*  Copyright (C) 2001 - Project Mayo
 *
 * Andrea Graziani (Ag)
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
**/
// yuvrgb.h //

// 13.Feb.01 John Funnell: a negative height value now signifies that the output should be flipped


#include "portab.h"

#ifndef _YUVRGB_H_
#define _YUVRGB_H_

void yuv2rgb_24(
	uint8_t *puc_y, int stride_y, 
	uint8_t *puc_u, 
	uint8_t *puc_v, int stride_uv, 
  uint8_t *puc_out, 
	int width_y, int height_y,
	unsigned int stride_out);
void yuv2rgb_32(
	uint8_t *puc_y, int stride_y, 
  uint8_t *puc_u, uint8_t *puc_v, int stride_uv, 
  uint8_t *puc_out, 
	int width_y, int height_y,
	unsigned int stride_out);
void yuv2rgb_555(
	uint8_t *puc_y, int stride_y, 
	uint8_t *puc_u, 
	uint8_t *puc_v, int stride_uv, 
  uint8_t *puc_out, 
	int width_y, int height_y,
	unsigned int stride_out);
void yuv2rgb_565(
	uint8_t *puc_y, int stride_y, 
  uint8_t *puc_u, 
	uint8_t *puc_v, int stride_uv, 
  uint8_t *puc_out, 
	int width_y, int height_y,
	unsigned int stride_out);

void yuv12_out(uint8_t *puc_y, int stride_y, 
  uint8_t *puc_u, uint8_t *puc_v, int stride_uv, 
  uint8_t *puc_out[], int width_y, int height_y,
	unsigned int stride_out);
void yuy2_out(uint8_t *puc_y, int stride_y, 
  uint8_t *puc_u, uint8_t *puc_v, int stride_uv, 
  uint8_t *puc_out, int width_y, int height_y,
	unsigned int stride_out); 
void uyvy_out(uint8_t *puc_y, int stride_y, 
  uint8_t *puc_u, uint8_t *puc_v, int stride_uv, 
  uint8_t *puc_out, int width_y, int height_y,
	unsigned int stride_out);

#endif // _YUVRGB_H_
