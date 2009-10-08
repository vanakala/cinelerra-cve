
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

#ifndef DENOISE_H
#define DENOISE_H



#include "bchash.inc"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"

class DenoiseEffect;
typedef enum { DECOMP, RECON } wavetype;

class DenoiseLevel : public BC_FPot
{
public:
	DenoiseLevel(DenoiseEffect *plugin, int x, int y);
	int handle_event();
	DenoiseEffect *plugin;
};

class DenoiseWindow : public BC_Window
{
public:
	DenoiseWindow(DenoiseEffect *plugin, int x, int y);
	void create_objects();
	void update();
	int close_event();
	DenoiseLevel *scale;
	DenoiseEffect *plugin;
};

PLUGIN_THREAD_HEADER(DenoiseEffect, DenoiseThread, DenoiseWindow)

class DenoiseConfig
{
public:
	DenoiseConfig();
	void copy_from(DenoiseConfig &that);
	int equivalent(DenoiseConfig &that);
	void interpolate(DenoiseConfig &prev, 
		DenoiseConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	double level;
};

class Tree
{
public:
	Tree(int input_length, int levels);
	~Tree();
	
	int input_length;
	int levels;
	double **values;
};

class WaveletCoeffs
{
public:
	WaveletCoeffs(double alpha, double beta);
	~WaveletCoeffs();
	
	double values[6];
	int length;
};

class WaveletFilters
{
public:
	WaveletFilters(WaveletCoeffs *wave_coeffs, wavetype transform);
	~WaveletFilters();

	double g[6], h[6];
	int length;
};

class DenoiseEffect : public PluginAClient
{
public:
	DenoiseEffect(PluginServer *server);
	~DenoiseEffect();

	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_realtime(int64_t size, double *input_ptr, double *output_ptr);




	int load_defaults();
	int save_defaults();
	void reset();
	void update_gui();
	void delete_dsp();

	void process_window();
	double dot_product(double *data, double *filter, char filtlen);
	int convolve_dec_2(double *input_sequence, 
		int64_t length,
		double *filter, 
		int filtlen, 
		double *output_sequence);
	int64_t decompose_branches(double *in_data, 
		int64_t length, 
		WaveletFilters *decomp_filter, 
		double *out_low, 
		double *out_high);
	int wavelet_decomposition(double *in_data, 
		int64_t in_length, 
		double **out_data);
	int tree_copy(double **output, 
		double **input, 
		int length, 
		int levels);
	int threshold(int window_size, double gammas, int levels);
	double dot_product_even(double *data, double *filter, int filtlen);
	double dot_product_odd(double *data, double *filter, int filtlen);
	int convolve_int_2(double *input_sequence, 
		int64_t length, 
		double *filter, 
		int filtlen, 
		int sum_output, 
		double *output_sequence);
	int64_t reconstruct_branches(double *in_low, 
		double *in_high, 
		int64_t in_length,
		WaveletFilters *recon_filter, 
		double *output);
	int wavelet_reconstruction(double **in_data, 
		int64_t in_length, 
		double *out_data);


	PLUGIN_CLASS_MEMBERS(DenoiseConfig, DenoiseThread)

// buffer for storing fragments until a complete window size is armed
	double *input_buffer;
	int64_t input_size;
	int64_t input_allocation;
// buffer for storing fragments until a fragment is ready to be read
	double *output_buffer;
	int64_t output_size;
	int64_t output_allocation;
	double *dsp_in;
	double *dsp_out;
// buffer for capturing output of a single iteration
	double *dsp_iteration;   
	Tree *ex_coeff_d, *ex_coeff_r, *ex_coeff_rn;
	WaveletCoeffs *wave_coeff_d, *wave_coeff_r;
	WaveletFilters *decomp_filter, *recon_filter;
// scaling factor for transferring from input_buffer
	double in_scale;
// power converted to scaling factor
	double out_scale;

// depends on the type of music
	int64_t levels;
// higher number reduces aliasing due to a high noise_level
// also increases high end
	int64_t iterations;
// daub6 coeffs
	double alpha;
	double beta;          
// power
	float output_level;
// higher number kills more noise at the expense of more aliasing
	float noise_level;
	int64_t window_size;
	int first_window;
	int initialized;
};


#endif
