/* putpic.c, block and motion vector encoding routines                      */

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

/* output motion vectors (6.2.5.2, 6.3.16.2)
 *
 * this routine also updates the predictions for motion vectors (PMV)
 */
 
static void putmvs(slice_engine_t *engine,
	pict_data_s *picture,
	mbinfo_s *mb,
	int PMV[2][2][2],
	int back)
{
	int hor_f_code;
	int vert_f_code;

	if( back )
	{
		hor_f_code = picture->back_hor_f_code;
		vert_f_code = picture->back_vert_f_code;
	}
	else
	{
		hor_f_code = picture->forw_hor_f_code;
		vert_f_code = picture->forw_vert_f_code;
	}


	if(picture->pict_struct == FRAME_PICTURE)
	{
    	if(mb->motion_type == MC_FRAME)
    	{
/* frame prediction */
    		putmv(engine, mb->MV[0][back][0] - PMV[0][back][0], hor_f_code);
    		putmv(engine, mb->MV[0][back][1] - PMV[0][back][1], vert_f_code);
    		PMV[0][back][0] = PMV[1][back][0] = mb->MV[0][back][0];
    		PMV[0][back][1] = PMV[1][back][1] = mb->MV[0][back][1];
    	}
    	else 
		if (mb->motion_type == MC_FIELD)
    	{
    		/* field prediction */
    		slice_putbits(engine, mb->mv_field_sel[0][back], 1);
    		putmv(engine, mb->MV[0][back][0] - PMV[0][back][0], hor_f_code);
    		putmv(engine, (mb->MV[0][back][1] >> 1) - (PMV[0][back][1] >> 1), vert_f_code);
    		slice_putbits(engine, mb->mv_field_sel[1][back], 1);
    		putmv(engine, mb->MV[1][back][0] - PMV[1][back][0], hor_f_code);
    		putmv(engine, (mb->MV[1][back][1] >> 1) - (PMV[1][back][1] >> 1), vert_f_code);
    		PMV[0][back][0] = mb->MV[0][back][0];
    		PMV[0][back][1] = mb->MV[0][back][1];
    		PMV[1][back][0] = mb->MV[1][back][0];
    		PMV[1][back][1] = mb->MV[1][back][1];
    	}
    	else
    	{
    		/* dual prime prediction */
    		putmv(engine, mb->MV[0][back][0] - PMV[0][back][0], hor_f_code);
    		putdmv(engine, mb->dmvector[0]);
    		putmv(engine, (mb->MV[0][back][1] >> 1) - (PMV[0][back][1] >> 1), vert_f_code);
    		putdmv(engine, mb->dmvector[1]);
    		PMV[0][back][0] = PMV[1][back][0] = mb->MV[0][back][0];
    		PMV[0][back][1] = PMV[1][back][1] = mb->MV[0][back][1];
    	}
	}
	else
	{
    	/* field picture */
    	if(mb->motion_type == MC_FIELD)
    	{
    		/* field prediction */
    		slice_putbits(engine, mb->mv_field_sel[0][back], 1);
    		putmv(engine, mb->MV[0][back][0] - PMV[0][back][0], hor_f_code);
    		putmv(engine, mb->MV[0][back][1] - PMV[0][back][1], vert_f_code);
    		PMV[0][back][0] = PMV[1][back][0] = mb->MV[0][back][0];
    		PMV[0][back][1] = PMV[1][back][1] = mb->MV[0][back][1];
    	}
    	else 
		if(mb->motion_type == MC_16X8)
    	{
    		/* 16x8 prediction */
    		slice_putbits(engine, mb->mv_field_sel[0][back], 1);
    		putmv(engine, mb->MV[0][back][0] - PMV[0][back][0], hor_f_code);
    		putmv(engine, mb->MV[0][back][1] - PMV[0][back][1], vert_f_code);
    		slice_putbits(engine, mb->mv_field_sel[1][back], 1);
    		putmv(engine, mb->MV[1][back][0] - PMV[1][back][0], hor_f_code);
    		putmv(engine, mb->MV[1][back][1] - PMV[1][back][1], vert_f_code);
    		PMV[0][back][0] = mb->MV[0][back][0];
    		PMV[0][back][1] = mb->MV[0][back][1];
    		PMV[1][back][0] = mb->MV[1][back][0];
    		PMV[1][back][1] = mb->MV[1][back][1];
    	}
    	else
    	{
    		/* dual prime prediction */
    		putmv(engine, mb->MV[0][back][0] - PMV[0][back][0], hor_f_code);
    		putdmv(engine, mb->dmvector[0]);
    		putmv(engine, mb->MV[0][back][1] - PMV[0][back][1], vert_f_code);
    		putdmv(engine, mb->dmvector[1]);
    		PMV[0][back][0] = PMV[1][back][0] = mb->MV[0][back][0];
    		PMV[0][back][1] = PMV[1][back][1] = mb->MV[0][back][1];
    	}
	}
}


