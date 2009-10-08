
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

#ifndef OVERLAYFRAME_H
#define OVERLAYFRAME_H

#include "loadbalance.h"
#include "overlayframe.inc"
#include "vframe.inc"


// Issues encoutered with overlay

// Floating point vs. int.  On both alpha and Intel 
// the floating point interpolation is over
// 2x faster than integer interpolation.  Pipelined CPUs probably don't benefit
// at all in long sequences of integer calculations.  Integer interpolation uses 32 
// siginificant bits while floating point uses 24, however.

// Single step vs. two step process.

// A single step process would require performing blend with the output
// of BILINEAR or BICUBIC and trimming the output to fractional pixels.
// This is easy.  

// However reading the input for
// BILINEAR and BICUBIC would require trimming the input to fractional
// pixels often repeatedly since the interpolation reads the same pixels more
// than once.  This is hard.

// In the two step process one step worries purely about scaling, ignoring 
// trimming at the input and output so redundant trimming is not done here.

// The translation engine focuses purely on trimming input pixels and
// blending with the output.

// Another advantage to the two step process is further optimization can be achieved
// by leaving out translation or scaling.

// Translation

typedef struct
{
	int in_x1;
	int in_fraction1;
	int in_x2;       // Might be same as in_x1 for boundary
	int in_fraction2;
	int output_fraction;
} transfer_table_i;

typedef struct
{
	int in_x1;
	float in_fraction1;
	int in_x2;       // Might be same as in_x1 for boundary
	float in_fraction2;
	float output_fraction;
} transfer_table_f;

typedef struct
{
	int input_pixel1;
	int input_pixel2;
	float input_fraction1;
	float input_fraction2;
	float input_fraction3;
	float total_fraction;
} bilinear_table_t;

class ScaleEngine;

class ScalePackage : public LoadPackage
{
public:
	ScalePackage();

	int out_row1, out_row2;
};

class ScaleUnit : public LoadClient
{
public:
	ScaleUnit(ScaleEngine *server, OverlayFrame *overlay);
	~ScaleUnit();
	
	float cubic_bspline(float x);

	void tabulate_bcubic_f(float* &coef_table, 
		int* &coord_table,
		float scale,
		float start, 
		int pixels,
		int total_pixels,
		float coefficient);
	void tabulate_blinear_f(int* &table_int1,
		int* &table_int2,
		float* &table_frac,
		float* &table_antifrac,
		float scale,
		int pixel1,
		int pixel2,
		float start,
		int total_pixels);

	void tabulate_bcubic_i(int* &coef_table, 
		int* &coord_table,
		float scale,
		int start, 
		int pixels,
		int total_pixels,
		float coefficient);
	void tabulate_blinear_i(int* &table_int1,
		int* &table_int2,
		int* &table_frac,
		int* &table_antifrac,
		float scale,
		int pixel1,
		int pixel2,
		float start,
		int total_pixels);
	void tabulate_reduction(bilinear_table_t* &table,
		float scale,
		int pixel1,
		int out_total,
		int in_total);
	void tabulate_enlarge(bilinear_table_t* &table,
		float scale,
		float pixel1,
		int out_total,
		int in_total);
	void dump_bilinear(bilinear_table_t *table, int total);

	void process_package(LoadPackage *package);
	
	OverlayFrame *overlay;
	ScaleEngine *engine;
};

class ScaleEngine : public LoadServer
{
public:
	ScaleEngine(OverlayFrame *overlay, int cpus);
	~ScaleEngine();
	
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	
	OverlayFrame *overlay;
// Global parameters for scaling units
	VFrame *scale_output;
	VFrame *scale_input;
	float w_scale;
	float h_scale;
	float in_x1_float;
	float in_y1_float;
	int out_w_int;
	int out_h_int;
	int interpolation_type;
};







class TranslateEngine;

class TranslatePackage : public LoadPackage
{
public:
	TranslatePackage();

	int out_row1, out_row2;
};


class TranslateUnit : public LoadClient
{
public:
	TranslateUnit(TranslateEngine *server, OverlayFrame *overlay);
	~TranslateUnit();

