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

#include "basic_prediction.h"

// Purpose: specialized basic motion compensation routines
void CopyBlock(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy;

	long *lpSrc = (long *) Src;
	long *lpDst = (long *) Dst;
	int lpStride = Stride >> 2;

	for (dy = 0; dy < 8; dy++) {
		lpDst[0] = lpSrc[0];
		lpDst[1] = lpSrc[1];
		lpSrc += lpStride;
		lpDst += lpStride;
	}
}
#define CopyBlockHorLoop \
	"movb (%%esi), %%al\n" 	\
	"incl %%esi\n" 		\
	"movb (%%esi), %%cl\n" 	\
	"addl %%ecx, %%eax\n" 	\
	"incl %%eax\n" 		\
	"shrl $1, %%eax\n" 	\
	"movb %%al, (%%edi)\n" 	\
	"incl %%edi\n"

// input: esi
// output: edi
// modifies: eax, ebx, edx
#define CopyBlockHorLoopFast \
	"movl (%%esi), %%edx\n"	\
	"movl 1(%%esi), %%ebx\n"\
	"movl %%edx, %%eax\n"	\
	"xorl %%ebx, %%edx\n"	\
	"shrl $1, %%edx\n"	\
	"adcl %%ebx, %%eax\n"	\
	"rcrl $1, %%eax\n"	\
	"andl $0x808080, %%edx\n"\
	"addl %%edx, %%eax\n"	\
	"movl %%eax, (%%edi)\n"	\
	"addl $4, %%esi\n"	\
	"addl $4, %%edi\n"

#define CopyBlockVerLoopFast \
	"movl (%%esi), %%edx\n"	\
	"movl (%%esi,%%ecx), %%ebx\n"\
	"movl %%edx, %%eax\n"	\
	"xorl %%ebx, %%edx\n"	\
	"shrl $1, %%edx\n"	\
	"adcl %%ebx, %%eax\n"	\
	"rcrl $1, %%eax\n"	\
	"andl $0x808080, %%edx\n"\
	"addl %%edx, %%eax\n"	\
	"movl %%eax, (%%edi)\n"	\
	"addl $4, %%esi\n"	\
	"addl $4, %%edi\n"


#define CopyBlockHorLoopRound \
	"movb (%%esi), %%al\n" 	\
	"incl %%esi\n" 		\
	"movb (%%esi), %%cl\n" 	\
	"addl %%ecx, %%eax\n" 	\
	"shrl $1, %%eax\n" 	\
	"movb %%al, (%%edi)\n" 	\
	"incl %%edi\n"

#define CopyBlockVerLoop \
    "movb (%%esi), %%al\n" 	\
    "movb (%%esi,%%ebx), %%cl\n" \
    "addl %%ecx, %%eax\n" 	\
    "incl %%eax\n" 		\
    "shrl $1, %%eax\n" 		\
    "movb %%al, (%%edi)\n" 	\
    "incl %%esi\n" 		\
    "incl %%edi\n"

#define CopyBlockVerLoopRound \
    "movb (%%esi), %%al\n" 	\
    "movb (%%esi,%%ebx), %%cl\n" \
    "addl %%ecx, %%eax\n" 	\
    "shrl $1, %%eax\n" 		\
    "movb %%al, (%%edi)\n" 	\
    "incl %%esi\n" 		\
    "incl %%edi\n"

#define CopyBlockHorVerLoop(STEP) \
    "movb " #STEP "(%%esi), %%al\n" 		\
    "movb " #STEP "+1(%%esi), %%cl\n" 		\
    "addl %%ecx, %%eax\n" 			\
    "movb " #STEP "(%%esi, %%ebx), %%cl\n" 	\
    "addl %%ecx, %%eax\n" 			\
    "movb " #STEP "+1(%%esi, %%ebx), %%cl\n" 	\
    "addl %%ecx, %%eax\n" 			\
    "addl $2, %%eax\n" 				\
    "shrl $2, %%eax\n" 				\
    "movb %%al, " #STEP "(%%edi)\n"

#define CopyBlockHorVerLoopRound(STEP) \
    "movb " #STEP "(%%esi), %%al\n" 		\
    "movb " #STEP "+1(%%esi), %%cl\n" 		\
    "addl %%ecx, %%eax\n" 			\
    "movb " #STEP "(%%esi, %%ebx), %%cl\n" 	\
    "addl %%ecx, %%eax\n" 			\
    "movb " #STEP "+1(%%esi, %%ebx), %%cl\n" 	\
    "addl %%ecx, %%eax\n" 			\
    "incl %%eax\n" 				\
    "shrl $2, %%eax\n" 				\
    "movb %%al, " #STEP "(%%edi)\n"