void* slice_engine_loop(slice_engine_t *engine)
{
	while(!engine->done)
	{
		pthread_mutex_lock(&(engine->input_lock));

		if(!engine->done)
		{
			pict_data_s *picture = engine->picture;
			int i, j, k, comp, cc;
			int mb_type;
			int PMV[2][2][2];
			int cbp, MBAinc = 0;
			short (*quant_blocks)[64] = picture->qblocks;

			k = engine->start_row * mb_width;
			for(j = engine->start_row; j < engine->end_row; j++)
			{
/* macroblock row loop */
//printf("putpic 1\n");
//slice_testbits(engine);
    			for(i = 0; i < mb_width; i++)
    			{
					mbinfo_s *cur_mb = &picture->mbinfo[k];
					int cur_mb_blocks = k * block_count;
//pthread_mutex_lock(&test_lock);
/* macroblock loop */
    				if(i == 0)
    				{
						slice_alignbits(engine);
/* slice header (6.2.4) */
        				if(mpeg1 || vertical_size <= 2800)
        					slice_putbits(engine, SLICE_MIN_START + j, 32); /* slice_start_code */
        				else
        				{
        					slice_putbits(engine, SLICE_MIN_START + (j & 127), 32); /* slice_start_code */
        					slice_putbits(engine, j >> 7, 3); /* slice_vertical_position_extension */
        				}
/* quantiser_scale_code */
//printf("putpic 1\n");slice_testbits(engine);
        				slice_putbits(engine, 
							picture->q_scale_type ? map_non_linear_mquant_hv[engine->prev_mquant] : engine->prev_mquant >> 1, 
							5);

        				slice_putbits(engine, 0, 1); /* extra_bit_slice */

//printf("putpic 1 %d %d %d\n",engine->prev_mquant, map_non_linear_mquant_hv[engine->prev_mquant], engine->prev_mquant >> 1);
//slice_testbits(engine);
        				/* reset predictors */

        				for(cc = 0; cc < 3; cc++)
        					engine->dc_dct_pred[cc] = 0;

        				PMV[0][0][0] = PMV[0][0][1] = PMV[1][0][0] = PMV[1][0][1] = 0;
        				PMV[0][1][0] = PMV[0][1][1] = PMV[1][1][0] = PMV[1][1][1] = 0;

        				MBAinc = i + 1; /* first MBAinc denotes absolute position */
    				}

    				mb_type = cur_mb->mb_type;

/* determine mquant (rate control) */
    				cur_mb->mquant = ratectl_calc_mquant(engine->ratectl, picture, k);

/* quantize macroblock */
//printf("putpic 1\n");
//printf("putpic 1\n");
//slice_testbits(engine);
    				if(mb_type & MB_INTRA)
    				{
//printf("putpic 2 %d\n", cur_mb->mquant);
						quant_intra_hv( picture,
					        		 picture->blocks[cur_mb_blocks],
									 quant_blocks[cur_mb_blocks],
									 cur_mb->mquant, 
									 &cur_mb->mquant );

//printf("putpic 3\n");
						cur_mb->cbp = cbp = (1<<block_count) - 1;
    				}
    				else
    				{
//printf("putpic 4 %p %d\n", picture->blocks[cur_mb_blocks], cur_mb_blocks);
						cbp = (*pquant_non_intra)(picture,
											  picture->blocks[cur_mb_blocks],
											  quant_blocks[cur_mb_blocks],
											  cur_mb->mquant,
											  &cur_mb->mquant);
//printf("putpic 5\n");
						cur_mb->cbp = cbp;
						if (cbp)
							mb_type|= MB_PATTERN;
    				}
//printf("putpic 6\n");
//printf("putpic 2\n");
//slice_testbits(engine);

/* output mquant if it has changed */
    				if(cbp && engine->prev_mquant != cur_mb->mquant)
        				mb_type |= MB_QUANT;

/* check if macroblock can be skipped */
    				if(i != 0 && i != mb_width - 1 && !cbp)
    				{
/* no DCT coefficients and neither first nor last macroblock of slice */

        				if(picture->pict_type == P_TYPE && 
							!(mb_type & MB_FORWARD))
        				{
        					/* P picture, no motion vectors -> skip */

        					/* reset predictors */

        					for(cc = 0; cc < 3; cc++)
            					 engine->dc_dct_pred[cc] = 0;

        					PMV[0][0][0] = PMV[0][0][1] = PMV[1][0][0] = PMV[1][0][1] = 0;
        					PMV[0][1][0] = PMV[0][1][1] = PMV[1][1][0] = PMV[1][1][1] = 0;

        					cur_mb->mb_type = mb_type;
        					cur_mb->skipped = 1;
        					MBAinc++;
        					k++;
        					continue;
        				}

        				if(picture->pict_type == B_TYPE && 
							picture->pict_struct == FRAME_PICTURE &&
            				cur_mb->motion_type == MC_FRAME && 
            				((picture->mbinfo[k - 1].mb_type ^ mb_type) & (MB_FORWARD | MB_BACKWARD)) == 0 &&
            				(!(mb_type & MB_FORWARD) ||
                				(PMV[0][0][0] == cur_mb->MV[0][0][0] &&
                				 PMV[0][0][1] == cur_mb->MV[0][0][1])) && 
            				(!(mb_type & MB_BACKWARD) ||
                				(PMV[0][1][0] == cur_mb->MV[0][1][0] &&
                				 PMV[0][1][1] == cur_mb->MV[0][1][1])))
        				{
/* conditions for skipping in B frame pictures:
 * - must be frame predicted
 * - must be the same prediction type (forward/backward/interp.)
 *   as previous macroblock
 * - relevant vectors (forward/backward/both) have to be the same
 *   as in previous macroblock
 */

        					cur_mb->mb_type = mb_type;
        					cur_mb->skipped = 1;
        					MBAinc++;
        					k++;
        					continue;
        				}

        				if (picture->pict_type == B_TYPE && 
							picture->pict_struct != FRAME_PICTURE && 
            				cur_mb->motion_type == MC_FIELD && 
            				((picture->mbinfo[k - 1].mb_type ^ mb_type) & (MB_FORWARD | MB_BACKWARD))==0 && 
            				(!(mb_type & MB_FORWARD) ||
                				(PMV[0][0][0] == cur_mb->MV[0][0][0] &&
                				 PMV[0][0][1] == cur_mb->MV[0][0][1] &&
                				 cur_mb->mv_field_sel[0][0] == (picture->pict_struct == BOTTOM_FIELD))) && 
            				(!(mb_type & MB_BACKWARD) ||
                				(PMV[0][1][0] == cur_mb->MV[0][1][0] &&
                				 PMV[0][1][1] == cur_mb->MV[0][1][1] &&
                				 cur_mb->mv_field_sel[0][1] == (picture->pict_struct == BOTTOM_FIELD))))
        				{
/* conditions for skipping in B field pictures:
 * - must be field predicted
 * - must be the same prediction type (forward/backward/interp.)
 *   as previous macroblock
 * - relevant vectors (forward/backward/both) have to be the same
 *   as in previous macroblock
 * - relevant motion_vertical_field_selects have to be of same
 *   parity as current field
 */

        					cur_mb->mb_type = mb_type;
        					cur_mb->skipped = 1;
        					MBAinc++;
        					k++;
        					continue;
        				}
    				}
//printf("putpic 3\n");
//slice_testbits(engine);

/* macroblock cannot be skipped */
    				cur_mb->skipped = 0;

/* there's no VLC for 'No MC, Not Coded':
 * we have to transmit (0,0) motion vectors
 */
    				if(picture->pict_type == P_TYPE && 
						!cbp && 
						!(mb_type & MB_FORWARD))
        				mb_type |= MB_FORWARD;

/* macroblock_address_increment */
    				putaddrinc(engine, MBAinc); 
    				MBAinc = 1;
    				putmbtype(engine, picture->pict_type, mb_type); /* macroblock type */
//printf("putpic 3\n");
//slice_testbits(engine);

    				if(mb_type & (MB_FORWARD | MB_BACKWARD) && 
						!picture->frame_pred_dct)
        				slice_putbits(engine, cur_mb->motion_type, 2);

//printf("putpic %x %d %d %d\n", mb_type, picture->pict_struct, cbp, picture->frame_pred_dct);
    				if(picture->pict_struct == FRAME_PICTURE && 
						cbp && 
						!picture->frame_pred_dct)
        				slice_putbits(engine, cur_mb->dct_type, 1);

    				if(mb_type & MB_QUANT)
    				{
        				slice_putbits(engine, 
							picture->q_scale_type ? map_non_linear_mquant_hv[cur_mb->mquant] : cur_mb->mquant >> 1,
							5);
        				engine->prev_mquant = cur_mb->mquant;
    				}

    				if(mb_type & MB_FORWARD)
    				{
/* forward motion vectors, update predictors */
        				putmvs(engine, picture, cur_mb, PMV, 0);
    				}

    				if(mb_type & MB_BACKWARD)
    				{
/* backward motion vectors, update predictors */
        				putmvs(engine, picture,  cur_mb, PMV, 1);
    				}

    				if(mb_type & MB_PATTERN)
    				{
        				putcbp(engine, (cbp >> (block_count - 6)) & 63);
        				if(chroma_format != CHROMA420)
        					slice_putbits(engine, cbp, block_count - 6);
    				}

//printf("putpic 4\n");
//slice_testbits(engine);

    				for(comp = 0; comp < block_count; comp++)
    				{
/* block loop */
        				if(cbp & (1 << (block_count - 1 - comp)))
        				{
        				  if(mb_type & MB_INTRA)
        				  {
            				  cc = (comp < 4) ? 0 : (comp & 1) + 1;
            				  putintrablk(engine, picture, quant_blocks[cur_mb_blocks + comp], cc);

        				  }
        				  else
            				  putnonintrablk(engine, picture, quant_blocks[cur_mb_blocks + comp]);
        				}
    				}
//printf("putpic 5\n");
//slice_testbits(engine);
//sleep(1);

    				/* reset predictors */
    				if(!(mb_type & MB_INTRA))
        				for(cc = 0; cc < 3; cc++)
        					engine->dc_dct_pred[cc] = 0;

    				if(mb_type & MB_INTRA || 
						(picture->pict_type == P_TYPE && !(mb_type & MB_FORWARD)))
    				{
        				PMV[0][0][0] = PMV[0][0][1] = PMV[1][0][0] = PMV[1][0][1] = 0;
        				PMV[0][1][0] = PMV[0][1][1] = PMV[1][1][0] = PMV[1][1][1] = 0;
    				}

    				cur_mb->mb_type = mb_type;
    				k++;
//pthread_mutex_unlock(&test_lock);
    			}
			}
		}
		pthread_mutex_unlock(&(engine->output_lock));
	}
}


