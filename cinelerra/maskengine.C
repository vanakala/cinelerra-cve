#include "clip.h"
#include "maskauto.h"
#include "maskautos.h"
#include "maskengine.h"
#include "mutex.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

MaskPackage::MaskPackage()
{
	apply_mutex = new Mutex;
}

MaskPackage::~MaskPackage()
{
	delete apply_mutex;
}







MaskUnit::MaskUnit(MaskEngine *engine)
 : LoadClient(engine)
{
	this->engine = engine;
	this->temp = 0;
}


MaskUnit::~MaskUnit()
{
	if(temp) delete temp;
}

#ifndef SQR
#define SQR(x) ((x) * (x))
#endif

#define OVERSAMPLE 8













#define DRAW_LINE_CLAMPED(type, value) \
{ \
	type **rows = (type**)frame->get_rows(); \
 \
	if(draw_y2 != draw_y1) \
	{ \
		float slope = ((float)draw_x2 - draw_x1) / ((float)draw_y2 - draw_y1); \
		int w = frame->get_w() - 1; \
		int h = frame->get_h(); \
 \
		for(float y = draw_y1; y < draw_y2; y++) \
		{ \
			if(y >= 0 && y < h) \
			{ \
				int x = (int)((y - draw_y1) * slope + draw_x1); \
				int y_i = (int)y; \
				int x_i = CLIP(x, 0, w); \
 \
				if(rows[y_i][x_i] == value) \
					rows[y_i][x_i] = 0; \
				else \
					rows[y_i][x_i] = value; \
			} \
		} \
	} \
}



void MaskUnit::draw_line_clamped(VFrame *frame, 
	int &x1, 
	int &y1, 
	int x2, 
	int y2,
	unsigned char k)
{
//printf("MaskUnit::draw_line_clamped 1 %d %d %d %d\n", x1, y1, x2, y2);
	int draw_x1;
	int draw_y1;
	int draw_x2;
	int draw_y2;
	unsigned char value;

	if(y2 < y1)
	{
		draw_x1 = x2;
		draw_y1 = y2;
		draw_x2 = x1;
		draw_y2 = y1;
	}
	else
	{
		draw_x1 = x1;
		draw_y1 = y1;
		draw_x2 = x2;
		draw_y2 = y2;
	}

	switch(frame->get_color_model())
	{
		case BC_A8:
			DRAW_LINE_CLAMPED(unsigned char, k);
			break;
		
		case BC_A16:
			DRAW_LINE_CLAMPED(uint16_t, k);
			break;
	}
}

void MaskUnit::blur_strip(float *val_p, 
	float *val_m, 
	float *dst, 
	float *src, 
	int size,
	int max)
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
	}




}