/**/

void CopyBlockHor(unsigned char * Src, unsigned char * Dst, int Stride)
{
	__asm__ (
	"movl %2, %%esi\n"
	"movl %3, %%edi\n"
	"pushl %%ebx\n"
	"1:\n"
	"pushl %%edx\n"

	CopyBlockHorLoopFast
	CopyBlockHorLoopFast

	"popl %%edx\n"
	"addl %%ecx, %%esi\n"
	"addl %%ecx, %%edi\n"
	"decl %%edx\n"
	"jnz 1b\n"
	"popl %%ebx\n"
	:
	: "c"(Stride-8), "d"(8), "g" (Src), "g"(Dst)
	: "esi", "edi"
	);
}
void CopyBlockVer(unsigned char * Src, unsigned char * Dst, int Stride)
{
	__asm__ (
	"movl %2, %%esi\n"
	"movl %3, %%edi\n"
	"pushl %%ebx\n"
	"1:\n"
	"pushl %%edx\n"

	CopyBlockVerLoopFast
	CopyBlockVerLoopFast

	"popl %%edx\n"
	"addl %%ecx, %%esi\n"
	"subl $8, %%esi\n"
	"addl %%ecx, %%edi\n"
	"subl $8, %%edi\n"
	"decl %%edx\n"
	"jnz 1b\n"
	"popl %%ebx\n"
	:
	: "c"(Stride), "d"(8), "g" (Src), "g"(Dst)
	: "esi", "edi"
	);
}
/*
void CopyBlockHor(unsigned char * Src, unsigned char * Dst, int Stride)
{
	__asm__ (
	"pushl %%ebx\n"
	"movl %1, %%ebx\n"
	"movl %4, %%esi\n"
	"movl %5, %%edi\n"
	"1:\n"

	CopyBlockHorLoop
	CopyBlockHorLoop
	CopyBlockHorLoop
	CopyBlockHorLoop

	CopyBlockHorLoop
	CopyBlockHorLoop
	CopyBlockHorLoop
	CopyBlockHorLoop

	"addl %%ebx, %%esi\n"
	"addl %%ebx, %%edi\n"
	"decl %%edx\n"
	"jnz 1b\n"
	"popl %%ebx\n"
	:
	: "a"(0), "g"(Stride-8), "c"(0), "d"(8), "g" (Src), "g"(Dst)
	);
}
*/
void CopyBlockHorRound(unsigned char * Src, unsigned char * Dst, int Stride)
{
	__asm__ (
	"movl %1, %%eax\n"
	"movl %4, %%esi\n"
	"movl %5, %%edi\n"
	"pushl %%ebx\n"
	"movl %%eax, %%ebx\n"
        "xorl %%eax, %%eax\n"
	"1:\n"

	CopyBlockHorLoopRound
	CopyBlockHorLoopRound
	CopyBlockHorLoopRound
	CopyBlockHorLoopRound

	CopyBlockHorLoopRound
	CopyBlockHorLoopRound
	CopyBlockHorLoopRound
	CopyBlockHorLoopRound

	"addl %%ebx, %%esi\n"
	"addl %%ebx, %%edi\n"
	"decl %%edx\n"
	"jnz 1b\n"
	"popl %%ebx\n"
	:
	: "a"(0), "g"(Stride-8), "c"(0), "d"(8), "g" (Src), "g"(Dst)
	: "esi", "edi"
	);
}

