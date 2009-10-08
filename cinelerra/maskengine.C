
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "bcsignals.h"
#include "condition.h"
#include "clip.h"
#include "maskauto.h"
#include "maskautos.h"
#include "maskengine.h"
#include "mutex.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "feather.h"


int64_t get_difference(struct timeval *start_time)
{
        struct timeval new_time;

	gettimeofday(&new_time, 0);

	new_time.tv_usec -= start_time->tv_usec;
	new_time.tv_sec -= start_time->tv_sec;
	if(new_time.tv_usec < 0)
	{
		new_time.tv_usec += 1000000;
		new_time.tv_sec--;
	}

	return (int64_t)new_time.tv_sec * 1000000 + 
		(int64_t)new_time.tv_usec;

}



MaskPackage::MaskPackage()
{
}

MaskPackage::~MaskPackage()
{
}





MaskUnit::MaskUnit(MaskEngine *engine)
 : LoadClient(engine)
{
	this->engine = engine;
	row_spans_h = 0;
	row_spans = 0;
}


MaskUnit::~MaskUnit()
{
	if (row_spans)
	{
		for (int i = 0; i < row_spans_h; i++) 
			free(row_spans[i]);
		delete [] row_spans;
	}
}

#ifndef SQR
#define SQR(x) ((x) * (x))
#endif
















inline void MaskUnit::draw_line_clamped(
	int draw_x1, 
	int draw_y1, 
	int draw_x2, 
	int draw_y2,
	int w,
	int h,
	int hoffset)
{
//printf("MaskUnit::draw_line_clamped 1 %d %d %d %d\n", x1, y1, x2, y2);
	if (draw_y1 == draw_y2) return; 

	if(draw_y2 < draw_y1)
	{ /* change the order */
		int tmp;
		tmp = draw_x1;
		draw_x1 = draw_x2;
		draw_x2 = tmp;
		tmp = draw_y1;
		draw_y1 = draw_y2;
		draw_y2 = tmp;
	}

	float slope = ((float)draw_x2 - draw_x1) / ((float)draw_y2 - draw_y1); 
	w--;
	for(int y_i = draw_y1; y_i < draw_y2; y_i++) 
	{ 
		if (y_i >= h) 
			return; // since y gets larger, there is no point in continuing
		else if(y_i >= 0) 
		{ 
			int x = (int)(slope * (y_i - draw_y1) + draw_x1); 
			int x_i = CLIP(x, 0, w); 

			/* now insert into span in order */
			short *span = row_spans[y_i + hoffset];	
			if (span[0] >= span[1]) { /* do the reallocation */
				span[1] *= 2;
				span = row_spans[y_i + hoffset] = (short *) realloc (span, span[1] * sizeof(short)); /* be careful! row_spans has to be updated! */
			};

			short index = 2;
			while (index < span[0]  && span[index] < x_i)
				index++;
			for (int j = span[0]; j > index; j--) {       // move forward
				span[j] = span[j-1];
			}
			span[index] = x_i;
			span[0] ++;
		} 
	} 
}

template<class T>
void MaskUnit::blur_strip(float *val_p, 
	float *val_m, 
	float *dst, 
	float *src, 
	int size,
	T max)
{
	float *sp_p = src;
	float *sp_m = src + size - 1;
	float *vp = val_p;
	float *vm = val_m + size - 1;
	float initial_p = sp_p[0];
	float initial_m = sp_m[0];

//printf("MaskUnit::blur_strip %d\n", size);
	for(int k = 0; k < size; k++)
	{
		int terms = (k < 4) ? k : 4;
		int l;
		for(l = 0; l <= terms; l++)
		{
			*vp += n_p[l] * sp_p[-l] - d_p[l] * vp[-l];
			*vm += n_m[l] * sp_m[l] - d_m[l] * vm[l];
		}

		for( ; l <= 4; l++)
		{
			*vp += (n_p[l] - bd_p[l]) * initial_p;
			*vm += (n_m[l] - bd_m[l]) * initial_m;
		}
		sp_p++;
		sp_m--;
		vp++;
		vm--;
	}

	for(int i = 0; i < size; i++)
	{
		float sum = val_p[i] + val_m[i];
		CLAMP(sum, 0, max);
		dst[i] = sum;
	}
}



int MaskUnit::do_feather_2(VFrame *output,
	VFrame *input, 
	float feather, 
	int start_out, 
	int end_out)
{
	
	int fint = (int)feather;
	DO_FEATHER_N(unsigned char, uint32_t, 0xffff, fint);

}


