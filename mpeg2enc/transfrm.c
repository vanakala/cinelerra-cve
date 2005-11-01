/* transfrm.c,  forward / inverse transformation                            */

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

#include "config.h"
#include "global.h"
#include <stdio.h>
#include <math.h>
#include "cpu_accel.h"

#ifdef X86_CPU
extern void fdct_mmx( int16_t * blk );
extern void idct_mmx( int16_t * blk, unsigned char *temp );

void add_pred_mmx (uint8_t *pred, uint8_t *cur,
				   int lx, int16_t *blk);
void sub_pred_mmx (uint8_t *pred, uint8_t *cur,
				   int lx, int16_t *blk);
#endif

extern void fdct( int16_t *blk );
extern void idct( int16_t *blk, unsigned char *temp );



/* private prototypes*/
static void add_pred (uint8_t *pred, uint8_t *cur,
					  int lx, int16_t *blk);
static void sub_pred (uint8_t *pred, uint8_t *cur,
					  int lx, int16_t *blk);

/*
  Pointers to version of transform and prediction manipulation
  routines to be used..
 */

static void (*pfdct)( int16_t * blk );
static void (*pidct)( int16_t * blk , unsigned char *temp);
static void (*padd_pred) (uint8_t *pred, uint8_t *cur,
						  int lx, int16_t *blk);
static void (*psub_pred) (uint8_t *pred, uint8_t *cur,
						  int lx, int16_t *blk);

/*
  Initialise DCT transformation routines
  Currently just activates MMX routines if available
 */


void init_transform_hv()
{
	int flags;
	flags = cpu_accel();

#ifdef X86_CPU
	if( (flags & ACCEL_X86_MMX) ) /* MMX CPU */
	{
		if(verbose) fprintf( stderr, "SETTING MMX for TRANSFORM!\n");
		pfdct = fdct_mmx;
		pidct = idct_mmx;
		padd_pred = add_pred_mmx;
		psub_pred = sub_pred_mmx;
	}
	else
#endif
	{
		pfdct = fdct;
		pidct = idct;
		padd_pred = add_pred;
		psub_pred = sub_pred;

	}
}

/* add prediction and prediction error, saturate to 0...255 */
static void add_pred(unsigned char *pred,
	unsigned char *cur,
	int lx,
	short *blk)
{
	register int j;

	for (j=0; j<8; j++)
	{
/*
 *     	for (i=0; i<8; i++)
 *     	  cur[i] = clp[blk[i] + pred[i]];
 */
    	cur[0] = clp[blk[0] + pred[0]];
    	cur[1] = clp[blk[1] + pred[1]];
    	cur[2] = clp[blk[2] + pred[2]];
    	cur[3] = clp[blk[3] + pred[3]];
    	cur[4] = clp[blk[4] + pred[4]];
    	cur[5] = clp[blk[5] + pred[5]];
    	cur[6] = clp[blk[6] + pred[6]];
    	cur[7] = clp[blk[7] + pred[7]];
 
    	blk += 8;
    	cur += lx;
    	pred += lx;
	}
}

/* subtract prediction from block data */
static void sub_pred(unsigned char *pred,
	unsigned char *cur,
	int lx,
	short *blk)
{
	register int j;

	for (j=0; j<8; j++)
	{
/*
 *     	for (i=0; i<8; i++)
 *     		blk[i] = cur[i] - pred[i];
 */
    	blk[0] = cur[0] - pred[0];
    	blk[1] = cur[1] - pred[1];
    	blk[2] = cur[2] - pred[2];
    	blk[3] = cur[3] - pred[3];
    	blk[4] = cur[4] - pred[4];
    	blk[5] = cur[5] - pred[5];
    	blk[6] = cur[6] - pred[6];
    	blk[7] = cur[7] - pred[7];

    	blk += 8;
    	cur += lx;
    	pred += lx;
	}
}

