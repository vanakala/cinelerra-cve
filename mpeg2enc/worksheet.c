#include <stdio.h>
#include <stdlib.h>
#include <time.h>



static unsigned long  MMX_AVGDIFF_1[]         = {0x00010001, 0x00010001};
static unsigned long  MMX_ACCUM_AND[]         = {0xffffffff, 0x00000000};

void inline mmx_start_block()
{
	asm(" 
		.align 8
		pxor %%mm7, %%mm7;     // Zero totals
		" : : );
}

void inline mmx_avgdiff(unsigned char *p1, unsigned char *p2, unsigned char *p3)
{
	asm("
		.align 8
		movq		(%%ebx),	   %%mm0;	   // Load 8 pixels from a
		pxor        %%mm4,         %%mm4;      // Zero out temp for unpacking a
		movq        %%mm0,         %%mm2;      // Make a copy of a for unpacking
		movq        (%%ecx),       %%mm1;	   // Load 8 pixels from b
		pxor        %%mm3,         %%mm3;      // Zero out b's upper unpacked destination
		punpcklbw   %%mm4,         %%mm2;	   // Unpack lower 4 pixels from a for addition
		movq        %%mm1,         %%mm5;      // Copy b for unpacking
		punpckhbw   %%mm4,         %%mm0;      // Unpack upper 4 pixels from a for addition
		punpcklbw   %%mm3,         %%mm5;      // Unpack lower 4 pixels from b for addition
		paddw       %%mm2,         %%mm5;      // Add lower a and lower b unpacked
		punpckhbw   %%mm3,         %%mm1;      // Unpack upper 4 pixels from b for addition
		paddw       %%mm0,         %%mm1;      // Add upper a and upper b unpacked
 		movq        (%%edx),       %%mm2;      // Load c for difference
 		paddw       MMX_AVGDIFF_1, %%mm5;      // Add 1 to the result of lower a + b
		pxor        %%mm4,         %%mm4;      // Zero out temp for c unpacking
 		movq        %%mm2,         %%mm3;      // Make a copy of c for unpacking
 		paddw       MMX_AVGDIFF_1, %%mm1;      // Add 1 to the result of upper a + b
		punpcklbw   %%mm4,         %%mm3;      // Unpack lower 4 pixels from c for subtraction
		punpckhbw   %%mm4,         %%mm2;      // Unpack upper 4 pixels from c
		movq        %%mm3,         %%mm0;      // Make a copy of lower c for absdiff
		psraw       $1,            %%mm5;      // Divide result of lower a + b by 2
		movq        %%mm2,         %%mm4;      // Make a copy of upper c for absdiff
		psraw       $1,            %%mm1;      // Divide result of upper a + b by 2
		psubusw     %%mm5,         %%mm3;      // Subtract lower pixels one way
		psubusw     %%mm1,         %%mm2;      // Subtract upper pixels one way
		psubusw     %%mm0,         %%mm5;      // Subtract lower pixels the other way
		por         %%mm5,         %%mm3;      // Or the result of the lower pixels
		psubusw     %%mm4,         %%mm1;      // Subtract upper pixels the other way
		por         %%mm1,         %%mm2;      // Or the result of the upper pixels
		paddw       %%mm3,         %%mm7;      // Accumulate lower pixels
		paddw       %%mm2,         %%mm7;      // Accumulate upper pixels
		"
		:
		: "b" (p1), "c" (p2), "d" (p3));
}

unsigned int mmx_accum_avgdiff()
{
	unsigned long long r = 0;
	asm("
		.align 8
		pxor            %%mm5,  %%mm5;         // Clear temp for unpacking
		movq            %%mm7,  %%mm6;         // Make a copy for unpacking
		punpcklwd       %%mm5,  %%mm6;         // Unpack lower 2 pixels for accumulation
		punpckhwd       %%mm5,  %%mm7;         // Unpack high 2 pixels for accumulation
 		paddw           %%mm6,  %%mm7;         // Add 2 doublewords in each register
 		movq            %%mm7,  %%mm6;         // Copy the result for a final add
 		pand            MMX_ACCUM_AND, %%mm7;  // And the result for accumulation
 		psrlq           $32,    %%mm6;         // Shift the copy right for accumulation
 		paddd           %%mm6,  %%mm7;         // Add the results
 		movq            %%mm7,  (%%ebx);       // Store result
		emms;
		"
		: :  "b" (&r));

	return (unsigned int)r;
}


unsigned int mmx_test(unsigned char *result)
{
	unsigned long long r = 255;
	asm("
		.align 8
		movq    (%%ecx),    %%mm0;
		movq    (%%ecx),    %%mm1;
		paddd   %%mm0,      %%mm1;
		movq    %%mm1,    (%%ebx);
		movq    %%mm0,    (%%ecx);
		"
		:
		: "b" (result), "c" (&r));
	return r;
}

int main(int argc, char *argv[])
{
	unsigned char pixels1[9] = { 13, 13, 12, 11, 11, 10, 9, 9, 10 };
	unsigned char pixels3[8] = { 15, 10, 7, 8, 14, 19, 21, 20 };
	unsigned char *p1, *p2, *p3;
	unsigned int result;
	
	p1 = &pixels1[0];
	p2 = &pixels1[1];
	p3 = &pixels3[0];
	
	printf("%d %d %d %d %d %d %d %d %d\n", p1[0], p1[1], p1[2], p1[3], p1[4], p1[5], p1[6], p1[7], p1[8]);
	printf("%d %d %d %d %d %d %d %d\n", p3[0], p3[1], p3[2], p3[3], p3[4], p3[5], p3[6], p3[7]);
	printf("-----------------------\n");
	mmx_start_block();
	mmx_avgdiff(p1, p2, p3);
	result = mmx_accum_avgdiff();
//	p3[0] = p3[1] = p3[2] = p3[4] = p3[5] = p3[6] = p3[7] = 0;
//	result = mmx_test(p3);
	printf("%d %d %d %d %d %d %d %d %d\n", p1[0], p1[1], p1[2], p1[3], p1[4], p1[5], p1[6], p1[7], p1[8]);
	printf("%d %d %d %d %d %d %d %d\n", p3[0], p3[1], p3[2], p3[3], p3[4], p3[5], p3[6], p3[7]);
	printf("%d\n", result);
}