void MaskUnit::process_package(LoadPackage *package)
{
	MaskPackage *ptr = (MaskPackage*)package;

	if(engine->recalculate && ptr->part == RECALCULATE_PART)
	{
		VFrame *mask;
//printf("MaskUnit::process_package 1 %d\n", get_package_number());
		if(engine->feather > 0) 
			mask = engine->temp_mask;
		else
			mask = engine->mask;

// Generated oversampling frame
		int mask_w = mask->get_w();
		int mask_h = mask->get_h();
		int oversampled_package_w = mask_w * OVERSAMPLE;
		int oversampled_package_h = (ptr->row2 - ptr->row1) * OVERSAMPLE;
//printf("MaskUnit::process_package 1\n");

		if(temp && 
			(temp->get_w() != oversampled_package_w ||
			temp->get_h() != oversampled_package_h))
		{
			delete temp;
			temp = 0;
		}
//printf("MaskUnit::process_package 1\n");

		if(!temp)
		{
			temp = new VFrame(0, 
				oversampled_package_w, 
				oversampled_package_h,
				BC_A8);
		}

		temp->clear_frame();
//printf("MaskUnit::process_package 1 %d\n", engine->point_sets.total);


// Draw oversampled region of polygons on temp
		for(int k = 0; k < engine->point_sets.total; k++)
		{
			int old_x, old_y;
			unsigned char max = k + 1;
			ArrayList<MaskPoint*> *points = engine->point_sets.values[k];

			if(points->total < 3) continue;
//printf("MaskUnit::process_package 2 %d %d\n", k, points->total);
			for(int i = 0; i < points->total; i++)
			{
				MaskPoint *point1 = points->values[i];
				MaskPoint *point2 = (i >= points->total - 1) ? 
					points->values[0] : 
					points->values[i + 1];

				float x, y;
				int segments = (int)(sqrt(SQR(point1->x - point2->x) + SQR(point1->y - point2->y)));
				float x0 = point1->x;
				float y0 = point1->y;
				float x1 = point1->x + point1->control_x2;
				float y1 = point1->y + point1->control_y2;
				float x2 = point2->x + point2->control_x1;
				float y2 = point2->y + point2->control_y1;
				float x3 = point2->x;
				float y3 = point2->y;

				for(int j = 0; j <= segments; j++)
				{
					float t = (float)j / segments;
					float tpow2 = t * t;
					float tpow3 = t * t * t;
					float invt = 1 - t;
					float invtpow2 = invt * invt;
					float invtpow3 = invt * invt * invt;

					x = (        invtpow3 * x0
						+ 3 * t     * invtpow2 * x1
						+ 3 * tpow2 * invt     * x2 
						+     tpow3            * x3);
					y = (        invtpow3 * y0 
						+ 3 * t     * invtpow2 * y1
						+ 3 * tpow2 * invt     * y2 
						+     tpow3            * y3);

					y -= ptr->row1;
					x *= OVERSAMPLE;
					y *= OVERSAMPLE;

					if(j > 0)
					{
						draw_line_clamped(temp, old_x, old_y, (int)x, (int)y, max);
					}

					old_x = (int)x;
					old_y = (int)y;
				}
			}

//printf("MaskUnit::process_package 1\n");





#define FILL_ROWS(type) \
for(int i = 0; i < oversampled_package_h; i++) \
{ \
	type *row = (type*)temp->get_rows()[i]; \
	int value = 0x0; \
	int total = 0; \
 \
 	for(int j = 0; j < oversampled_package_w; j++) \
		if(row[j] == max) total++; \
 \
 	if(total > 1) \
	{ \
		if(total & 0x1) total--; \
		for(int j = 0; j < oversampled_package_w; j++) \
		{ \
			if(row[j] == max && total > 0) \
			{ \
				if(value)  \
					value = 0x0; \
				else \
					value = max; \
				total--; \
			} \
			else \
			{ \
				if(value) row[j] = value; \
			} \
		} \
	} \
}


// Fill in the polygon in the horizontal direction
			switch(temp->get_color_model())
			{
				case BC_A8:
					FILL_ROWS(unsigned char);
					break;

				case BC_A16:
					FILL_ROWS(uint16_t);
					break;
			}
		}








#define DOWNSAMPLE(type, value) \
for(int i = 0; i < ptr->row2 - ptr->row1; i++) \
{ \
	type *output_row = (type*)mask->get_rows()[i + ptr->row1]; \
	unsigned char **input_rows = (unsigned char**)temp->get_rows() + i * OVERSAMPLE; \
 \
 \
	for(int j = 0; j < mask_w; j++) \
	{ \
		int64_t total = 0; \
 \
/* Accumulate pixel */ \
		for(int k = 0; k < OVERSAMPLE; k++) \
		{ \
			unsigned char *input_vector = input_rows[k] + j * OVERSAMPLE; \
			for(int l = 0; l < OVERSAMPLE; l++) \
			{ \
				total += (input_vector[l] ? value : 0); \
			} \
		} \
 \
/* Divide pixel */ \
		if(OVERSAMPLE == 8) \
			total >>= 6; \
		else \
		if(OVERSAMPLE == 4) \
			total >>= 2; \
		else \
		if(OVERSAMPLE == 2) \
			total >>= 2; \
		else \
			total /= OVERSAMPLE * OVERSAMPLE; \
 \
		output_row[j] = total; \
	} \
}


// Downsample polygon
		switch(mask->get_color_model())
		{
			case BC_A8:
			{
				unsigned char value;
				value = (int)((float)engine->value / 100 * 0xff);
				DOWNSAMPLE(unsigned char, value);
				break;
			}

			case BC_A16:
			{
				uint16_t value;
				value = (int)((float)engine->value / 100 * 0xffff);
				DOWNSAMPLE(uint16_t, value);
				break;
			}
		}

	}


	if(ptr->part == RECALCULATE_PART)
	{
// The feather could span more than one package so can't do it until
// all packages are drawn.
		if(get_package_number() >= engine->get_total_packages() / 2 - 1)
		{
			for(int i = engine->get_total_packages() / 2; 
				i < engine->get_total_packages();
				i++)
			{
				MaskPackage *package = (MaskPackage*)engine->get_package(i);
				package->apply_mutex->unlock();
			}
		}

	}

//printf("MaskUnit::process_package 2\n");

	if(ptr->part == APPLY_PART)
	{
//printf("MaskUnit::process_package 2.1\n");
		ptr->apply_mutex->lock();
		ptr->apply_mutex->unlock();
//printf("MaskUnit::process_package 2.2\n");

		if(engine->recalculate)
		{
// Feather polygon
			if(engine->feather > 0) do_feather(engine->mask, 
				engine->temp_mask, 
				engine->feather, 
				ptr->row1, 
				ptr->row2);

		}
//printf("MaskUnit::process_package 3 %f\n", engine->feather);




// Apply mask
		int mask_w = engine->mask->get_w();


#define APPLY_MASK_SUBTRACT_ALPHA(type, max, components, do_yuv) \
{ \
	type *output_row = (type*)engine->output->get_rows()[i]; \
	type *mask_row = (type*)engine->mask->get_rows()[i]; \
	int chroma_offset = (max + 1) / 2; \
 \
	for(int j  = 0; j < mask_w; j++) \
	{ \
		if(components == 4) \
		{ \
			output_row[j * 4 + 3] = output_row[j * 4 + 3] * (max - mask_row[j]) / max; \
		} \
		else \
		{ \
			output_row[j * 3] = output_row[j * 3] * (max - mask_row[j]) / max; \
 \
			output_row[j * 3 + 1] = output_row[j * 3 + 1] * (max - mask_row[j]) / max; \
			output_row[j * 3 + 2] = output_row[j * 3 + 2] * (max - mask_row[j]) / max; \
 \
			if(do_yuv) \
			{ \
				output_row[j * 3 + 1] += chroma_offset * mask_row[j] / max; \
				output_row[j * 3 + 2] += chroma_offset * mask_row[j] / max; \
			} \
		} \
	} \
}

#define APPLY_MASK_MULTIPLY_ALPHA(type, max, components, do_yuv) \
{ \
	type *output_row = (type*)engine->output->get_rows()[i]; \
	type *mask_row = (type*)engine->mask->get_rows()[i]; \
	int chroma_offset = (max + 1) / 2; \
 \
	for(int j  = 0; j < mask_w; j++) \
	{ \
		if(components == 4) \
		{ \
			output_row[j * 4 + 3] = output_row[j * 4 + 3] * mask_row[j] / max; \
		} \
		else \
		{ \
			output_row[j * 3] = output_row[j * 3] * mask_row[j] / max; \
 \
			output_row[j * 3 + 1] = output_row[j * 3 + 1] * mask_row[j] / max; \
			output_row[j * 3 + 2] = output_row[j * 3 + 2] * mask_row[j] / max; \
 \
			if(do_yuv) \
			{ \
				output_row[j * 3 + 1] += chroma_offset * (max - mask_row[j]) / max; \
				output_row[j * 3 + 2] += chroma_offset * (max - mask_row[j]) / max; \
			} \
		} \
	} \
}




//printf("MaskUnit::process_package 1 %d\n", engine->mode);
		for(int i = ptr->row1; i < ptr->row2; i++)
		{
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
					}
					break;
			}
		}
	}
