// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "bchash.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "denoise.h"
#include "picon_png.h"
#include "units.h"

#define WINDOW_SIZE 4096
#define WINDOW_BORDER (WINDOW_SIZE / 2)

#define NUM_WAVELET_COEFFS 6

REGISTER_PLUGIN

DenoiseEffect::DenoiseEffect(PluginServer *server)
 : PluginAClient(server)
{
	thread = 0;
	ex_coeff_d = 0;
	ex_coeff_r = 0;
	ex_coeff_rn = 0;
	wave_coeff_d = 0;
	wave_coeff_r = 0;
	decomp_filter = 0;
	recon_filter = 0;
	input_buffer = 0;
	output_buffer = 0;
	input_size = 0;
	output_size = 0;
	input_allocation = 0;
	output_allocation = 0;
	dsp_iteration = 0;
	in_scale = 0;
	out_scale = 0;
	dsp_in = 0;
	dsp_out = 0;
	initialized = 0;

	alpha = 1.359803732;
	beta = -0.782106385;
	output_level = 1.0;
	levels = 1;
	iterations = 1;

	PLUGIN_CONSTRUCTOR_MACRO
}

DenoiseEffect::~DenoiseEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
	delete ex_coeff_d;
	delete ex_coeff_r;
	delete ex_coeff_rn;
	delete wave_coeff_d;
	delete wave_coeff_r;
	delete decomp_filter;
	delete recon_filter;
	delete [] input_buffer;
	delete [] output_buffer;
	delete [] dsp_in;
	delete [] dsp_out;
	delete [] dsp_iteration;
}

void DenoiseEffect::reset_plugin()
{
	if(initialized)
	{
		delete ex_coeff_d;
		ex_coeff_d = 0;
		delete ex_coeff_r;
		ex_coeff_r = 0;
		delete ex_coeff_rn;
		ex_coeff_rn = 0;
		delete wave_coeff_d;
		wave_coeff_d = 0;
		delete wave_coeff_r;
		wave_coeff_r = 0;
		delete decomp_filter;
		decomp_filter = 0;
		delete recon_filter;
		recon_filter = 0;
		delete [] input_buffer;
		input_buffer = 0;
		delete [] output_buffer;
		output_buffer = 0;
		delete [] dsp_in;
		dsp_in = 0;
		delete [] dsp_out;
		dsp_out = 0;
		delete [] dsp_iteration;
		dsp_iteration = 0;
		input_size = 0;
		output_size = 0;
		input_allocation = 0;
		output_allocation = 0;
		initialized = 0;
	}
}

PLUGIN_CLASS_METHODS

void DenoiseEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("DENOISE"))
			{
				config.level = input.tag.get_property("LEVEL", config.level);
			}
		}
	}
}

void DenoiseEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("DENOISE");
	output.tag.set_property("LEVEL", config.level);
	output.append_tag();
	output.tag.set_title("/DENOISE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void DenoiseEffect::load_defaults()
{
	defaults = load_defaults_file("denoise.rc");

	config.level = defaults->get("LEVEL", config.level);
}

void DenoiseEffect::save_defaults()
{
	defaults->update("LEVEL", config.level);
	defaults->save();
}

double DenoiseEffect::dot_product(double *data, double *filter, int filtlen)
{
	double sum = 0.0;

	for(int i = 0; i < filtlen; i++)
		sum += *data-- * *filter++;
	return sum;
}

void DenoiseEffect::convolve_dec_2(double *input_sequence, 
	int length,
	double *filter, 
	int filtlen, 
	double *output_sequence)
{
// convolve the input sequence with the filter and decimate by two
	int offset;
	int nearend = length - filtlen;

	for(int i = 0; i < length; i += 2)
	{
		if(i < filtlen)
			*output_sequence++ = dot_product(input_sequence + i,
				filter, i + 1);
		else
		if(i >= nearend)
		{
			offset = i - nearend;
			*output_sequence++ = dot_product(input_sequence + i,
				filter + offset, filtlen - offset);
		}
		else
			*output_sequence++ = dot_product(input_sequence + i, filter, filtlen);
	}
}

void DenoiseEffect::decompose_branches(double *in_data,
	int length, 
	WaveletFilters *decomp_filter, 
	double *out_low, 
	double *out_high)
{
// Take input data and filters and form two branches of half the
// original length. Length of branches is returned.
	convolve_dec_2(in_data, length, decomp_filter->h, decomp_filter->length, out_low);
	convolve_dec_2(in_data, length, decomp_filter->g, decomp_filter->length, out_high);
}

void DenoiseEffect::wavelet_decomposition(double *in_data, 
	int in_length, 
	double **out_data)
{
	for(int i = 0; i < levels; i++)
	{
		decompose_branches(in_data,
			in_length, 
			decomp_filter, 
			out_data[2 * i], 
			out_data[(2 * i) + 1]);

		in_data = out_data[2 * i];
	}
}

void DenoiseEffect::threshold(int window_size, double gammas, int levels)
{
	int i, j;
	double threshold, cv, abs_coeff_r;
	double *coeff_r, *coeff_l;
	int length;
	int sign;

	for(i = 0; i < levels; i++) 
	{
		length = (window_size >> (i + 1));
		threshold = sqrt(2 * log(length) / log(2)) * gammas / sqrt(length);

		for(j = 0; j < length; j++) 
		{
			coeff_r = &(ex_coeff_r->values[(2 * i) + 1][j]);
			coeff_l = &(ex_coeff_rn->values[(2 * i) + 1][j]);

			sign = signbit(*coeff_r);
			abs_coeff_r = fabs(*coeff_r);
			cv = abs_coeff_r - threshold;

			if(sign)
				cv = -cv;

			if(abs_coeff_r > threshold) 
			{
				*coeff_r = cv;
				*coeff_l = 0.0;
			}
			else 
			{
				*coeff_l = *coeff_r;
				*coeff_r = 0.0;
			}
		}
	}
}

double DenoiseEffect::dot_product_even(double *data, double *filter, int filtlen)
{
	double sum = 0.0;

	for(int i = 0; i < filtlen; i += 2)
		sum += *data-- * filter[i];
	return sum;
}

double DenoiseEffect::dot_product_odd(double *data, double *filter, int filtlen)
{
	double sum = 0.0;

	for(int i = 1; i < filtlen; i += 2)
		sum += *data-- * filter[i];
	return sum;
}

void DenoiseEffect::convolve_int_2(double *input_sequence, 
	int length,
	double *filter, 
	int filtlen, 
	int sum_output, 
	double *output_sequence)
// insert zeros between each element of the input sequence and
// convolve with the filter to interpolate the data
{
	int i, j;
	int endpoint = length - filtlen / 2;
	int startpoint = (filtlen / 2) - 1;

	if(sum_output)
	{
// summation with previous convolution
// every other dot product interpolates the data
		for(i = 0, j = 1; i < startpoint; i++, j++)
		{
			*output_sequence++ += dot_product_odd(input_sequence + startpoint,
				filter, filtlen);
			*output_sequence++ += dot_product_even(input_sequence + startpoint +1,
				filter, filtlen);
		}
		for(; i < endpoint; i++, j++)
		{
			*output_sequence++ += dot_product_odd(input_sequence + i,
				filter, filtlen);
			*output_sequence++ += dot_product_even(input_sequence + j,
				filter, filtlen);
		}
		for(; j < length; i++, j++)
		{
			int flen = filtlen - (i - endpoint);

			*output_sequence++ += dot_product_odd(input_sequence + i,
				filter, filtlen);
			*output_sequence++ += dot_product_even(input_sequence + j,
				filter, filtlen);
		}
		*output_sequence++ += dot_product_odd(input_sequence + i, filter, filtlen);
		*output_sequence++ += dot_product_even(input_sequence + i, filter, filtlen);
	}
	else
	{
// first convolution of pair
// every other dot product interpolates the data
		for(i = 0, j = 1; i < startpoint; i++, j++)
		{
			*output_sequence++ = dot_product_odd(input_sequence + startpoint,
				filter, filtlen);
			*output_sequence++ = dot_product_even(input_sequence + startpoint +1,
				filter, filtlen);
		}
		for(; i < endpoint; i++, j++)
		{
			*output_sequence++ = dot_product_odd(input_sequence + i,
				filter, filtlen);
			*output_sequence++ = dot_product_even(input_sequence + j,
				filter, filtlen);
		}
		for(; j < length; i++, j++)
		{
			int flen = filtlen - (i - endpoint);

			*output_sequence++ = dot_product_odd(input_sequence + i,
				filter, filtlen);
			*output_sequence++ = dot_product_even(input_sequence + j,
				filter, filtlen);
		}
		*output_sequence++ = dot_product_odd(input_sequence + i, filter, filtlen);
		*output_sequence++ = dot_product_even(input_sequence + i, filter, filtlen);
	}
}

int DenoiseEffect::reconstruct_branches(double *in_low, 
	double *in_high, 
	int in_length,
	WaveletFilters *recon_filter, 
	double *output)
{
// take input data and filters and form two branches of half the
// original length. length of branches is returned
	convolve_int_2(in_low, in_length, recon_filter->h,
			recon_filter->length, 0, output);
	convolve_int_2(in_high, in_length, recon_filter->g,
			recon_filter->length, 1, output);
	return in_length * 2;
}

void DenoiseEffect::wavelet_reconstruction(double **in_data, 
	int in_length, 
	double *out_data)
{
	double *output;
	int i;

	in_length = in_length >> levels;
// destination of all but last branch reconstruction is the next
// higher intermediate approximation
	for(i = levels - 1; i > 0; i--)
	{
		output = in_data[2 * (i - 1)];
		in_length = reconstruct_branches(in_data[2 * i], 
			in_data[(2 * i) + 1],
			in_length, 
			recon_filter, 
			output);
	}
// destination of the last branch reconstruction is the output data
	reconstruct_branches(in_data[0], 
		in_data[1], 
		in_length, 
		recon_filter, 
		out_data);
}

void DenoiseEffect::process_window()
{
	int i, j;

	for(j = 0; j < iterations; j++)
	{
		wavelet_decomposition(dsp_in, WINDOW_SIZE, ex_coeff_d->values);

		ex_coeff_r->copy_values(ex_coeff_d);
		ex_coeff_rn->copy_values(ex_coeff_d);

// qualify coeffs
		threshold(WINDOW_SIZE, config.level * 10.0, levels);

		wavelet_reconstruction(ex_coeff_r->values, WINDOW_SIZE, dsp_iteration);
		wavelet_reconstruction(ex_coeff_rn->values, WINDOW_SIZE, dsp_in);

		for(i = 0; i < WINDOW_SIZE; i++)
			dsp_out[i] += dsp_iteration[i];
	}
}

AFrame *DenoiseEffect::process_tmpframe(AFrame *input)
{
	int size = input->get_length();
	int new_allocation;

	if(load_configuration())
		update_gui();

	if(!initialized)
	{
		dsp_in = new double[WINDOW_SIZE];
		dsp_out = new double[WINDOW_SIZE];
		dsp_iteration = new double[WINDOW_SIZE];
		ex_coeff_d = new Tree(WINDOW_SIZE, levels);
		ex_coeff_r = new Tree(WINDOW_SIZE, levels);
		ex_coeff_rn = new Tree(WINDOW_SIZE, levels);
		wave_coeff_d = new WaveletCoeffs(alpha, beta);
		wave_coeff_r = new WaveletCoeffs(alpha, beta);
		decomp_filter = new WaveletFilters(wave_coeff_d, DECOMP);
		recon_filter = new WaveletFilters(wave_coeff_r, RECON);
		in_scale = 65535 / sqrt(WINDOW_SIZE) / iterations;
		out_scale = output_level / 65535 * sqrt(WINDOW_SIZE);
		initialized = 1;
	}
	else if(PTSEQU(input->get_pts(), get_start()))
	{
		input_size = 0;
		output_size = 0;
	}

	if(!input_buffer)
		new_allocation = input->get_buffer_length() * 2;
	else
		new_allocation = input_size + size;

	if(new_allocation > input_allocation)
	{
		double *new_input = new double[new_allocation];

		if(input_buffer)
		{
			memcpy(new_input, input_buffer, sizeof(double) * input_size);
			delete [] input_buffer;
		}
		input_buffer = new_input;
		input_allocation = new_allocation;
	}
// Append input buffer
	memcpy(input_buffer + input_size, input->buffer,
		size * sizeof(double));
	input_size += size;

// Have enough to do some windows
	while(input_size >= WINDOW_SIZE)
	{
// Load dsp_in
		for(int i = 0; i < WINDOW_SIZE; i++)
		{
			dsp_in[i] = input_buffer[i] * in_scale;
		}

		memset(dsp_out, 0, sizeof(double) * WINDOW_SIZE);

		process_window();

		if(!output_buffer)
			new_allocation = input->get_buffer_length() * 2;
		else
			new_allocation = output_size + WINDOW_SIZE;

		if(new_allocation > output_allocation)
		{
			double *new_output = new double[new_allocation];

			if(output_buffer)
			{
				memcpy(new_output, output_buffer, sizeof(double) * output_size);
				delete [] output_buffer;
			}
			else
				memset(new_output, 0, new_allocation * sizeof(double));

			output_buffer = new_output;
			output_allocation = new_allocation;
		}

// Crossfade into the output buffer
		if(output_size >= WINDOW_BORDER)
		{
			for(int i = 0, j = output_size - WINDOW_BORDER;
				i < WINDOW_BORDER; i++, j++)
			{
				double src_level = (double)i / WINDOW_BORDER;
				double dst_level = (double)(WINDOW_BORDER - i) / WINDOW_BORDER;
				output_buffer[j] = output_buffer[j] * dst_level +
					out_scale * dsp_out[i] * src_level;
			}

			for(int i = 0; i < WINDOW_SIZE - WINDOW_BORDER; i++)
				output_buffer[output_size + i] =
					dsp_out[WINDOW_BORDER + i] * out_scale;
			output_size += WINDOW_SIZE - WINDOW_BORDER;
		}
		else
		{
// First buffer has no crossfade
			for(int i = 0; i < WINDOW_SIZE; i++)
				output_buffer[output_size + i] =
					dsp_out[i] * out_scale;
			output_size += WINDOW_SIZE;
		}

// Shift input buffer forward
		input_size -= WINDOW_SIZE - WINDOW_BORDER;
		memmove(input_buffer, &input_buffer[WINDOW_SIZE - WINDOW_BORDER],
			input_size * sizeof(double));
	}
// Last frame - copy all to output
	if(PTSEQU(input->get_end_pts(), get_end()))
	{
		memcpy(input->buffer, output_buffer, size * sizeof(double));
		output_size = 0;
	}
	else
	if(output_size - WINDOW_BORDER >= size)
	{
		memcpy(input->buffer, output_buffer, sizeof(double) * size);
		output_size -= size;
		memmove(output_buffer, &output_buffer[size], output_size * sizeof(double));
	}
	else if(output_size > WINDOW_BORDER)
	{
// delay in first frame
		int copy_size = output_size - WINDOW_BORDER;
		int new_start = size - copy_size;
		memset(input->buffer, 0, sizeof(double) * new_start);
		memcpy(&input_buffer[new_start], output_buffer, sizeof(double) * copy_size);
		output_size -= copy_size;
		memmove(output_buffer, &output_buffer[copy_size], output_size * sizeof(double));
	}
	else
		memset(input->buffer, 0, sizeof(double) * size);

	return input;
}


Tree::Tree(int input_length, int levels)
{
	this->input_length = input_length;
	this->levels = levels;
	values_allocated = 2 * levels;

// create decomposition tree
	values = new double*[values_allocated];

	for(int i = 0; i < levels; i++)
	{
		input_length /= 2;

		if(input_length == 0)
			break;

		values[2 * i] = new double[input_length];
		values[2 * i + 1] = new double[input_length];
	}
}

Tree::~Tree()
{
	for(int i = 0; i < values_allocated; i++)
		delete values[i];

	delete values;
}

void Tree::copy_values(Tree *that)
{
	int length = input_length;
	for(int i = 0; i < levels; i++)
	{
		length /= 2;
		int copy_length = length * sizeof(double);

		memcpy(values[2 * i], that->values[2 * i], copy_length);
		memcpy(values[2 * i + 1], that->values[2 * i + 1], copy_length);
	}
}

WaveletCoeffs::WaveletCoeffs(double alpha, double beta)
{
	double tcosa;
	double tcosb;
	double tsina;
	double tsinb;

	sincos(alpha, &tsina, &tcosa);
	sincos(beta, &tsinb, &tcosb);

// calculate first two wavelet coefficients  a = a(-2) and b = a(-1)
	values[0] = ((1.0 + tcosa + tsina) * (1.0 - tcosb - tsinb)
		+ 2.0 * tsinb * tcosa) / 4.0;
	values[1] = ((1.0 - tcosa + tsina) * (1.0 + tcosb - tsinb)
		- 2.0 * tsinb * tcosa) / 4.0;

	sincos(alpha - beta, &tsina, &tcosa);

// calculate last four wavelet coefficients  c = a(0), d = a(1), 
// e = a(2), and f = a(3)
	values[2]  = (1.0 + tcosa + tsina) / 2.0;
	values[3]  = (1.0 + tcosa - tsina) / 2.0;
	values[4]  = 1 - values[0] - values[2];
	values[5]  = 1 - values[1] - values[3];

// zero out very small coefficient values caused by truncation error
	for(int i = 0; i < NUM_WAVELET_COEFFS; i++)
	{
		if(fabs(values[i]) < 1.0e-15)
			values[i] = 0.0;
	}
}


WaveletFilters::WaveletFilters(WaveletCoeffs *wave_coeffs, wavetype transform)
{
	int i, j, k;

// find the first non-zero wavelet coefficient
	for(i = 0; i < NUM_WAVELET_COEFFS && wave_coeffs->values[i] == 0.0 ; i++);

// find the last non-zero wavelet coefficient
	for(j = NUM_WAVELET_COEFFS - 1; j >= 0 && wave_coeffs->values[j] == 0.0; j--);

// Form the decomposition filters h~ and g~ or the reconstruction
// filters h and g.  The division by 2 in the construction
// of the decomposition filters is for normalization.
	length = j - i + 1;
	for(k = 0; k < length; k++)
	{
		if(transform == DECOMP)
		{
			h[k] = wave_coeffs->values[j--] / 2.0;
			g[k] = (double)(((i & 0x01) * 2) - 1) *
				wave_coeffs->values[i] / 2.0;
			i++;
		}
		else
		{
			h[k] = wave_coeffs->values[i++];
			g[k] = (double)(((j & 0x01) * 2) - 1) *
				wave_coeffs->values[j];
			j--;
		}
	}

// clear out the additional array locations, if any
	while (k < NUM_WAVELET_COEFFS)
	{
		h[k] = 0.0;
		g[k++] = 0.0;
	}
}


DenoiseConfig::DenoiseConfig()
{
	level = 1.0;
}

void DenoiseConfig::copy_from(DenoiseConfig &that)
{
	level = that.level;
}

int DenoiseConfig::equivalent(DenoiseConfig &that)
{
	return EQUIV(level, that.level);
}

void DenoiseConfig::interpolate(DenoiseConfig &prev, 
	DenoiseConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->level = prev.level * prev_scale + next.level * next_scale;
}

PLUGIN_THREAD_METHODS

DenoiseWindow::DenoiseWindow(DenoiseEffect *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y, 
	280,
	50)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Level:")));
	x += 70;
	add_subwindow(scale = new DenoiseLevel(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void DenoiseWindow::update()
{
	scale->update(plugin->config.level);
}


DenoiseLevel::DenoiseLevel(DenoiseEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.level, 0, 1.0)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int DenoiseLevel::handle_event()
{
	plugin->config.level = get_value();
	plugin->send_configure_change();
	return 1;
}