/* quantization / variable length encoding of a complete picture */
void putpict(pict_data_s *picture)
{
	int i, prev_mquant;

	for(i = 0; i < processors; i++)
	{
		ratectl_init_pict(ratectl[i], picture); /* set up rate control */
	}

/* picture header and picture coding extension */
	putpicthdr(picture);
	if(!mpeg1) putpictcodext(picture);
/* Flush buffer before switching to slice mode */
	alignbits();

/* Start loop */
	for(i = 0; i < processors; i++)
	{
		slice_engines[i].prev_mquant = ratectl_start_mb(ratectl[i], picture);
		slice_engines[i].picture = picture;
		slice_engines[i].ratectl = ratectl[i];
		slice_initbits(&slice_engines[i]);

		pthread_mutex_unlock(&(slice_engines[i].input_lock));
	}

/* Wait for completion and write slices */
	for(i = 0; i < processors; i++)
	{
		pthread_mutex_lock(&(slice_engines[i].output_lock));
		slice_finishslice(&slice_engines[i]);
	}

	for(i = 0; i < processors; i++)
		ratectl_update_pict(ratectl[i], picture);
}

void start_slice_engines()
{
	int i;
	int rows_per_processor = (int)((float)mb_height2 / processors + 0.5);
	int current_row = 0;
	pthread_attr_t attr;
	pthread_mutexattr_t mutex_attr;

	pthread_mutexattr_init(&mutex_attr);
	pthread_attr_init(&attr);
//	pthread_mutex_init(&test_lock, &mutex_attr);

	slice_engines = calloc(1, sizeof(slice_engine_t) * processors);
	for(i = 0; i < processors; i++)
	{
		slice_engines[i].start_row = current_row;
		current_row += rows_per_processor;
		if(current_row > mb_height2) current_row = mb_height2;
		slice_engines[i].end_row = current_row;
		
		pthread_mutex_init(&(slice_engines[i].input_lock), &mutex_attr);
		pthread_mutex_lock(&(slice_engines[i].input_lock));
		pthread_mutex_init(&(slice_engines[i].output_lock), &mutex_attr);
		pthread_mutex_lock(&(slice_engines[i].output_lock));
		slice_engines[i].done = 0;
		pthread_create(&(slice_engines[i].tid), 
			&attr, 
			(void*)slice_engine_loop, 
			&slice_engines[i]);
	}
}

void stop_slice_engines()
{
	int i;
	for(i = 0; i < processors; i++)
	{
		slice_engines[i].done = 1;
		pthread_mutex_unlock(&(slice_engines[i].input_lock));
		pthread_join(slice_engines[i].tid, 0);
		pthread_mutex_destroy(&(slice_engines[i].input_lock));
		pthread_mutex_destroy(&(slice_engines[i].output_lock));
		if(slice_engines[i].slice_buffer) free(slice_engines[i].slice_buffer);
	}
	free(slice_engines);
//	pthread_mutex_destroy(&test_lock);
}