void MaskUnit::do_feather(VFrame *output,
	VFrame *input, 
	float feather, 
	int start_out, 
	int end_out)
{
//printf("MaskUnit::do_feather %f\n", feather);
// Get constants
	double constants[8];
	double div;
	double std_dev = sqrt(-(double)(feather * feather) / (2 * log(1.0 / 255.0)));
	div = sqrt(2 * M_PI) * std_dev;
	constants[0] = -1.783 / std_dev;
	constants[1] = -1.723 / std_dev;
	constants[2] = 0.6318 / std_dev;
	constants[3] = 1.997  / std_dev;
	constants[4] = 1.6803 / div;
	constants[5] = 3.735 / div;
	constants[6] = -0.6803 / div;
	constants[7] = -0.2598 / div;

	n_p[0] = constants[4] + constants[6];
	n_p[1] = exp(constants[1]) *
				(constants[7] * sin(constants[3]) -
				(constants[6] + 2 * constants[4]) * cos(constants[3])) +
				exp(constants[0]) *
				(constants[5] * sin(constants[2]) -
				(2 * constants[6] + constants[4]) * cos(constants[2]));

	n_p[2] = 2 * exp(constants[0] + constants[1]) *
				((constants[4] + constants[6]) * cos(constants[3]) * 
				cos(constants[2]) - constants[5] * 
				cos(constants[3]) * sin(constants[2]) -
				constants[7] * cos(constants[2]) * sin(constants[3])) +
				constants[6] * exp(2 * constants[0]) +
				constants[4] * exp(2 * constants[1]);

	n_p[3] = exp(constants[1] + 2 * constants[0]) *
				(constants[7] * sin(constants[3]) - 
				constants[6] * cos(constants[3])) +
				exp(constants[0] + 2 * constants[1]) *
				(constants[5] * sin(constants[2]) - constants[4] * 
				cos(constants[2]));
	n_p[4] = 0.0;

	d_p[0] = 0.0;
	d_p[1] = -2 * exp(constants[1]) * cos(constants[3]) -
				2 * exp(constants[0]) * cos(constants[2]);

	d_p[2] = 4 * cos(constants[3]) * cos(constants[2]) * 
				exp(constants[0] + constants[1]) +
				exp(2 * constants[1]) + exp (2 * constants[0]);

	d_p[3] = -2 * cos(constants[2]) * exp(constants[0] + 2 * constants[1]) -
				2 * cos(constants[3]) * exp(constants[1] + 2 * constants[0]);

	d_p[4] = exp(2 * constants[0] + 2 * constants[1]);

	for(int i = 0; i < 5; i++) d_m[i] = d_p[i];

	n_m[0] = 0.0;
	for(int i = 1; i <= 4; i++)
		n_m[i] = n_p[i] - d_p[i] * n_p[0];

	double sum_n_p, sum_n_m, sum_d;
	double a, b;

	sum_n_p = 0.0;
	sum_n_m = 0.0;
	sum_d = 0.0;
	for(int i = 0; i < 5; i++)
	{
		sum_n_p += n_p[i];
		sum_n_m += n_m[i];
		sum_d += d_p[i];
	}

	a = sum_n_p / (1 + sum_d);
	b = sum_n_m / (1 + sum_d);

	for(int i = 0; i < 5; i++)
	{
		bd_p[i] = d_p[i] * a;
		bd_m[i] = d_m[i] * b;
	}






















#define DO_FEATHER(type, max) \
{ \
	int frame_w = input->get_w(); \
	int frame_h = input->get_h(); \
	int size = MAX(frame_w, frame_h); \
	float *src = new float[size]; \
	float *dst = new float[size]; \
	float *val_p = new float[size]; \
	float *val_m = new float[size]; \
	int start_in = start_out - (int)feather; \
	int end_in = end_out + (int)feather; \
	if(start_in < 0) start_in = 0; \
	if(end_in > frame_h) end_in = frame_h; \
	int strip_size = end_in - start_in; \
	type **in_rows = (type**)input->get_rows(); \
	type **out_rows = (type**)output->get_rows(); \
	int j; \
 \
/* printf("DO_FEATHER 1\n"); */ \
	for(j = 0; j < frame_w; j++) \
	{ \
/* printf("DO_FEATHER 1.1 %d\n", j); */ \
		bzero(val_p, sizeof(float) * (end_in - start_in)); \
		bzero(val_m, sizeof(float) * (end_in - start_in)); \
		for(int l = 0, k = start_in; k < end_in; l++, k++) \
		{ \
			src[l] = (float)in_rows[k][j]; \
		} \
 \
		blur_strip(val_p, val_m, dst, src, strip_size, max); \
 \
		for(int l = start_out - start_in, k = start_out; k < end_out; l++, k++) \
		{ \
			out_rows[k][j] = (type)dst[l]; \
		} \
	} \
 \
	for(j = start_out; j < end_out; j++) \
	{ \
/* printf("DO_FEATHER 2 %d\n", j); */ \
		bzero(val_p, sizeof(float) * frame_w); \
		bzero(val_m, sizeof(float) * frame_w); \
		for(int k = 0; k < frame_w; k++) \
		{ \
			src[k] = (float)out_rows[j][k]; \
		} \
 \
		blur_strip(val_p, val_m, dst, src, frame_w, max); \
 \
		for(int k = 0; k < frame_w; k++) \
		{ \
			out_rows[j][k] = (type)dst[k]; \
		} \
	} \
 \
/* printf("DO_FEATHER 3\n"); */ \
 \
	delete [] src; \
	delete [] dst; \
	delete [] val_p; \
	delete [] val_m; \
/* printf("DO_FEATHER 4\n"); */ \
}








//printf("do_feather %d\n", frame->get_color_model());
	switch(input->get_color_model())
	{
		case BC_A8:
			DO_FEATHER(unsigned char, 0xff);
			break;
		
		case BC_A16:
			DO_FEATHER(uint16_t, 0xffff);
			break;
		
		case BC_A_FLOAT:
			DO_FEATHER(float, 1.0f);
			break;
	}




}

