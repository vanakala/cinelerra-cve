/* putbits.c, bit-level output                                              */

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

extern FILE *outfile; /* the only global var we need here */

/* private data */
static unsigned char outbfr;
static int outcnt;
static int bytecnt;

void slice_initbits(slice_engine_t *engine)
{
	engine->outcnt = 8;
	engine->slice_size = 0;
}

void slice_testbits(slice_engine_t *engine)
{
	int i;
	printf("slice test size %x outcnt %d outbfr %x\n", engine->slice_size, engine->outcnt, engine->outbfr << engine->outcnt);
/*
 * 	for(i = 0; i < engine->slice_size; i++)
 * 		printf("%02x ", engine->slice_buffer[i]);
 * 	printf("%x\n", engine->outbfr << engine->outcnt);
 */
}

void slice_putc(slice_engine_t *engine, unsigned char c)
{
	if(engine->slice_size >= engine->slice_allocated)
	{
		long new_allocation = (engine->slice_allocated > 0) ? (engine->slice_allocated * 2) : 64;
		unsigned char *new_buffer = calloc(1, new_allocation);
		if(engine->slice_buffer)
		{
			memcpy(new_buffer, engine->slice_buffer, engine->slice_size);
			free(engine->slice_buffer);
		}
		engine->slice_buffer = new_buffer;
		engine->slice_allocated = new_allocation;
	}
	engine->slice_buffer[engine->slice_size++] = c;
}

void slice_putbits(slice_engine_t *engine, long val, int n)
{
	int i;
	unsigned int mask;

	mask = 1 << (n - 1); /* selects first (leftmost) bit */

	for(i = 0; i < n; i++)
	{
    	engine->outbfr <<= 1;

    	if(val & mask)
    		engine->outbfr |= 1;

    	mask >>= 1; /* select next bit */
    	engine->outcnt--;

    	if(engine->outcnt == 0) /* 8 bit buffer full */
    	{
    		slice_putc(engine, engine->outbfr);
    		engine->outcnt = 8;
			engine->outbfr = 0;
    	}
	}
}

/* zero bit stuffing to next byte boundary (5.2.3, 6.2.1) */
void slice_alignbits(slice_engine_t *engine)
{
    if(engine->outcnt != 8)
        slice_putbits(engine, 0, engine->outcnt);
}

void slice_finishslice(slice_engine_t *engine)
{
	int i;
	slice_alignbits(engine);

	if(!fwrite(engine->slice_buffer, 1, engine->slice_size, outfile))
	{
		perror("Write error");
	}
	bytecnt += engine->slice_size;
}



/* initialize buffer, call once before first putbits or alignbits */
void mpeg2_initbits()
{
	outcnt = 8;
	bytecnt = 0;
}


/* write rightmost n (0<=n<=32) bits of val to outfile */
void mpeg2enc_putbits(val,n)
int val;
int n;
{
  int i;
  unsigned int mask;

  mask = 1 << (n-1); /* selects first (leftmost) bit */

  for (i=0; i<n; i++)
  {
    outbfr <<= 1;

    if (val & mask)
      outbfr|= 1;

    mask >>= 1; /* select next bit */
    outcnt--;

    if (outcnt==0) /* 8 bit buffer full */
    {
      putc(outbfr,outfile);
      outcnt = 8;
      bytecnt++;
    }
  }
}

/* zero bit stuffing to next byte boundary (5.2.3, 6.2.1) */
void alignbits()
{
  if (outcnt!=8)
    mpeg2enc_putbits(0,outcnt);
}

/* return total number of generated bits */
double bitcount()
{
	return (double)8 * bytecnt + (8 - outcnt);
}
