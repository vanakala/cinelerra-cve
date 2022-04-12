// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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

MaskPackage::MaskPackage()
{
}


MaskUnit::MaskUnit(MaskEngine *engine)
 : LoadClient(engine)
{
	this->engine = engine;
	row_spans_h = 0;
	row_spans = 0;
	src = 0;
	dst = 0;
	val_p = 0;
	val_m = 0;
	size_allocated = 0;
}

MaskUnit::~MaskUnit()
{
	if(row_spans)
	{
		for (int i = 0; i < row_spans_h; i++) 
			free(row_spans[i]);
		delete [] row_spans;
	}
	delete [] src;
	delete [] dst;
	delete [] val_p;
	delete [] val_m;
}

inline void MaskUnit::draw_line_clamped(
	int draw_x1, 
	int draw_y1, 
	int draw_x2, 
	int draw_y2,
	int w,
	int h,
	int hoffset)
{
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

	double slope = (double)(draw_x2 - draw_x1) / (draw_y2 - draw_y1);
	w--;
	for(int y_i = draw_y1; y_i < draw_y2; y_i++) 
	{ 
		if(y_i >= h) 
			return; // since y gets larger, there is no point in continuing
		else if(y_i >= 0) 
		{ 
			int x = round(slope * (y_i - draw_y1) + draw_x1);
			int x_i = CLIP(x, 0, w); 

			// now insert into span in order
			short *span = row_spans[y_i + hoffset];
			if(span[0] >= span[1])
			{
			// do the reallocation
				span[1] *= 2;
				span = row_spans[y_i + hoffset] = (short *)realloc(span, span[1] * sizeof(short));
				// be careful! row_spans has to be updated!
			}

			short index = 2;
			while (index < span[0]  && span[index] < x_i)
				index++;
			for(int j = span[0]; j > index; j--)
				span[j] = span[j-1];

			span[index] = x_i;
			span[0] ++;
		} 
	} 
}

void MaskUnit::blur_strip(double *val_p, double *val_m,
	double *dst, double *src,
	int size, double max)
{
	double *sp_p = src;
	double *sp_m = src + size - 1;
	double *vp = val_p;
	double *vm = val_m + size - 1;
	double initial_p = sp_p[0];
	double initial_m = sp_m[0];

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
		double sum = val_p[i] + val_m[i];
		CLAMP(sum, 0, max);
		dst[i] = sum;
	}
}


int MaskUnit::do_feather_2(VFrame *output,
	VFrame *input, 
	int feather,
	int start_out, 
	int end_out)
{
	DO_FEATHER_N(unsigned char, uint32_t, 0xffff, feather);
}