//printf("MaskUnit::process_package 4 %d\n", get_package_number());
}





MaskEngine::MaskEngine(int cpus)
 : LoadServer(cpus, cpus * OVERSAMPLE * 2)
// : LoadServer(1, 2)
{
	mask = 0;
}

MaskEngine::~MaskEngine()
{
	if(mask) 
	{
		delete mask;
		delete temp_mask;
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
	MaskAutos *keyframe_set, 
	long position, 
	int direction)
{
//printf("MaskEngine::do_mask 1\n");
	Auto *current = 0;
	MaskAuto *default_auto = (MaskAuto*)keyframe_set->default_auto;
	MaskAuto *keyframe = (MaskAuto*)keyframe_set->get_prev_auto(position, 
		direction,
		current);


// Nothing to be done
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

//printf("MaskEngine::do_mask 1 %d\n", recalculate);
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

//printf("MaskEngine::do_mask %d\n", recalculate);
	if(!recalculate)
	{
		if(point_sets.total != keyframe_set->total_submasks(position, direction))
			recalculate = 1;
	}

//printf("MaskEngine::do_mask %d\n", recalculate);
	if(!recalculate)
	{
		for(int i = 0; 
			i < keyframe_set->total_submasks(position, direction) && !recalculate; 
			i++)
		{
			ArrayList<MaskPoint*> *new_points = new ArrayList<MaskPoint*>;
			keyframe_set->get_points(new_points, i, position, direction);
			if(!points_equivalent(new_points, point_sets.values[i])) recalculate = 1;
			new_points->remove_all_objects();
		}
	}

//printf("MaskEngine::do_mask 3 %d\n", recalculate);
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
		point_sets.remove_all_objects();

		for(int i = 0; 
			i < keyframe_set->total_submasks(position, direction); 
			i++)
		{
			ArrayList<MaskPoint*> *new_points = new ArrayList<MaskPoint*>;
			keyframe_set->get_points(new_points, i, position, direction);
			point_sets.append(new_points);
		}
	}

//printf("MaskEngine::do_mask 4 %d\n", recalculate);


	this->output = output;
	this->mode = default_auto->mode;
	this->feather = keyframe->feather;
	this->value = keyframe->value;
//printf("MaskEngine::do_mask 5\n");


// Run units
	process_packages();


//printf("MaskEngine::do_mask 6\n");
}

void MaskEngine::init_packages()
{
//printf("MaskEngine::init_packages 1\n");
	int division = (int)((float)output->get_h() / (total_packages / 2) + 0.5);
	if(division < 1) division = 1;

// Always a multiple of 2 packages exist
	for(int i = 0; i < get_total_packages() / 2; i++)
	{
		MaskPackage *part1 = (MaskPackage*)packages[i];
		MaskPackage *part2 = (MaskPackage*)packages[i + total_packages / 2];
		part2->row1 = part1->row1 = division * i;
		part2->row2 = part1->row2 = division * i + division;
		part2->row1 = part1->row1 = MIN(output->get_h(), part1->row1);
		part2->row2 = part1->row2 = MIN(output->get_h(), part1->row2);
		
		if(i >= (total_packages / 2) - 1) 
		{
			part2->row2 = part1->row2 = output->get_h();
		}

		part2->apply_mutex->lock();

		part1->part = RECALCULATE_PART;
		part2->part = APPLY_PART;
	}
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