void MaskUnit::process_package(LoadPackage *package)
{
	MaskPackage *ptr = (MaskPackage*)package;

	int start_row = SHRT_MIN;         // part for which mask exists
	int end_row;
	if(engine->recalculate)
	{
		VFrame *mask;
		if(engine->feather > 0) 
			mask = engine->temp_mask;
		else
			mask = engine->mask;

SET_TRACE
// Generated oversampling frame
		int mask_w = mask->get_w();
		int mask_h = mask->get_h();
		int mask_color_model = mask->get_color_model();
		int oversampled_package_w = mask_w * OVERSAMPLE;
		int oversampled_package_h = (ptr->row2 - ptr->row1) * OVERSAMPLE;
//printf("MaskUnit::process_package 1\n");

SET_TRACE

		int local_first_nonempty_rowspan = SHRT_MIN;
		int local_last_nonempty_rowspan = SHRT_MIN;

		if (!row_spans || row_spans_h != mask_h * OVERSAMPLE) {
			int i;	
			if (row_spans) {   /* size change */
				for (i = 0; i < row_spans_h; i++) 
					free(row_spans[i]);
				delete [] row_spans;
			}
			row_spans_h = mask_h * OVERSAMPLE;
			row_spans = new short *[mask_h * OVERSAMPLE]; 
			for (i= 0; i<mask_h * OVERSAMPLE; i++) {
				/* we use malloc so we can use realloc */
				row_spans[i] = (short *)malloc(sizeof(short) * NUM_SPANS);
				/* [0] is initialized later */
				row_spans[i][1] = NUM_SPANS;
			}
		}

SET_TRACE
//printf("MaskUnit::process_package 1 %d\n", engine->point_sets.total);

SET_TRACE

// Draw bezier curves onto span buffer
//struct timeval start_time;
//gettimeofday(&start_time, 0);

		for(int k = 0; k < engine->point_sets.total; k++)
		{
			int old_x, old_y;
			old_x = SHRT_MIN; // sentinel
			ArrayList<MaskPoint*> *points = engine->point_sets.values[k];

			if(points->total < 2) continue;
//printf("MaskUnit::process_package 2 %d %d\n", k, points->total);
			for (int i = ptr->row1 * OVERSAMPLE; i < ptr->row2 * OVERSAMPLE; i++) 
				row_spans[i][0] = 2; /* initialize to zero */ 
			(ptr->row1*OVERSAMPLE, ptr->row2*OVERSAMPLE); // init just my rows
			for(int i = 0; i < points->total; i++)
			{
				MaskPoint *point1 = points->values[i];
				MaskPoint *point2 = (i >= points->total - 1) ? 
					points->values[0] : 
					points->values[i + 1];

				float x0 = point1->x;
				float y0 = point1->y;
				float x1 = point1->x + point1->control_x2;
				float y1 = point1->y + point1->control_y2;
				float x2 = point2->x + point2->control_x1;
				float y2 = point2->y + point2->control_y1;
				float x3 = point2->x;
				float y3 = point2->y;

				// possible optimization here... since these coordinates are bounding box for curve
				// we can continue with next curve if they are out of our range

				// forward differencing bezier curves implementation taken from GPL code at
				// http://cvs.sourceforge.net/viewcvs.py/guliverkli/guliverkli/src/subtitles/Rasterizer.cpp?rev=1.3



				float cx3, cx2, cx1, cx0, cy3, cy2, cy1, cy0;


				// [-1 +3 -3 +1]
				// [+3 -6 +3  0]
				// [-3 +3  0  0]
				// [+1  0  0  0]

		 		cx3 = (-  x0 + 3*x1 - 3*x2 + x3) * OVERSAMPLE;
				cx2 = ( 3*x0 - 6*x1 + 3*x2) * OVERSAMPLE;
				cx1 = (-3*x0 + 3*x1) * OVERSAMPLE;
				cx0 = (   x0) * OVERSAMPLE;

				cy3 = (-  y0 + 3*y1 - 3*y2 + y3) * OVERSAMPLE;
				cy2 = ( 3*y0 - 6*y1 + 3*y2) * OVERSAMPLE;
				cy1 = (-3*y0 + 3*y1) * OVERSAMPLE;
				cy0 = (   y0 - ptr->row1) * OVERSAMPLE;

				float maxaccel1 = fabs(2*cy2) + fabs(6*cy3);
				float maxaccel2 = fabs(2*cx2) + fabs(6*cx3);

				float maxaccel = maxaccel1 > maxaccel2 ? maxaccel1 : maxaccel2;
				float h = 1.0;

				if(maxaccel > 8.0 * OVERSAMPLE) h = sqrt((8.0 * OVERSAMPLE) / maxaccel);

				for(float t = 0.0; t < 1.0; t += h)
				{
					int x = (int) (cx0 + t*(cx1 + t*(cx2 + t*cx3)));
					int y = (int) (cy0 + t*(cy1 + t*(cy2 + t*cy3)));

					if (old_x != SHRT_MIN) 
						draw_line_clamped(old_x, old_y, x, y, oversampled_package_w, oversampled_package_h, ptr->row1 * OVERSAMPLE);
					old_x = x;
					old_y = y;
				}

				int x = (int)(x3 * OVERSAMPLE);
				int y = (int)((y3 - ptr->row1) * OVERSAMPLE);
				draw_line_clamped(old_x, old_y, x, y, oversampled_package_w, oversampled_package_h, ptr->row1 * OVERSAMPLE);
				old_x = (int)x;
				old_y = (int)y;
		
			}
//printf("MaskUnit::process_package 1\n");

			// Now we have ordered spans ready!
			//printf("Segment : %i , row1: %i\n", oversampled_package_h, ptr->row1);
			uint16_t value;
			if (mask_color_model == BC_A8)
				value = (int)((float)engine->value / 100 * 0xff);
			else
				value = (int)((float)engine->value / 100 * 0xffff);	// also for BC_A_FLOAT

			/* Scaneline sampling, inspired by Graphics gems I, page 81 */
			for (int i = ptr->row1; i < ptr->row2; i++) 
			{
				short min_x = SHRT_MAX;
				short max_x = SHRT_MIN;
				int j; 				/* universal counter for 0..OVERSAMPLE-1 */
				short *span;			/* current span - set inside loops with j */
				short span_p[OVERSAMPLE];	/* pointers to current positions in spans */
				#define P (span_p[j])		/* current span pointer */
				#define MAXP (span[0])		/* current span length */
				int num_empty_spans = 0;
				/* get the initial span pointers ready */
				for (j = 0; j < OVERSAMPLE; j++)
				{	
					span = row_spans[j + i * OVERSAMPLE];
					P = 2;              /* starting pointers to spans */
						/* hypotetical hypotetical fix goes here: take care that there is maximum one empty span for every subpixel */ 
					if (MAXP != 2) {                                        /* if span is not empty */
						if (span[2] < min_x) min_x = span[2];           /* take start of the first span */
						if (span[MAXP-1] > max_x) max_x = span[MAXP-1]; /* and end of last */
					} else              
					{	/* span is empty */
						num_empty_spans ++;	
					}	
				}
				if (num_empty_spans == OVERSAMPLE)
					continue; /* no work for us here */
				else 
				{       /* if we have engaged first nonempty rowspan...	remember it to speed up mask applying */
					if (local_first_nonempty_rowspan < 0 || i < local_first_nonempty_rowspan) 
						local_first_nonempty_rowspan = i;  
					if (i > local_last_nonempty_rowspan) local_last_nonempty_rowspan = i;
				}
				/* we have some pixels to fill, do coverage calculation for span */

				void *output_row = (unsigned char*)mask->get_rows()[i];
				min_x = min_x / OVERSAMPLE;
				max_x = (max_x + OVERSAMPLE - 1) / OVERSAMPLE;
				
				/* printf("row %i, pixel range: %i %i, spans0: %i\n", i, min_x, max_x, row_spans[i*OVERSAMPLE][0]-2); */

				/* this is not a full loop, since we jump trough h if possible */
				for (int h = min_x; h <= max_x; h++) 
				{
					short pixelleft = h * OVERSAMPLE;  /* leftmost subpixel of pixel*/
					short pixelright = pixelleft + OVERSAMPLE - 1; /* rightmost subpixel of pixel */
					uint32_t coverage = 0;
					int num_left = 0;               /* number of spans that have start left of the next pixel */
					short right_end = SHRT_MAX;     /* leftmost end of any span - right end of a full scanline */
					short right_start = SHRT_MAX;   /* leftmost start of any span - left end of empty scanline */

					for (j=0; j< OVERSAMPLE; j++) 
					{	
						char chg = 1;
						span = row_spans[j + i * OVERSAMPLE];
						while (P < MAXP && chg)
						{
						//	printf("Sp: %i %i\n", span[P], span[P+1]);
							if (span[P] == span[P+1])           /* ignore empty spans */
							{
								P +=2;
								continue;
							}
							if (span[P] <= pixelright)          /* if span start is before the end of pixel */
								coverage += MIN(span[P+1], pixelright)  /* 'clip' the span to pixel */
		                                                          - MAX(span[P], pixelleft) + 1;
							if (span[P+1] <= pixelright) 
								P += 2;
							else 
								chg = 0;
						} 
						if (P == MAXP) 
							num_left = -OVERSAMPLE; /* just take care that num_left cannot equal OVERSAMPLE or zero again */
						else	
						{ 
							if (span[P] <= pixelright)  /* if span starts before subpixel in the pixel on the right */
							{    /* useful for determining filled space till next non-fully-filled pixel */
								num_left ++;						
								if (span[P+1] < right_end) right_end = span[P+1]; 
							} else 
							{    /* useful for determining empty space till next non-empty pixel */
								if (span[P] < right_start) right_start = span[P]; 
							}
						}
					}
					// calculate coverage
					coverage *= value;
					coverage /= OVERSAMPLE * OVERSAMPLE;

					// when we have multiple masks the highest coverage wins
					switch (mask_color_model)
					{
					case BC_A8:
						if (((unsigned char *) output_row)[h] < coverage)
							((unsigned char*)output_row)[h] = coverage;
						break;
					case BC_A16:
						if (((uint16_t *) output_row)[h] < coverage)
							((uint16_t *) output_row)[h] = coverage;
						break;
					case BC_A_FLOAT:
						if (((float *) output_row)[h] < coverage/float(0xffff))
							((float *) output_row)[h] = coverage/float(0xffff);
						break;
					}
					/* possible optimization: do joining of multiple masks by span logics, not by bitmap logics*/
					
					if (num_left == OVERSAMPLE) 
					{
						/* all current spans start more left than next pixel */
						/* this means we can probably (if lucky) draw a longer horizontal line */
						right_end = (right_end / OVERSAMPLE) - 1; /* last fully covered pixel */
						if (right_end > h)
						{
							if (mask_color_model == BC_A8) 
								memset((char *)output_row + h + 1, value, right_end - h);
							else {
								/* we are fucked, since there is no 16bit memset */
								if (mask_color_model == BC_A16) {
									for (int z = h +1; z <= right_end; z++)
										((uint16_t *) output_row)[z] =  value;
								} else {
									for (int z = h +1; z <= right_end; z++)
										((float *) output_row)[z] =  value/float(0xffff);
								}
							}
							h = right_end;  
						}
					} else 
					if (num_left == 0) 
					{
						/* all current spans start right of next pixel */ 
						/* this means we can probably (if lucky) skip some pixels */
						right_start = (right_start / OVERSAMPLE) - 1; /* last fully empty pixel */
						if (right_start > h)
						{
							h = right_start;
						}
					}
				}
			}
		}
		engine->protect_data.lock();
		if (local_first_nonempty_rowspan < engine->first_nonempty_rowspan)
			engine->first_nonempty_rowspan = local_first_nonempty_rowspan;
		if (local_last_nonempty_rowspan > engine->last_nonempty_rowspan)
			engine->last_nonempty_rowspan = local_last_nonempty_rowspan;
		engine->protect_data.unlock();
	

//		int64_t dif= get_difference(&start_time);
//		printf("diff: %lli\n", dif);
	}	/* END OF RECALCULATION! */

SET_TRACE

	/* possible optimization: this could be useful for do_feather also */

	// Feather polygon
	if(engine->recalculate && engine->feather > 0) 
	{	
		/* first take care that all packages are already drawn onto mask */
		pthread_mutex_lock(&engine->stage1_finished_mutex);
		engine->stage1_finished_count ++;
		if (engine->stage1_finished_count == engine->get_total_packages())
		{
			// let others pass
			pthread_cond_broadcast(&engine->stage1_finished_cond);
		}
		else
		{
			// wait until all are finished
			while (engine->stage1_finished_count < engine->get_total_packages())
				pthread_cond_wait(&engine->stage1_finished_cond, &engine->stage1_finished_mutex);
		}
		pthread_mutex_unlock(&engine->stage1_finished_mutex);
		
		/* now do the feather */
//printf("MaskUnit::process_package 3 %f\n", engine->feather);

	struct timeval start_time;
	gettimeofday(&start_time, 0);

	/* 
	{
	// EXPERIMENTAL CODE to find out how values between old and new do_feather map
	// create a testcase and find out the closest match between do_feather_2 at 3 and do_feather
	//			2	3	4	5	6	7	8	10	13	15
	// do_feather_2		3	5	7	9	11	13	15	19	25	29
	// do_feather_1		2.683	3.401	4.139	4.768	5.315	5.819	6.271	7.093	8.170	8.844		
	// diff				0.718	0.738	0.629	0.547	0.504	0.452
	// {(2,2.683),(3,3.401),(4,4.139),(5,4.768),(6,5.315),(7,5.819),(8,6.271),(10,7.093),(13,8.170),(15,8.844)}
	// use http://mss.math.vanderbilt.edu/cgi-bin/MSSAgent/~pscrooke/MSS/fitpoly.def
	// for calculating the coefficients

		VFrame *df2 = new VFrame (*engine->mask);
		VFrame *one_sample = new VFrame(*engine->mask);
		do_feather_2(df2, 
			engine->temp_mask, 
			25, 
			ptr->row1, 
			ptr->row2);
		float ftmp;
		for (ftmp = 8.15; ftmp <8.18; ftmp += 0.001) 
		{
			do_feather(one_sample, 
			engine->temp_mask, 
			ftmp, 
			ptr->row1, 
			ptr->row2);
			double squarediff = 0;
			for (int i=0; i< engine->mask->get_h(); i++)
				for (int j = 0; j< engine->mask->get_w(); j++)
				{
					double v1= ((unsigned char *)one_sample->get_rows()[i])[j];
					double v2= ((unsigned char *)df2->get_rows()[i])[j];
					squarediff += (v1-v2)*(v1-v2);
				}
			squarediff = sqrt(squarediff);
			printf("for value 3: ftmp: %2.3f, squarediff: %f\n", ftmp, squarediff);
		}
	}
	*/	
	
		int done = 0;
		done = do_feather_2(engine->mask,        // try if we have super fast implementation ready
				engine->temp_mask,
				engine->feather * 2 - 1, 
				ptr->row1, 
				ptr->row2);
		if (done) {
			engine->realfeather = engine->feather;
		}
		if (!done)
		{
		//	printf("not done\n");
			float feather = engine->feather;
			engine->realfeather = 0.878441 + 0.988534*feather - 0.0490204 *feather*feather  + 0.0012359 *feather*feather*feather;
			do_feather(engine->mask, 
				engine->temp_mask, 
				engine->realfeather, 
				ptr->row1, 
				ptr->row2); 
		}
		int64_t dif= get_difference(&start_time);
		printf("diff: %lli\n", dif);
	} else
	if (engine->feather <= 0) {
		engine->realfeather = 0;
	}
	start_row = MAX (ptr->row1, engine->first_nonempty_rowspan - (int)ceil(engine->realfeather)); 
	end_row = MIN (ptr->row2, engine->last_nonempty_rowspan + 1 + (int)ceil(engine->realfeather));



// Apply mask


/* use the info about first and last column that are coloured from rowspan!  */
/* possible optimisation: also remember total spans */
/* possible optimisation: lookup for  X * (max - *mask_row) / max, where max is known mask_row and X are variabiles */
#define APPLY_MASK_SUBTRACT_ALPHA(type, max, components, do_yuv) \
{ \
	type chroma_offset = (max + 1) / 2; \
	for(int i = start_row; i < end_row; i++) \
	{ \
	type *output_row = (type*)engine->output->get_rows()[i]; \
	type *mask_row = (type*)engine->mask->get_rows()[i]; \
	\
 \
	for(int j  = 0; j < mask_w; j++) \
	{ \
		if(components == 4) \
		{ \
			output_row[3] = output_row[3] * (max - *mask_row) / max; \
		} \
		else \
		{ \
			output_row[0] = output_row[0] * (max - *mask_row) / max; \
 \
			output_row[1] = output_row[1] * (max - *mask_row) / max; \
			output_row[2] = output_row[2] * (max - *mask_row) / max; \
 \
			if(do_yuv) \
			{ \
				output_row[1] += chroma_offset * *mask_row / max; \
				output_row[2] += chroma_offset * *mask_row / max; \
			} \
		} \
		output_row += components; \
		mask_row += 1;		 \
	} \
	} \
}

#define APPLY_MASK_MULTIPLY_ALPHA(type, max, components, do_yuv) \
{ \
	type chroma_offset = (max + 1) / 2; \
		for(int i = ptr->row1; i < ptr->row2; i++) \
		{ \
	type *output_row = (type*)engine->output->get_rows()[i]; \
	type *mask_row = (type*)engine->mask->get_rows()[i]; \
 \
        if (components == 4) output_row += 3; \
	for(int j  = mask_w; j != 0;  j--) \
	{ \
		if(components == 4) \
		{ \
			*output_row = *output_row * *mask_row / max; \
		} \
		else \
		{ \
			output_row[0] = output_row[3] * *mask_row / max; \
 \
			output_row[1] = output_row[1] * *mask_row / max; \
			output_row[2] = output_row[2] * *mask_row / max; \
 \
			if(do_yuv) \
			{ \
				output_row[1] += chroma_offset * (max - *mask_row) / max; \
				output_row[2] += chroma_offset * (max - *mask_row) / max; \
			} \
		} \
		output_row += components; \
		mask_row += 1;		 \
	} \
	} \
}


//struct timeval start_time;
//gettimeofday(&start_time, 0);

//printf("MaskUnit::process_package 1 %d\n", engine->mode);
	int mask_w = engine->mask->get_w();
	switch(engine->mode)
	{
		case MASK_MULTIPLY_ALPHA:
			switch(engine->output->get_color_model())
			{
				case BC_RGB888:
					APPLY_MASK_MULTIPLY_ALPHA(unsigned char, 0xff, 3, 0);
					break;
				case BC_YUV888:
					APPLY_MASK_MULTIPLY_ALPHA(unsigned char, 0xff, 3, 1);
					break;
				case BC_YUVA8888:
				case BC_RGBA8888:
					APPLY_MASK_MULTIPLY_ALPHA(unsigned char, 0xff, 4, 0);
					break;
				case BC_RGB161616:
					APPLY_MASK_MULTIPLY_ALPHA(uint16_t, 0xffff, 3, 0);
					break;
				case BC_YUV161616:
					APPLY_MASK_MULTIPLY_ALPHA(uint16_t, 0xffff, 3, 1);
					break;
				case BC_YUVA16161616:
				case BC_RGBA16161616:
					APPLY_MASK_MULTIPLY_ALPHA(uint16_t, 0xffff, 4, 0);
					break;
				case BC_RGB_FLOAT:
					APPLY_MASK_MULTIPLY_ALPHA(float, 1.0f, 3, 0);
					break;
				case BC_RGBA_FLOAT:
					APPLY_MASK_MULTIPLY_ALPHA(float, 1.0f, 4, 0);
					break;
			}
			break;

		case MASK_SUBTRACT_ALPHA:
			switch(engine->output->get_color_model())
			{
				case BC_RGB888:
					APPLY_MASK_SUBTRACT_ALPHA(unsigned char, 0xff, 3, 0);
					break;
				case BC_YUV888:
					APPLY_MASK_SUBTRACT_ALPHA(unsigned char, 0xff, 3, 1);
					break;
				case BC_YUVA8888:
				case BC_RGBA8888:
					APPLY_MASK_SUBTRACT_ALPHA(unsigned char, 0xff, 4, 0);
					break;
				case BC_RGB161616:
					APPLY_MASK_SUBTRACT_ALPHA(uint16_t, 0xffff, 3, 0);
					break;
				case BC_YUV161616:
					APPLY_MASK_SUBTRACT_ALPHA(uint16_t, 0xffff, 3, 1);
					break;
				case BC_YUVA16161616:
				case BC_RGBA16161616:
					APPLY_MASK_SUBTRACT_ALPHA(uint16_t, 0xffff, 4, 0);
					break;
				case BC_RGB_FLOAT:
					APPLY_MASK_SUBTRACT_ALPHA(float, 1.0f, 3, 0);
					break;
				case BC_RGBA_FLOAT:
					APPLY_MASK_SUBTRACT_ALPHA(float, 1.0f, 4, 0);
					break;
			}
			break;
	}
//	int64_t dif= get_difference(&start_time);
//	printf("diff: %lli\n", dif);
//printf("diff2: %lli\n", get_difference(&start_time));
//printf("MaskUnit::process_package 4 %d\n", get_package_number());
}