void transform_engine_loop(transform_engine_t *engine)
{
	while(!engine->done)
	{
		pthread_mutex_lock(&(engine->input_lock));
		
		if(!engine->done)
		{
			pict_data_s *picture = engine->picture;
			uint8_t **pred = engine->pred;
			uint8_t **cur = engine->cur;
			mbinfo_s *mbi = picture->mbinfo;
			int16_t (*blocks)[64] = picture->blocks;
			int i, j, i1, j1, k, n, cc, offs, lx;

			k = (engine->start_row / 16) * (width / 16);

			for(j = engine->start_row; j < engine->end_row; j += 16)
    			for(i = 0; i < width; i += 16)
    			{
					mbi[k].dctblocks = &blocks[k * block_count];

    				for(n = 0; n < block_count; n++)
    				{
/* color component index */
        				cc = (n < 4) ? 0 : (n & 1) + 1; 
        				if(cc == 0)
        				{
/* A.Stevens Jul 2000 Record dct blocks associated with macroblock */
/* We'll use this for quantisation calculations                    */
/* luminance */
							if ((picture->pict_struct == FRAME_PICTURE) && mbi[k].dct_type)
							{
/* field DCT */
								offs = i + ((n & 1) << 3) + width * (j + ((n & 2) >> 1));
								lx = width << 1;
							}
							else
							{
/* frame DCT */
								offs = i + ((n & 1) << 3) + width2 * (j + ((n & 2) << 2));
								lx = width2;
							}

							if (picture->pict_struct == BOTTOM_FIELD)
								offs += width;
        				}
        				else
        				{
/* chrominance */
/* scale coordinates */
        					i1 = (chroma_format == CHROMA444) ? i : i >> 1;
        					j1 = (chroma_format != CHROMA420) ? j : j >> 1;

        					if ((picture->pict_struct==FRAME_PICTURE) && mbi[k].dct_type
            					&& (chroma_format!=CHROMA420))
        					{
/* field DCT */
            					offs = i1 + (n&8) + chrom_width*(j1+((n&2)>>1));
            					lx = chrom_width<<1;
        					}
        					else
        					{
/* frame DCT */
            					offs = i1 + (n&8) + chrom_width2*(j1+((n&2)<<2));
            					lx = chrom_width2;
        					}

        					if(picture->pict_struct==BOTTOM_FIELD)
            					offs += chrom_width;
        				}

						(*psub_pred)(pred[cc]+offs,cur[cc]+offs,lx,
									 blocks[k*block_count+n]);
						(*pfdct)(blocks[k*block_count+n]);
    				}

    				k++;
    			}
		}
		pthread_mutex_unlock(&(engine->output_lock));
	}
}

/* subtract prediction and transform prediction error */
void transform(pict_data_s *picture,
	uint8_t *pred[], uint8_t *cur[])
{
	int i;
/* Start loop */
	for(i = 0; i < processors; i++)
	{
		transform_engines[i].picture = picture;
		transform_engines[i].pred = pred;
		transform_engines[i].cur = cur;
		pthread_mutex_unlock(&(transform_engines[i].input_lock));
	}

/* Wait for completion */
	for(i = 0; i < processors; i++)
	{
		pthread_mutex_lock(&(transform_engines[i].output_lock));
	}
}



void start_transform_engines()
{
	int i;
	int rows_per_processor = (int)((float)height2 / 16 / processors + 0.5);
	int current_row = 0;
	pthread_attr_t  attr;
	pthread_mutexattr_t mutex_attr;

	pthread_mutexattr_init(&mutex_attr);
	pthread_attr_init(&attr);
	transform_engines = calloc(1, sizeof(transform_engine_t) * processors);
	for(i = 0; i < processors; i++)
	{
		transform_engines[i].start_row = current_row * 16;
		current_row += rows_per_processor;
		if(current_row > height2 / 16) current_row = height2 / 16;
		transform_engines[i].end_row = current_row * 16;
		pthread_mutex_init(&(transform_engines[i].input_lock), &mutex_attr);
		pthread_mutex_lock(&(transform_engines[i].input_lock));
		pthread_mutex_init(&(transform_engines[i].output_lock), &mutex_attr);
		pthread_mutex_lock(&(transform_engines[i].output_lock));
		transform_engines[i].done = 0;
		pthread_create(&(transform_engines[i].tid), 
			&attr, 
			(void*)transform_engine_loop, 
			&transform_engines[i]);
	}
}