/**/
/*
void CopyBlockVer(unsigned char * Src, unsigned char * Dst, int Stride)
{
	__asm__ (
	"pushl %%ebx\n"
	"movl %1, %%ebx\n"
	"movl %4, %%esi\n"
	"movl %5, %%edi\n"
	"1:\n"
	CopyBlockVerLoop
	CopyBlockVerLoop
	CopyBlockVerLoop
	CopyBlockVerLoop

	CopyBlockVerLoop
	CopyBlockVerLoop
	CopyBlockVerLoop
	CopyBlockVerLoop

	"addl %%ebx, %%esi\n"
	"subl $8, %%esi\n"
	"addl %%ebx, %%edi\n"
	"subl $8, %%edi\n"
	"decl %%edx\n"
	"jnz 1b\n"
	"popl %%ebx\n"
	:
	: "a"(0), "g"(Stride), "c"(0), "d"(8), "g" (Src), "g"(Dst)
	);
}
*/
/**/
void CopyBlockVerRound(unsigned char * Src, unsigned char * Dst, int Stride)
{
	__asm__ (
	"movl %1, %%eax\n"
	"movl %4, %%esi\n"
	"movl %5, %%edi\n"
	"pushl %%ebx\n"
	"movl %%eax, %%ebx\n"
        "xorl %%eax, %%eax\n"
	"1:\n"
	CopyBlockVerLoopRound
	CopyBlockVerLoopRound
	CopyBlockVerLoopRound
	CopyBlockVerLoopRound

	CopyBlockVerLoopRound
	CopyBlockVerLoopRound
	CopyBlockVerLoopRound
	CopyBlockVerLoopRound

	"addl %%ebx, %%esi\n"
	"subl $8, %%esi\n"
	"addl %%ebx, %%edi\n"
	"subl $8, %%edi\n"
	"decl %%edx\n"
	"jnz 1b\n"
	"popl %%ebx\n"
	:
	: "a"(0), "g"(Stride), "c"(0), "d"(8), "g" (Src), "g"(Dst)
	: "esi", "edi"
	);
}/**/
void CopyBlockHorVer(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy, dx;

	for (dy = 0; dy < 8; dy++) {
		for (dx = 0; dx < 8; dx++) {
			Dst[dx] = (Src[dx] + Src[dx+1] + 
			Src[dx+Stride] + Src[dx+Stride+1] +2) >> 2; // horver interpolation with rounding
		}
		Src += Stride;
		Dst += Stride;
	}
}
/**/
void CopyBlockHorVerRound(unsigned char * Src, unsigned char * Dst, int Stride)
{
	int dy, dx;

	for (dy = 0; dy < 8; dy++) {
		for (dx = 0; dx < 8; dx++) {
			Dst[dx] = (Src[dx] + Src[dx+1] + 
								Src[dx+Stride] + Src[dx+Stride+1] +1) >> 2; // horver interpolation with rounding
		}
		Src += Stride;
		Dst += Stride;
	}
}
/** *** **/
void CopyMBlock(unsigned char * Src, unsigned char * Dst, int Stride)
{
    __asm__ 
    (
    "movl %0, %%eax\n"
    "movl %2, %%esi\n"
    "movl %3, %%edi\n"
    "pushl %%ebx\n"
    "movl %%eax, %%ebx\n"
    "1:\n"
    
    "movl (%%esi), %%eax\n"
    "movl %%eax, (%%edi)\n"
    "addl $4, %%esi\n"
    "addl $4, %%edi\n"
    
    "movl (%%esi), %%eax\n"
    "movl %%eax, (%%edi)\n"
    "addl $4, %%esi\n"
    "addl $4, %%edi\n"
    
    "movl (%%esi), %%eax\n"
    "movl %%eax, (%%edi)\n"
    "addl $4, %%esi\n"
    "addl $4, %%edi\n"
    
    "movl (%%esi), %%eax\n"
    "movl %%eax, (%%edi)\n"

    "addl %%ebx, %%esi\n"
    "addl %%ebx, %%edi\n"
    "decl %%edx\n"
    "jnz 1b\n"
    "popl %%ebx\n"
    : 
    : "g"(Stride-12), "d"(16), "g" (Src), "g" (Dst) 
    : "esi", "edi"
    );
}
/**/