MaskEngine::MaskEngine(int cpus)
 : LoadServer(cpus, cpus )      /* these two HAVE to be the same, since packages communicate  */
// : LoadServer(1, 2)
{
	mask = 0;
	pthread_mutex_init(&stage1_finished_mutex, NULL);
	pthread_cond_init(&stage1_finished_cond, NULL);
}

MaskEngine::~MaskEngine()
{
	pthread_cond_destroy(&stage1_finished_cond);
	pthread_mutex_destroy(&stage1_finished_mutex);
	if(mask) 
	{
		delete mask;
		delete temp_mask;
	}

	for(int i = 0; i < point_sets.total; i++)
	{
		ArrayList<MaskPoint*> *points = point_sets.values[i];
		points->remove_all_objects();
	}
	point_sets.remove_all_objects();
}

int MaskEngine::points_equivalent(ArrayList<MaskPoint*> *new_points, 
	ArrayList<MaskPoint*> *points)
{
//printf("MaskEngine::points_equivalent %d %d\n", new_points->total, points->total);
	if(new_points->total != points->total) return 0;
	
	for(int i = 0; i < new_points->total; i++)
	{
		if(!(*new_points->values[i] == *points->values[i])) return 0;
	}
	
	return 1;
}

void MaskEngine::do_mask(VFrame *output, 
	int64_t start_position,
	double frame_rate,
	double project_frame_rate,
	MaskAutos *keyframe_set, 
	int direction,
	int before_plugins)
{
	int64_t start_position_project = (int64_t)(start_position *
		project_frame_rate / 
		frame_rate);
	Auto *current = 0;
	MaskAuto *default_auto = (MaskAuto*)keyframe_set->default_auto;
	MaskAuto *keyframe = (MaskAuto*)keyframe_set->get_prev_auto(start_position_project, 
		direction,
		current);
	
	if (keyframe->apply_before_plugins != before_plugins)
		return;


	int total_points = 0;
	for(int i = 0; i < keyframe->masks.total; i++)
	{
		SubMask *mask = keyframe->get_submask(i);
		int submask_points = mask->points.total;
		if(submask_points > 1) total_points += submask_points;
	}

//printf("MaskEngine::do_mask 1 %d %d\n", total_points, keyframe->value);
// Ignore certain masks
	if(total_points < 2 || 
		(keyframe->value == 0 && default_auto->mode == MASK_SUBTRACT_ALPHA))
	{
		return;
	}

// Fake certain masks
	if(keyframe->value == 0 && default_auto->mode == MASK_MULTIPLY_ALPHA)
	{
		output->clear_frame();
		return;
	}

//printf("MaskEngine::do_mask 1\n");

	int new_color_model = 0;
	recalculate = 0;

	switch(output->get_color_model())
	{
		case BC_RGB_FLOAT:
		case BC_RGBA_FLOAT:
			new_color_model = BC_A_FLOAT;
			break;

		case BC_RGB888:
		case BC_RGBA8888:
		case BC_YUV888:
		case BC_YUVA8888:
			new_color_model = BC_A8;
			break;

		case BC_RGB161616:
		case BC_RGBA16161616:
		case BC_YUV161616:
		case BC_YUVA16161616:
			new_color_model = BC_A16;
			break;
	}

// Determine if recalculation is needed
SET_TRACE

	if(mask && 
		(mask->get_w() != output->get_w() ||
		mask->get_h() != output->get_h() ||
		mask->get_color_model() != new_color_model))
	{
		delete mask;
		delete temp_mask;
		mask = 0;
		recalculate = 1;
	}

	if(!recalculate)
	{
		if(point_sets.total != keyframe_set->total_submasks(start_position_project, 
			direction))
			recalculate = 1;
	}

	if(!recalculate)
	{
		for(int i = 0; 
			i < keyframe_set->total_submasks(start_position_project, 
				direction) && !recalculate; 
			i++)
		{
			ArrayList<MaskPoint*> *new_points = new ArrayList<MaskPoint*>;
			keyframe_set->get_points(new_points, 
				i, 
				start_position_project, 
				direction);
			if(!points_equivalent(new_points, point_sets.values[i])) recalculate = 1;
			new_points->remove_all_objects();
			delete new_points;
		}
	}

	if(recalculate ||
		!EQUIV(keyframe->feather, feather) ||
		!EQUIV(keyframe->value, value))
	{
		recalculate = 1;
		if(!mask) 
		{
			mask = new VFrame(0, 
					output->get_w(), 
					output->get_h(),
					new_color_model);
			temp_mask = new VFrame(0, 
					output->get_w(), 
					output->get_h(),
					new_color_model);
		}
		if(keyframe->feather > 0)
			temp_mask->clear_frame();
		else
			mask->clear_frame();

		for(int i = 0; i < point_sets.total; i++)
		{
			ArrayList<MaskPoint*> *points = point_sets.values[i];
			points->remove_all_objects();
		}
		point_sets.remove_all_objects();

		for(int i = 0; 
			i < keyframe_set->total_submasks(start_position_project, 
				direction); 
			i++)
		{
			ArrayList<MaskPoint*> *new_points = new ArrayList<MaskPoint*>;
			keyframe_set->get_points(new_points, 
				i, 
				start_position_project, 
				direction);
			point_sets.append(new_points);
		}
	}



	this->output = output;
	this->mode = default_auto->mode;
	this->feather = keyframe->feather;
	this->value = keyframe->value;


// Run units
SET_TRACE
	process_packages();
SET_TRACE


}

void MaskEngine::init_packages()
{
SET_TRACE
//printf("MaskEngine::init_packages 1\n");
	int division = (int)((float)output->get_h() / (get_total_packages()) + 0.5);
	if(division < 1) division = 1;

	stage1_finished_count = 0;
	if (recalculate) {
		last_nonempty_rowspan = SHRT_MIN;
		first_nonempty_rowspan = SHRT_MAX;
	}
SET_TRACE
// Always a multiple of 2 packages exist
	for(int i = 0; i < get_total_packages(); i++)
	{
		MaskPackage *pkg = (MaskPackage*)get_package(i);
		pkg->row1 = division * i;
		pkg->row2 = MIN (division * i + division, output->get_h());
		
		if(i == get_total_packages() - 1)  // last package
		{
			pkg->row2 = pkg->row2 = output->get_h();
		}

	}
SET_TRACE
//printf("MaskEngine::init_packages 2\n");
}

LoadClient* MaskEngine::new_client()
{
	return new MaskUnit(this);
}

LoadPackage* MaskEngine::new_package()
{
	return new MaskPackage;
}