void stop_transform_engines()
{
	int i;
	for(i = 0; i < processors; i++)
	{
		transform_engines[i].done = 1;
		pthread_mutex_unlock(&(transform_engines[i].input_lock));
		pthread_join(transform_engines[i].tid, 0);
		pthread_mutex_destroy(&(transform_engines[i].input_lock));
		pthread_mutex_destroy(&(transform_engines[i].output_lock));
	}
	free(transform_engines);
}









/* inverse transform prediction error and add prediction */
void itransform_engine_loop(transform_engine_t *engine)
{
	while(!engine->done)
	{
		pthread_mutex_lock(&(engine->input_lock));

		if(!engine->done)
		{
			pict_data_s *picture = engine->picture;
			uint8_t **pred = engine->pred;
			uint8_t **cur = engine->cur;
			int i, j, i1, j1, k, n, cc, offs, lx;
    		mbinfo_s *mbi = picture->mbinfo;
/* Its the quantised / inverse quantised blocks were interested in
   for inverse transformation */
			int16_t (*blocks)[64] = picture->qblocks;

			k = (engine->start_row / 16) * (width / 16);

			for(j = engine->start_row; j < engine->end_row; j += 16)
				for(i = 0; i < width; i += 16)
				{
					for(n = 0; n < block_count; n++)
					{
    				  	cc = (n < 4) ? 0 : (n & 1) + 1; /* color component index */

    				  	if(cc == 0)
    					{
/* luminance */
    						if((picture->pict_struct == FRAME_PICTURE) && mbi[k].dct_type)
    						{
/* field DCT */
        						offs = i + ((n & 1) << 3) + width * (j + ((n & 2) >> 1));
        						lx = width<<1;
    						}
    						else
    						{
/* frame DCT */
        						offs = i + ((n & 1) << 3) + width2 * (j + ((n & 2) << 2));
        						lx = width2;
    						}

    						if(picture->pict_struct == BOTTOM_FIELD)
        					offs += width;
    					}
    					else
    					{
/* chrominance */

/* scale coordinates */
    						i1 = (chroma_format==CHROMA444) ? i : i>>1;
    						j1 = (chroma_format!=CHROMA420) ? j : j>>1;

    						if((picture->pict_struct == FRAME_PICTURE) && mbi[k].dct_type
        						&& (chroma_format != CHROMA420))
    						{
/* field DCT */
        						offs = i1 + (n & 8) + chrom_width * (j1 + ((n & 2) >> 1));
        						lx = chrom_width << 1;
    						}
    						else
    						{
/* frame DCT */
        						offs = i1 + (n&8) + chrom_width2 * (j1 + ((n & 2) << 2));
        						lx = chrom_width2;
    						}

    						if(picture->pict_struct == BOTTOM_FIELD)
        						offs += chrom_width;
    				    }

//pthread_mutex_lock(&test_lock);
						(*pidct)(blocks[k*block_count+n], engine->temp);
						(*padd_pred)(pred[cc]+offs,cur[cc]+offs,lx,blocks[k*block_count+n]);
//pthread_mutex_unlock(&test_lock);
					}

					k++;
				}
		}
		pthread_mutex_unlock(&(engine->output_lock));
	}
}