void MaskUnit::do_feather(VFrame *output,
	VFrame *input, 
	int feather,
	int start_out, 
	int end_out)
{
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

	int frame_w = input->get_w();
	int frame_h = input->get_h();
	int size = MAX(frame_w, frame_h);

	if(size_allocated < size)
	{
		if(src)
		{
			delete [] src;
			delete [] dst;
			delete [] val_p;
			delete [] val_m;
		}
		src = new double[size];
		dst = new double[size];
		val_p = new double[size];
		val_m = new double[size];
		size_allocated = size;
	}
	int start_in = start_out - feather;
	int end_in = end_out + feather;

	if(start_in < 0)
		start_in = 0;
	if(end_in > frame_h)
		end_in = frame_h;

	int strip_size = end_in - start_in;

	for(int j = 0; j < frame_w; j++)
	{
		memset(val_p, 0, sizeof(double) * (end_in - start_in));
		memset(val_m, 0, sizeof(double) * (end_in - start_in));

		for(int l = 0, k = start_in; k < end_in; l++, k++)
		{
			uint16_t *in_row = (uint16_t*)input->get_row_ptr(k);
			src[l] = in_row[j];
		}

		blur_strip(val_p, val_m, dst, src, strip_size, 0xffff);

		for(int l = start_out - start_in, k = start_out; k < end_out; l++, k++)
		{
			uint16_t *out_row = (uint16_t*)output->get_row_ptr(k);
			out_row[j] = dst[l];
		}
	}

	for(int j = start_out; j < end_out; j++) \
	{
		uint16_t *out_row = (uint16_t*)output->get_row_ptr(j);
		memset(val_p, 0, sizeof(double) * frame_w);
		memset(val_m, 0, sizeof(double) * frame_w);

		for(int k = 0; k < frame_w; k++)
			src[k] = out_row[k];

		blur_strip(val_p, val_m, dst, src, frame_w, 0xffff);

		for(int k = 0; k < frame_w; k++)
			out_row[k] = dst[k];
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

// Generated oversampling frame
		int mask_w = mask->get_w();
		int mask_h = mask->get_h();
		int mask_color_model = mask->get_color_model();
		int oversampled_package_w = mask_w * OVERSAMPLE;
		int oversampled_package_h = (ptr->row2 - ptr->row1) * OVERSAMPLE;

		int local_first_nonempty_rowspan = mask_h;
		int local_last_nonempty_rowspan = 0;

		if(!row_spans || row_spans_h != mask_h * OVERSAMPLE)
		{
			int i;
			if(row_spans) 
			{  // size change
				for(i = 0; i < row_spans_h; i++)
					free(row_spans[i]);
				delete [] row_spans;
			}
			row_spans_h = mask_h * OVERSAMPLE;
			row_spans = new short *[mask_h * OVERSAMPLE]; 
			for(i= 0; i<mask_h * OVERSAMPLE; i++)
			{
				// we use malloc so we can use realloc
				row_spans[i] = (short *)malloc(sizeof(short) * NUM_SPANS);
				// [0] is initialized later
				row_spans[i][1] = NUM_SPANS;
			}
		}

		for(int k = 0; k < engine->point_sets.total; k++)
		{
			int old_x, old_y;
			old_x = SHRT_MIN; // sentinel
			ArrayList<MaskPoint*> *points = engine->point_sets.values[k];

			if(points->total < 2)
				continue;
			for(int i = ptr->row1 * OVERSAMPLE; i < ptr->row2 * OVERSAMPLE; i++)
				row_spans[i][0] = 2;

			for(int i = 0; i < points->total; i++)
			{
				MaskPoint *point1 = points->values[i];
				MaskPoint *point2 = (i >= points->total - 1) ? 
					points->values[0] : 
					points->values[i + 1];

				double x0 = point1->x;
				double y0 = point1->y;
				double x1 = point1->x + point1->control_x2;
				double y1 = point1->y + point1->control_y2;
				double x2 = point2->x + point2->control_x1;
				double y2 = point2->y + point2->control_y1;
				double x3 = point2->x;
				double y3 = point2->y;

// possible optimization here... since these coordinates are bounding box for curve
// we can continue with next curve if they are out of our range

// forward differencing bezier curves implementation taken from GPL code at
// http://cvs.sourceforge.net/viewcvs.py/guliverkli/guliverkli/src/subtitles/Rasterizer.cpp?rev=1.3

				double cx3, cx2, cx1, cx0, cy3, cy2, cy1, cy0;

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

				double maxaccel1 = fabs(2 * cy2) + fabs(6 * cy3);
				double maxaccel2 = fabs(2 * cx2) + fabs(6 * cx3);

				double maxaccel = maxaccel1 > maxaccel2 ? maxaccel1 : maxaccel2;
				double h = 1.0;

				if(maxaccel > 8.0 * OVERSAMPLE)
					h = sqrt((8.0 * OVERSAMPLE) / maxaccel);

				for(double t = 0.0; t < 1.0; t += h)
				{
					int x = cx0 + t * (cx1 + t * (cx2 + t * cx3));
					int y = cy0 + t * (cy1 + t * (cy2 + t * cy3));

					if (old_x != SHRT_MIN)
						draw_line_clamped(old_x, old_y, x, y,
							oversampled_package_w,
							oversampled_package_h,
							ptr->row1 * OVERSAMPLE);
					old_x = x;
					old_y = y;
				}

				int x = x3 * OVERSAMPLE;
				int y = (y3 - ptr->row1) * OVERSAMPLE;
				draw_line_clamped(old_x, old_y, x, y,
					oversampled_package_w,
					oversampled_package_h,
					ptr->row1 * OVERSAMPLE);
				old_x = x;
				old_y = y;
			}

			int value = ((double)engine->value / 100 * 0xffff);

			/* Scaneline sampling, inspired by Graphics gems I, page 81 */
			for (int i = ptr->row1; i < ptr->row2; i++) 
			{
				short min_x = SHRT_MAX;
				short max_x = SHRT_MIN;
				int j;                          // universal counter for 0..OVERSAMPLE-1
				short *span;                    // current span - set inside loops with j
				short span_p[OVERSAMPLE];       // pointers to current positions in spans
				#define P (span_p[j])           // current span pointer
				#define MAXP (span[0])          // current span length
				int num_empty_spans = 0;

				// get the initial span pointers ready
				for (j = 0; j < OVERSAMPLE; j++)
				{
					span = row_spans[j + i * OVERSAMPLE];
					P = 2;              // starting pointers to spans
					// hypotetical hypotetical fix goes here: take care that there is maximum one empty span for every subpixel
					if(MAXP != 2)
					{ // if span is not empty
						if(span[2] < min_x)
							min_x = span[2];           // take start of the first span
						if(span[MAXP-1] > max_x)
							max_x = span[MAXP-1]; // and end of last
					}
					else
					{ // span is empty
						num_empty_spans++;
					}
				}
				if(num_empty_spans == OVERSAMPLE)
					continue;       // no work for us here
				else
				{  // if we have engaged first nonempty rowspan... remember it to speed up mask applying
					if(i < local_first_nonempty_rowspan)
						local_first_nonempty_rowspan = i;
					if(i > local_last_nonempty_rowspan)
						local_last_nonempty_rowspan = i;
				}
				// we have some pixels to fill, do coverage calculation for span

				uint16_t *output_row = (uint16_t*)mask->get_row_ptr(i);
				min_x = min_x / OVERSAMPLE;
				max_x = (max_x + OVERSAMPLE - 1) / OVERSAMPLE;

				// this is not a full loop, since we jump trough h if possible
				for(int h = min_x; h <= max_x; h++)
				{
					short pixelleft = h * OVERSAMPLE;  // leftmost subpixel of pixel
					short pixelright = pixelleft + OVERSAMPLE - 1; // rightmost subpixel of pixel
					int coverage = 0;
					int num_left = 0;               // number of spans that have start left of the next pixel
					short right_end = SHRT_MAX;     // leftmost end of any span - right end of a full scanline
					short right_start = SHRT_MAX;   // leftmost start of any span - left end of empty scanline

					for(int j = 0; j < OVERSAMPLE; j++)
					{
						char chg = 1;
						span = row_spans[j + i * OVERSAMPLE];
						while (P < MAXP && chg)
						{
							if(span[P] == span[P+1])
							{ // ignore empty spans
								P += 2;
								continue;
							}
							if(span[P] <= pixelright) // if span start is before the end of pixel
								coverage += MIN(span[P+1], pixelright)  // 'clip' the span to pixel
									- MAX(span[P], pixelleft) + 1;
							if(span[P+1] <= pixelright)
								P += 2;
							else 
								chg = 0;
						} 
						if(P == MAXP)
							num_left = -OVERSAMPLE; // just take care that num_left cannot equal OVERSAMPLE or zero again
						else
						{ 
							if(span[P] <= pixelright)  // if span starts before subpixel in the pixel on the right
							{    // useful for determining filled space till next non-fully-filled pixel
								num_left++;
								if(span[P+1] < right_end)
									right_end = span[P+1];
							}
							else
							{  // useful for determining empty space till next non-empty pixel
								if(span[P] < right_start)
									right_start = span[P];
							}
						}
					}
					// calculate coverage
					coverage *= value;
					coverage /= OVERSAMPLE * OVERSAMPLE;

					// when we have multiple masks the highest coverage wins
					if(output_row[h] < coverage)
						output_row[h] = coverage;

					// possible optimization: do joining of multiple masks by span logics, not by bitmap logics

					if(num_left == OVERSAMPLE)
					{
						// all current spans start more left than next pixel
						// this means we can probably (if lucky) draw a longer horizontal line
						right_end = (right_end / OVERSAMPLE) - 1; // last fully covered pixel
						if(right_end > h)
						{
							for(int z = h +1; z <= right_end; z++)
								output_row[z] = value;
							h = right_end;
						}
					}
					else if(num_left == 0)
					{
						// all current spans start right of next pixel
						// this means we can probably (if lucky) skip some pixels
						right_start = (right_start / OVERSAMPLE) - 1; // last fully empty pixel
						if(right_start > h)
							h = right_start;
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

	} // END OF RECALCULATION!

	// possible optimization: this could be useful for do_feather also

	// Feather polygon
	if(engine->recalculate && engine->feather > 0) 
	{
		// first take care that all packages are already drawn onto mask
		pthread_mutex_lock(&engine->stage1_finished_mutex);
		engine->stage1_finished_count++;
		if(engine->stage1_finished_count == engine->get_total_packages())
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

		// now do the feather

		int done = 0;
		done = do_feather_2(engine->mask,        // try if we have super fast implementation ready
				engine->temp_mask,
				engine->feather * 2 - 1, 
				ptr->row1, 
				ptr->row2);
		if(done)
			engine->realfeather = engine->feather;
		else
		{
			double feather = engine->feather;
			engine->realfeather = round(0.878441 + 0.988534 * feather -
				0.0490204 * feather * feather +
				0.0012359 * feather * feather * feather);
			do_feather(engine->mask, 
				engine->temp_mask, 
				engine->realfeather, 
				ptr->row1, 
				ptr->row2); 
		}
	}
	else if(engine->feather <= 0)
		engine->realfeather = 0;

	start_row = MAX(ptr->row1, engine->first_nonempty_rowspan - engine->realfeather);
	end_row = MIN(ptr->row2, engine->last_nonempty_rowspan + 1 + engine->realfeather);

// Apply mask
/* use the info about first and last column that are coloured from rowspan!  */
/* possible optimisation: also remember total spans */
/* possible optimisation: lookup for  X * (max - *mask_row) / max, where max is known mask_row and X are variabiles */

	int mask_w = engine->mask->get_w();

	switch(engine->mode)
	{
	case MASK_MULTIPLY_ALPHA:
		switch(engine->output->get_color_model())
		{
		case BC_RGBA16161616:
			for(int i = ptr->row1; i < ptr->row2; i++) \
			{
				uint16_t *output_row = (uint16_t*)engine->output->get_row_ptr(i);
				uint16_t *mask_row = (uint16_t*)engine->mask->get_row_ptr(i);

				output_row += 3;

				for(int j = mask_w; j != 0;  j--)
				{
					*output_row = *output_row * *mask_row / 0xffff;
					output_row += 4;
					mask_row += 1;
				}
			}
			break;

		case BC_AYUV16161616:
			for(int i = ptr->row1; i < ptr->row2; i++)
			{
				uint16_t *output_row = (uint16_t*)engine->output->get_row_ptr(i); \
				uint16_t *mask_row = (uint16_t*)engine->mask->get_row_ptr(i); \

				for(int j = mask_w; j != 0;  j--)
				{
					*output_row = *output_row * *mask_row / 0xffff;
					output_row += 4;
					mask_row += 1;
				}
			}
			break;
		}
		break;

	case MASK_SUBTRACT_ALPHA:
		switch(engine->output->get_color_model())
		{
		case BC_RGBA16161616:
			for(int i = start_row; i < end_row; i++)
			{
				uint16_t *output_row = (uint16_t*)engine->output->get_row_ptr(i); \
				uint16_t *mask_row = (uint16_t*)engine->mask->get_row_ptr(i); \

				for(int j  = 0; j < mask_w; j++)
				{
					output_row[3] = output_row[3] *
						(0xffff - *mask_row) / 0xffff;
					output_row += 4;
					mask_row += 1;
				}
			}
			break;
		case BC_AYUV16161616:
			for(int i = start_row; i < end_row; i++)
			{
				uint16_t *output_row = (uint16_t*)engine->output->get_row_ptr(i);
				uint16_t *mask_row = (uint16_t*)engine->mask->get_row_ptr(i);

				for(int j  = 0; j < mask_w; j++) \
				{
					output_row[0] = output_row[0] *
						(0xffff - *mask_row) / 0xffff;
					output_row += 4;
					mask_row += 1;
				}
			}
			break;
		}
		break;

	case MASK_MULTIPLY_COLOR:
		switch(engine->output->get_color_model())
		{
		case BC_RGBA16161616:
			for(int i = ptr->row1; i < ptr->row2; i++)
			{
				uint16_t *output_row = (uint16_t*)engine->output->get_row_ptr(i);
				uint16_t *mask_row = (uint16_t*)engine->mask->get_row_ptr(i);

				for(int j = mask_w; j != 0;  j--)
				{
					output_row[0] = output_row[0] * *mask_row / 0xffff;
					output_row[1] = output_row[1] * *mask_row / 0xffff;
					output_row[2] = output_row[2] * *mask_row / 0xffff;

					output_row += 4;
					mask_row += 1;
				}
			}
			break;
		case BC_AYUV16161616:
			for(int i = ptr->row1; i < ptr->row2; i++)
			{
				uint16_t *output_row = (uint16_t*)engine->output->get_row_ptr(i);
				uint16_t *mask_row = (uint16_t*)engine->mask->get_row_ptr(i);

				for(int j = mask_w; j != 0;  j--)
				{
					output_row[1] = output_row[1] * *mask_row / 0xffff;
					output_row[2] = output_row[2] * *mask_row / 0xffff;
					output_row[3] = output_row[3] * *mask_row / 0xffff;

					output_row[2] += 0x8000 * (0xffff - *mask_row) / 0xffff;
					output_row[3] += 0x8000 * (0xffff - *mask_row) / 0xffff;
					output_row += 4;
					mask_row += 1;
				}
			}
			break;
		}
		break;

	case MASK_SUBTRACT_COLOR:
		switch(engine->output->get_color_model())
		{
		case BC_RGBA16161616:
			for(int i = start_row; i < end_row; i++)
			{
				uint16_t *output_row = (uint16_t*)engine->output->get_row_ptr(i);
				uint16_t *mask_row = (uint16_t*)engine->mask->get_row_ptr(i);

				for(int j  = 0; j < mask_w; j++)
				{
					output_row[0] = output_row[0] * (0xffff - *mask_row) / 0xffff;
					output_row[1] = output_row[1] * (0xffff - *mask_row) / 0xffff;
					output_row[2] = output_row[2] * (0xffff - *mask_row) / 0xffff;

					output_row += 4;
					mask_row += 1;
				}
			}
			break;
		case BC_AYUV16161616:
			for(int i = start_row; i < end_row; i++)
			{
				uint16_t *output_row = (uint16_t*)engine->output->get_row_ptr(i);
				uint16_t *mask_row = (uint16_t*)engine->mask->get_row_ptr(i);

				for(int j = 0; j < mask_w; j++)
				{
					output_row[1] = output_row[1] * (0xffff - *mask_row) / 0xffff;
					output_row[2] = output_row[2] * (0xffff - *mask_row) / 0xffff;
					output_row[3] = output_row[3] * (0xffff - *mask_row) / 0xffff;
					output_row[2] += 0x8000 * *mask_row / 0xffff;
					output_row[3] += 0x8000 * *mask_row / 0xffff;
					output_row += 4;
					mask_row += 1;
				}
			}
			break;
		}
		break;
	}
}


MaskEngine::MaskEngine(int cpus)
 : LoadServer(cpus, cpus)      /* these two HAVE to be the same, since packages communicate  */
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
	if(new_points->total != points->total) return 0;

	for(int i = 0; i < new_points->total; i++)
	{
		if(!(*new_points->values[i] == *points->values[i])) return 0;
	}
	return 1;
}

void MaskEngine::do_mask(VFrame *output, 
	MaskAutos *keyframe_set, 
	int before_plugins)
{
	ptstime start_pts = output->get_pts();
	int new_value;
	int new_feather;
	int cur_mode = keyframe_set->get_mode();
	int total_points = 0;
	int new_color_model = 0;

	MaskAuto *keyframe = (MaskAuto*)keyframe_set->get_prev_auto(start_pts);

	if(!keyframe)
		return;
	if(keyframe->apply_before_plugins != before_plugins)
		return;

	for(int i = 0; i < keyframe->masks.total; i++)
	{
		SubMask *mask = keyframe->get_submask(i);
		int submask_points = mask->points.total;
		if(submask_points > 1) total_points += submask_points;
	}

	keyframe->interpolate_values(start_pts, &new_value, &new_feather);

// Ignore certain masks
	if(total_points < 2 || (new_value == 0 &&
			(cur_mode == MASK_SUBTRACT_ALPHA || cur_mode == MASK_SUBTRACT_COLOR)))
		return;

// Fake certain masks
	if(new_value == 0 && cur_mode == MASK_MULTIPLY_COLOR)
	{
		output->clear_frame();
		return;
	}

	recalculate = 0;

	switch(output->get_color_model())
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		new_color_model = BC_A16;
		break;
	default:
		return;
	}

// Determine if recalculation is needed

	if(mask && (mask->get_w() != output->get_w() ||
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
		if(point_sets.total != keyframe_set->total_submasks(start_pts))
			recalculate = 1;
	}

	if(!recalculate)
	{
		for(int i = 0; 
			i < keyframe_set->total_submasks(start_pts) && !recalculate;
			i++)
		{
			ArrayList<MaskPoint*> *new_points = new ArrayList<MaskPoint*>;

			keyframe_set->get_points(new_points, i, start_pts);
			if(!points_equivalent(new_points, point_sets.values[i]))
				recalculate = 1;
			new_points->remove_all_objects();
			delete new_points;
		}
	}

	if(recalculate || new_feather != feather || new_value != value)
	{
		recalculate = 1;
		if(!mask) 
		{
			mask = new VFrame(0, output->get_w(),
					output->get_h(),
					new_color_model);
			temp_mask = new VFrame(0, output->get_w(),
					output->get_h(),
					new_color_model);
		}
		if(new_feather > 0)
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
			i < keyframe_set->total_submasks(start_pts);
			i++)
		{
			ArrayList<MaskPoint*> *new_points = new ArrayList<MaskPoint*>;
			keyframe_set->get_points(new_points, 
				i, 
				start_pts);
			point_sets.append(new_points);
		}
	}
	this->output = output;
	this->mode = cur_mode;
	this->feather = new_feather;
	this->value = new_value;

// Run units
	process_packages();
}

void MaskEngine::init_packages()
{
	int division = ((double)output->get_h() / (get_total_packages()) + 0.5);

	if(division < 1)
		division = 1;

	stage1_finished_count = 0;

	if(recalculate)
	{
		last_nonempty_rowspan = 0;
		first_nonempty_rowspan = output->get_h();
	}

	for(int i = 0; i < get_total_packages(); i++)
	{
		MaskPackage *pkg = (MaskPackage*)get_package(i);
		pkg->row1 = division * i;
		pkg->row2 = MIN (division * i + division, output->get_h());
	}
}

LoadClient* MaskEngine::new_client()
{
	return new MaskUnit(this);
}

LoadPackage* MaskEngine::new_package()
{
	return new MaskPackage;
}