void CopyMBlockHor(unsigned char * Src, unsigned char * Dst, int Stride)
{
	__asm__ (
	"movl %2, %%esi\n"
	"movl %3, %%edi\n"
	"pushl %%ebx\n"
	"1:\n"
	"pushl %%edx\n"

	CopyBlockHorLoopFast
	CopyBlockHorLoopFast
	CopyBlockHorLoopFast
	CopyBlockHorLoopFast

	"popl %%edx\n"
	"addl %%ecx, %%esi\n"
	"addl %%ecx, %%edi\n"
	"decl %%edx\n"
	"jnz 1b\n"
	"popl %%ebx\n"
	:
	: "c"(Stride-16), "d"(16), "g" (Src), "g"(Dst)
	: "esi", "edi"
	);
}
void CopyMBlockVer(unsigned char * Src, unsigned char * Dst, int Stride)
{
	__asm__ (
	"movl %2, %%esi\n"
	"movl %3, %%edi\n"
	"pushl %%ebx\n"
	"1:\n"
	"pushl %%edx\n"

	CopyBlockVerLoopFast
	CopyBlockVerLoopFast
	CopyBlockVerLoopFast
	CopyBlockVerLoopFast

	"popl %%edx\n"
	"addl %%ecx, %%esi\n"
	"subl $16, %%esi\n"
	"addl %%ecx, %%edi\n"
	"subl $16, %%edi\n"
	"decl %%edx\n"
	"jnz 1b\n"
	"popl %%ebx\n"
	:
	: "c"(Stride), "d"(16), "g" (Src), "g"(Dst)
	: "esi", "edi"
	);
}
/*
void CopyMBlockHor(unsigned char * Src, unsigned char * Dst, int Stride)
{
    __asm__ (
    "pushl %%ebx\n"
    "movl %1, %%ebx\n"
    "movl %4, %%esi\n"
    "movl %5, %%edi\n"
    "1:\n"
    CopyBlockHorLoop
    CopyBlockHorLoop
    CopyBlockHorLoop
    CopyBlockHorLoop

    CopyBlockHorLoop
    CopyBlockHorLoop
    CopyBlockHorLoop
    CopyBlockHorLoop

    CopyBlockHorLoop
    CopyBlockHorLoop
    CopyBlockHorLoop
    CopyBlockHorLoop

    CopyBlockHorLoop
    CopyBlockHorLoop
    CopyBlockHorLoop
    CopyBlockHorLoop
    
    "addl %%ebx, %%esi\n"
    "addl %%ebx, %%edi\n"
    "decl %%edx\n"
    "jnz 1b\n"
    "popl %%ebx\n"
    :
    : "a"(0), "g"(Stride-16), "c"(0), "d"(16), "g" (Src), "g"(Dst)
    );
}
*/
/*
void CopyMBlockVer(unsigned char * Src, unsigned char * Dst, int Stride)
{
    __asm__ (
    "pushl %%ebx\n"
    "movl %1, %%ebx\n"
    "movl %4, %%esi\n"
    "movl %5, %%edi\n"
    "1:\n"
    CopyBlockVerLoop
    CopyBlockVerLoop
    CopyBlockVerLoop
    CopyBlockVerLoop

    CopyBlockVerLoop
    CopyBlockVerLoop
    CopyBlockVerLoop
    CopyBlockVerLoop

    CopyBlockVerLoop
    CopyBlockVerLoop
    CopyBlockVerLoop
    CopyBlockVerLoop

    CopyBlockVerLoop
    CopyBlockVerLoop
    CopyBlockVerLoop
    CopyBlockVerLoop
    
    "addl %%ebx, %%esi\n"
    "subl $16, %%esi\n"
    "addl %%ebx, %%edi\n"
    "subl $16, %%edi\n"
    "decl %%edx\n"
    "jnz 1b\n"
    "popl %%ebx\n"
    :
    : "a"(0), "g"(Stride), "c"(0), "d"(16), "g" (Src), "g"(Dst)
    );
}
*/
void CopyMBlockHorRound(unsigned char * Src, unsigned char * Dst, int Stride)
{
    __asm__ (
    "movl %1, %%eax\n"
    "movl %4, %%esi\n"
    "movl %5, %%edi\n"
    "pushl %%ebx\n"
    "movl %%eax, %%ebx\n"
    "xorl %%eax, %%eax\n"
    "1:\n"
    CopyBlockHorLoopRound
    CopyBlockHorLoopRound
    CopyBlockHorLoopRound
    CopyBlockHorLoopRound

    CopyBlockHorLoopRound
    CopyBlockHorLoopRound
    CopyBlockHorLoopRound
    CopyBlockHorLoopRound

    CopyBlockHorLoopRound
    CopyBlockHorLoopRound
    CopyBlockHorLoopRound
    CopyBlockHorLoopRound

    CopyBlockHorLoopRound
    CopyBlockHorLoopRound
    CopyBlockHorLoopRound
    CopyBlockHorLoopRound
    
    "addl %%ebx, %%esi\n"
    "addl %%ebx, %%edi\n"
    "decl %%edx\n"
    "jnz 1b\n"
    "popl %%ebx\n"
    :
    : "a"(0), "g"(Stride-16), "c"(0), "d"(16), "g" (Src), "g"(Dst)
    : "esi", "edi"
    );
}
void CopyMBlockVerRound(unsigned char * Src, unsigned char * Dst, int Stride)
{
    __asm__ (
    "movl %1, %%eax\n"
    "movl %4, %%esi\n"
    "movl %5, %%edi\n"
    "pushl %%ebx\n"
    "movl %%eax, %%ebx\n"
    "xorl %%eax, %%eax\n"
    "1:\n"
    CopyBlockVerLoopRound
    CopyBlockVerLoopRound
    CopyBlockVerLoopRound
    CopyBlockVerLoopRound

    CopyBlockVerLoopRound
    CopyBlockVerLoopRound
    CopyBlockVerLoopRound
    CopyBlockVerLoopRound

    CopyBlockVerLoopRound
    CopyBlockVerLoopRound
    CopyBlockVerLoopRound
    CopyBlockVerLoopRound

    CopyBlockVerLoopRound
    CopyBlockVerLoopRound
    CopyBlockVerLoopRound
    CopyBlockVerLoopRound
    
    "addl %%ebx, %%esi\n"
    "subl $16, %%esi\n"
    "addl %%ebx, %%edi\n"
    "subl $16, %%edi\n"
    "decl %%edx\n"
    "jnz 1b\n"
    "popl %%ebx\n"
    :
    : "a"(0), "g"(Stride), "c"(0), "d"(16), "g" (Src), "g"(Dst)
    : "esi", "edi"
    );
}
/**/
void CopyMBlockHorVer(unsigned char * Src, unsigned char * Dst, int Stride)
{
    __asm__
    (
    "movl %1, %%eax\n"
    "movl %4, %%esi\n"
    "movl %5, %%edi\n"
    "pushl %%ebx\n"
    "movl %%eax, %%ebx\n"
    "xorl %%eax, %%eax\n"
    "1:\n"
    CopyBlockHorVerLoop(0)
    CopyBlockHorVerLoop(1)
    CopyBlockHorVerLoop(2)
    CopyBlockHorVerLoop(3)
    CopyBlockHorVerLoop(4)
    CopyBlockHorVerLoop(5)
    CopyBlockHorVerLoop(6)
    CopyBlockHorVerLoop(7)
    CopyBlockHorVerLoop(8)
    CopyBlockHorVerLoop(9)
    CopyBlockHorVerLoop(10)
    CopyBlockHorVerLoop(11)
    CopyBlockHorVerLoop(12)
    CopyBlockHorVerLoop(13)
    CopyBlockHorVerLoop(14)
    CopyBlockHorVerLoop(15)

    "addl %%ebx, %%esi\n"
    "addl %%ebx, %%edi\n"
    "decl %%edx\n"
    "jnz 1b\n"
    "popl %%ebx\n"
    :
    : "a"(0), "g"(Stride), "c" (0), "d" (16), "g" (Src), "g" (Dst)
    : "esi", "edi"
    );
}