void itransform(pict_data_s *picture,
	uint8_t *pred[], uint8_t *cur[])
{
	int i;
/* Start loop */
	for(i = 0; i < processors; i++)
	{
		itransform_engines[i].picture = picture;
		itransform_engines[i].cur = cur;
		itransform_engines[i].pred = pred;
		pthread_mutex_unlock(&(itransform_engines[i].input_lock));
	}

/* Wait for completion */
	for(i = 0; i < processors; i++)
	{
		pthread_mutex_lock(&(itransform_engines[i].output_lock));
	}
}

void start_itransform_engines()
{
	int i;
	int rows_per_processor = (int)((float)height2 / 16 / processors + 0.5);
	int current_row = 0;
	pthread_attr_t  attr;
	pthread_mutexattr_t mutex_attr;

	pthread_mutexattr_init(&mutex_attr);
	pthread_attr_init(&attr);
	itransform_engines = calloc(1, sizeof(transform_engine_t) * processors);
	for(i = 0; i < processors; i++)
	{
		itransform_engines[i].start_row = current_row * 16;
		current_row += rows_per_processor;
		if(current_row > height2 / 16) current_row = height2 / 16;
		itransform_engines[i].end_row = current_row * 16;
		pthread_mutex_init(&(itransform_engines[i].input_lock), &mutex_attr);
		pthread_mutex_lock(&(itransform_engines[i].input_lock));
		pthread_mutex_init(&(itransform_engines[i].output_lock), &mutex_attr);
		pthread_mutex_lock(&(itransform_engines[i].output_lock));
		itransform_engines[i].done = 0;
		pthread_create(&(itransform_engines[i].tid), 
			&attr, 
			(void*)itransform_engine_loop, 
			&itransform_engines[i]);
	}
}

void stop_itransform_engines()
{
	int i;
	for(i = 0; i < processors; i++)
	{
		itransform_engines[i].done = 1;
		pthread_mutex_unlock(&(itransform_engines[i].input_lock));
		pthread_join(itransform_engines[i].tid, 0);
		pthread_mutex_destroy(&(itransform_engines[i].input_lock));
		pthread_mutex_destroy(&(itransform_engines[i].output_lock));
	}
	free(itransform_engines);
}




/*
 * select between frame and field DCT
 *
 * preliminary version: based on inter-field correlation
 */

void dct_type_estimation(
	pict_data_s *picture,
	uint8_t *pred, uint8_t *cur
	)
{

	struct mbinfo *mbi = picture->mbinfo;

	int16_t blk0[128], blk1[128];
	int i, j, i0, j0, k, offs, s0, s1, sq0, sq1, s01;
	double d, r;

	k = 0;

	for (j0=0; j0<height2; j0+=16)
		for (i0=0; i0<width; i0+=16)
		{
			if (picture->frame_pred_dct || picture->pict_struct!=FRAME_PICTURE)
				mbi[k].dct_type = 0;
			else
			{
				/* interlaced frame picture */
				/*
				 * calculate prediction error (cur-pred) for top (blk0)
				 * and bottom field (blk1)
				 */
				for (j=0; j<8; j++)
				{
					offs = width*((j<<1)+j0) + i0;
					for (i=0; i<16; i++)
					{
						blk0[16*j+i] = cur[offs] - pred[offs];
						blk1[16*j+i] = cur[offs+width] - pred[offs+width];
						offs++;
					}
				}
				/* correlate fields */
				s0=s1=sq0=sq1=s01=0;

				for (i=0; i<128; i++)
				{
					s0+= blk0[i];
					sq0+= blk0[i]*blk0[i];
					s1+= blk1[i];
					sq1+= blk1[i]*blk1[i];
					s01+= blk0[i]*blk1[i];
				}

				d = (sq0-(s0*s0)/128.0)*(sq1-(s1*s1)/128.0);

				if (d>0.0)
				{
					r = (s01-(s0*s1)/128.0)/sqrt(d);
					if (r>0.5)
						mbi[k].dct_type = 0; /* frame DCT */
					else
						mbi[k].dct_type = 1; /* field DCT */
				}
				else
					mbi[k].dct_type = 1; /* field DCT */
			}
			k++;
		}
}
