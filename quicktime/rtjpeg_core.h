/* 
   bttvgrab 0.15.4 [1999-03-23]
   (c) 1998, 1999 by Joerg Walter <trouble@moes.pmnet.uni-oldenburg.de>
   Maintained by: Joerg Walter
   Current version at http:/*moes.pmnet.uni-oldenburg.de/bttvgrab/ */

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   This file is a modified version of RTjpeg 0.1.2, (C) Justin Schoeman 1998
*/

#include <asm/types.h>

/*#define MMX_TRACE */

/*#define MMX */

#ifdef MMX
#include "mmx.h"
#endif

typedef unsigned char __u8;
typedef signed char __s8;
typedef unsigned short __u16;
typedef signed short __s16;
typedef unsigned long __u32;
typedef signed long __s32;
typedef unsigned long long __u64;

typedef struct
{
#ifndef MMX
	static __s32 RTjpeg_ws[64+31];
#endif
	__u8 RTjpeg_alldata[2*64+4*64+4*64+4*64+4*64+32];

	__s16 *RTjpeg_block;
	__s32 *RTjpeg_lqt;
	__s32 *RTjpeg_cqt;
	__u32 *RTjpeg_liqt;
	__u32 *RTjpeg_ciqt;

	unsigned char RTjpeg_lb8;
	unsigned char RTjpeg_cb8;
	int RTjpeg_width, RTjpeg_height;
	int RTjpeg_Ywidth, RTjpeg_Cwidth;
	int RTjpeg_Ysize, RTjpeg_Csize;

	__s16 *RTjpeg_old=NULL;

#ifdef MMX
	mmx_t RTjpeg_lmask;
	mmx_t RTjpeg_cmask;
#else
	__u16 RTjpeg_lmask;
	__u16 RTjpeg_cmask;
#endif
	int RTjpeg_mtest=0;
	
	unsigned long tbls[128];
} rtjpeg_t;

extern void RTjpeg_init_Q(__u8 Q);
extern void RTjpeg_init_compress(long unsigned int *buf, int width, int height, __u8 Q);
extern void RTjpeg_init_decompress(long unsigned int *buf, int width, int height);
extern int RTjpeg_compressYUV420(__s8 *sp, unsigned char *bp);
extern int RTjpeg_compressYUV422(__s8 *sp, unsigned char *bp);
extern void RTjpeg_decompressYUV420(__s8 *sp, __u8 *bp);
extern void RTjpeg_decompressYUV422(__s8 *sp, __u8 *bp);
extern int RTjpeg_compress8(__s8 *sp, unsigned char *bp);
extern void RTjpeg_decompress8(__s8 *sp, __u8 *bp);

extern void RTjpeg_init_mcompress(void);
extern int RTjpeg_mcompressYUV420(__s8 *sp, unsigned char *bp, __u16 lmask, __u16 cmask);
extern int RTjpeg_mcompressYUV422(__s8 *sp, unsigned char *bp, __u16 lmask, __u16 cmask);
extern int RTjpeg_mcompress8(__s8 *sp, unsigned char *bp, __u16 lmask);
extern void RTjpeg_set_test(int i);

extern void RTjpeg_yuv420rgb(__u8 *buf, __u8 *rgb, int stride);
extern void RTjpeg_yuv422rgb(__u8 *buf, __u8 *rgb, int stride);
extern void RTjpeg_yuvrgb8(__u8 *buf, __u8 *rgb, int stride);
extern void RTjpeg_yuvrgb16(__u8 *buf, __u8 *rgb, int stride);
extern void RTjpeg_yuvrgb24(__u8 *buf, __u8 *rgb, int stride);
extern void RTjpeg_yuvrgb32(__u8 *buf, __u8 *rgb, int stride);
		  