void CopyMBlockHorVerRound(unsigned char * Src, unsigned char * Dst, int Stride)
{
    __asm__
    (
    "movl %1, %%eax\n"
    "movl %4, %%esi\n"
    "movl %5, %%edi\n"
    "pushl %%ebx\n"
    "movl %%eax, %%ebx\n"
    "xorl %%eax, %%eax\n"
    "1:\n"
    CopyBlockHorVerLoopRound(0)
    CopyBlockHorVerLoopRound(1)
    CopyBlockHorVerLoopRound(2)
    CopyBlockHorVerLoopRound(3)
    CopyBlockHorVerLoopRound(4)
    CopyBlockHorVerLoopRound(5)
    CopyBlockHorVerLoopRound(6)
    CopyBlockHorVerLoopRound(7)
    CopyBlockHorVerLoopRound(8)
    CopyBlockHorVerLoopRound(9)
    CopyBlockHorVerLoopRound(10)
    CopyBlockHorVerLoopRound(11)
    CopyBlockHorVerLoopRound(12)
    CopyBlockHorVerLoopRound(13)
    CopyBlockHorVerLoopRound(14)
    CopyBlockHorVerLoopRound(15)

    "addl %%ebx, %%esi\n"
    "addl %%ebx, %%edi\n"
    "decl %%edx\n"
    "jnz 1b\n"
    "popl %%ebx\n"
    :
    : "a"(0), "g"(Stride), "c" (0), "d" (16), "g" (Src), "g" (Dst)
    : "esi", "edi"
    );
}