	void process_package(LoadPackage *package);
	static void translation_array_f(transfer_table_f* &table, 
		float out_x1, 
		float out_x2,
		float in_x1,
		float in_x2,
		int in_total, 
		int out_total, 
		int &out_x1_int,
		int &out_x2_int);
	void translation_array_i(transfer_table_i* &table, 
		float out_x1, 
		float out_x2,
		float in_x1,
		float in_x2,
		int in_total, 
		int out_total, 
		int &out_x1_int,
		int &out_x2_int);
	void translate(VFrame *output, 
			VFrame *input, 
			float in_x1,
			float in_y1,
			float in_x2,
			float in_y2,
			float out_x1,
			float out_y1,
			float out_x2,
			float out_y2,
			float alpha,
			int mode,
			int row1,
			int row2);

	OverlayFrame *overlay;
	TranslateEngine *engine;
};

class TranslateEngine : public LoadServer
{
public:
	TranslateEngine(OverlayFrame *overlay, int cpus);
	~TranslateEngine();
	
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	
	OverlayFrame *overlay;
// Global parameters for translate units
	VFrame *translate_output;
	VFrame *translate_input;
	float translate_in_x1;
	float translate_in_y1;
	float translate_in_x2;
	float translate_in_y2;
	float translate_out_x1;
	float translate_out_y1;
	float translate_out_x2;
	float translate_out_y2;
	float translate_alpha;
	int translate_mode;
};









class ScaleTranslateEngine;

class ScaleTranslatePackage : public LoadPackage
{
public:
	ScaleTranslatePackage();

	int out_row1, out_row2;
};


class ScaleTranslateUnit : public LoadClient
{
public:
	ScaleTranslateUnit(ScaleTranslateEngine *server, OverlayFrame *overlay);
	~ScaleTranslateUnit();

	void process_package(LoadPackage *package);
	void scale_array_f(int* &table, 
		int out_x1, 
		int out_x2,
		float in_x1,
		float in_x2);
	
	OverlayFrame *overlay;
	ScaleTranslateEngine *scale_translate;
};

class ScaleTranslateEngine : public LoadServer
{
public:
	ScaleTranslateEngine(OverlayFrame *overlay, int cpus);
	~ScaleTranslateEngine();
	
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	
	OverlayFrame *overlay;
	
	
// Arguments
	VFrame *output;
	VFrame *input;
	float in_x1;
	float in_y1;
	float in_x2;
	float in_y2;
	int out_x1;
	int out_y1;
	int out_x2;
	int out_y2;
	float alpha;
	int mode;
};















class BlendEngine;

class BlendPackage : public LoadPackage
{
public:
	BlendPackage();

	int out_row1, out_row2;
};


class BlendUnit : public LoadClient
{
public:
	BlendUnit(BlendEngine *server, OverlayFrame *overlay);
	~BlendUnit();

	void process_package(LoadPackage *package);
	void translation_array_f(transfer_table_f* &table, 
		float out_x1, 
		float out_x2,
		float in_x1,
		float in_x2,
		int in_total, 
		int out_total, 
		int &out_x1_int,
		int &out_x2_int);
	void translate(VFrame *output, 
			VFrame *input, 
			float in_x1,
			float in_y1,
			float in_x2,
			float in_y2,
			float out_x1,
			float out_y1,
			float out_x2,
			float out_y2,
			float alpha,
			int mode,
			int row1,
			int row2);

	OverlayFrame *overlay;
	BlendEngine *blend_engine;
};

class BlendEngine : public LoadServer
{
public:
	BlendEngine(OverlayFrame *overlay, int cpus);
	~BlendEngine();
	
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	
	OverlayFrame *overlay;
	
	
// Arguments
	VFrame *output;
	VFrame *input;
	float alpha;
	int mode;
};













class OverlayFrame
{
public:
	OverlayFrame(int cpus = 1);
	virtual ~OverlayFrame();

// Alpha is from 0 - 1
	int overlay(VFrame *output, 
		VFrame *input, 
		float in_x1, 
		float in_y1, 
		float in_x2, 
		float in_y2, 
		float out_x1, 
		float out_y1, 
		float out_x2, 
		float out_y2, 
		float alpha,        // 0 - 1
		int mode,
		int interpolation_type);
	int use_alpha, use_float, mode, interpolate;
	int color_model;

	BlendEngine *blend_engine;
	ScaleEngine *scale_engine;
	TranslateEngine *translate_engine;
	ScaleTranslateEngine *scaletranslate_engine;


	VFrame *temp_frame;
	int cpus;

};

#endif
