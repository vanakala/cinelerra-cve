// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "clip.h"
#include "bchash.h"
#include "deinterlace.h"
#include "deinterwindow.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>
#include <math.h>

REGISTER_PLUGIN


DeInterlaceConfig::DeInterlaceConfig()
{
	mode = DEINTERLACE_AVG;
	dominance = 0;
	adaptive = 1;
	threshold = 40;
}

int DeInterlaceConfig::equivalent(DeInterlaceConfig &that)
{
	return mode == that.mode &&
		dominance == that.dominance &&
		adaptive == that.adaptive &&
		threshold == that.threshold;
}

void DeInterlaceConfig::copy_from(DeInterlaceConfig &that)
{
	mode = that.mode;
	dominance = that.dominance;
	adaptive = that.adaptive;
	threshold = that.threshold;
}

void DeInterlaceConfig::interpolate(DeInterlaceConfig &prev,
	DeInterlaceConfig &next,
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	copy_from(prev);
}


DeInterlaceMain::DeInterlaceMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

DeInterlaceMain::~DeInterlaceMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS


void DeInterlaceMain::deinterlace_top(VFrame *input, VFrame *output, int dominance)
{
	int w = input->get_w();
	int h = input->get_h();

	for(int i = 0; i < h - 1; i += 2)
	{
		uint16_t *input_row = (uint16_t*)input->get_row_ptr(dominance ? i + 1 : i);
		uint16_t *output_row1 = (uint16_t*)output->get_row_ptr(i);
		uint16_t *output_row2 = (uint16_t*)output->get_row_ptr(i + 1);

		memcpy(output_row1, input_row, w * 4 * sizeof(uint16_t));
		memcpy(output_row2, input_row, w * 4 * sizeof(uint16_t));
	}
}

VFrame *DeInterlaceMain::deinterlace_avg_top(VFrame *input, VFrame *output, int dominance)
{
	VFrame *result;
	int w = input->get_w();
	int h = input->get_h();
	int max_h = h - 1;
	uint64_t abs_diff = 0, total = 0;

	changed_rows = 0;

	for(int i = 0; i < max_h; i += 2)
	{
		int in_number1 = dominance ? i - 1 : i + 0;
		int in_number2 = dominance ? i + 1 : i + 2;
		int out_number1 = dominance ? i - 1 : i;
		int out_number2 = dominance ? i : i + 1;
		in_number1 = MAX(in_number1, 0);
		in_number2 = MIN(in_number2, max_h);
		out_number1 = MAX(out_number1, 0);
		out_number2 = MIN(out_number2, max_h);

		uint16_t *input_row1 = (uint16_t*)input->get_row_ptr(in_number1);
		uint16_t *input_row2 = (uint16_t*)input->get_row_ptr(in_number2);
		uint16_t *input_row3 = (uint16_t*)input->get_row_ptr(out_number2);
		uint16_t *out_row1 = (uint16_t*)output->get_row_ptr(out_number1);
		uint16_t *out_row2 = (uint16_t*)output->get_row_ptr(out_number2);
		int64_t sum = 0;
		int64_t accum_r, accum_b, accum_g, accum_a;

		memcpy(out_row1, input_row1, w * 4 * sizeof(uint16_t));

		for(int j = 0; j < w; j++)
		{
			accum_r = (*input_row1++) + (*input_row2++);
			accum_g = (*input_row1++) + (*input_row2++);
			accum_b = (*input_row1++) + (*input_row2++);
			accum_a = (*input_row1++) + (*input_row2++);

			accum_r /= 2;
			accum_g /= 2;
			accum_b /= 2;
			accum_a /= 2;

			total += *input_row3;
			sum = ((int64_t)*input_row3++) - accum_r;
			abs_diff += (sum < 0 ? -sum : sum);
			*out_row2++ = accum_r;

			total += *input_row3;
			sum = ((int64_t)*input_row3++) - accum_g;
			abs_diff += (sum < 0 ? -sum : sum);
			*out_row2++ = accum_g;

			total += *input_row3;
			sum = ((int64_t)*input_row3++) - accum_b;
			abs_diff += (sum < 0 ? -sum : sum);
			*out_row2++ = accum_b;

			total += *input_row3;
			sum = ((int64_t)*input_row3++) - accum_a;
			abs_diff += (sum < 0 ? -sum : sum);
			*out_row2++ = accum_a;
		}
	}

	int64_t threshold = (int64_t)total * config.threshold / THRESHOLD_SCALAR;

	if(abs_diff > threshold || !config.adaptive)
	{
		result = output;
		changed_rows = 240;
	}
	else
	{
		result = input;
		changed_rows = 0;
	}
	return result;
}

void DeInterlaceMain::deinterlace_avg(VFrame *input, VFrame *output)
{
	int count = input->get_w() * 4;
	int h = input->get_h();

	for(int i = 0; i < h - 1; i += 2)
	{
		uint16_t *input_row1 = (uint16_t*)input->get_row_ptr(i);
		uint16_t *input_row2 = (uint16_t*)input->get_row_ptr(i + 1);
		uint16_t *output_row1 = (uint16_t*)output->get_row_ptr(i);
		uint16_t *output_row2 = (uint16_t*)output->get_row_ptr(i + 1);
		uint16_t result;

		for(int j = 0; j < count; j++)
		{
			result = ((int)input_row1[j] + input_row2[j]) / 2;
			output_row1[j] = result;
			output_row2[j] = result;
		}
	}
}

void DeInterlaceMain::deinterlace_swap(VFrame *input, VFrame *output, int dominance)
{
	int count = input->get_w() * 4;
	int h = input->get_h();

	for(int i = dominance; i < h - 1; i += 2)
	{
		uint16_t *input_row1 = (uint16_t*)input->get_row_ptr(i);
		uint16_t *input_row2 = (uint16_t*)input->get_row_ptr(i + 1);
		uint16_t *output_row1 = (uint16_t*)output->get_row_ptr(i);
		uint16_t *output_row2 = (uint16_t*)output->get_row_ptr(i + 1);
		uint16_t temp1, temp2;

		for(int j = 0; j < count; j++)
		{
			temp1 = input_row1[j];
			temp2 = input_row2[j];
			output_row1[j] = temp2;
			output_row2[j] = temp1;
		}
	}
}

void DeInterlaceMain::deinterlace_temporalswap(VFrame *input, VFrame *prevframe, VFrame *output, int dominance)
{
	int w = input->get_w();
	int h = input->get_h();

	for(int i = 0; i < h - 1; i += 2) 
	{
		uint16_t *input_row1;
		uint16_t *input_row2;
		uint16_t *output_row1 = (uint16_t*)output->get_row_ptr(i);
		uint16_t *output_row2 = (uint16_t*)output->get_row_ptr(i + 1);
		uint16_t temp1, temp2;

		if(dominance)
		{
			input_row1 = (uint16_t*)input->get_row_ptr(i);
			input_row2 = (uint16_t*)prevframe->get_row_ptr(i + 1);
		}
		else
		{
			input_row1 = (uint16_t*)prevframe->get_row_ptr(i);
			input_row2 = (uint16_t*)input->get_row_ptr(i + 1);
		}

		for(int j = 0; j < w * 4; j++)
		{
			temp1 = input_row1[j];
			temp2 = input_row2[j];
			output_row1[j] = temp1;
			output_row2[j] = temp2;
		}
	}
}

// Bob & Weave deinterlacer:
// For each pixel,
//  if it's similar to the previous frame
//    then keep it
//    else average with line above and below
// Similar is defined as in abs(difference)/(sum) < threshold

#define FABS(a) (((a) < 0)?(0 - (a)) : (a))

void DeInterlaceMain::deinterlace_bobweave(VFrame *input, VFrame *prevframe, VFrame *output, int dominance)
{
	int threshold = config.threshold;
	int noise_threshold = 0;
	double exp_threshold = exp(((double)threshold - 50 ) / 2);
	int count = input->get_w() * 4;
	int h = input->get_h();
	uint16_t *row_above=(uint16_t*)input->get_row_ptr(0);

	for(int i = dominance ? 0 : 1; i < h - 1; i += 2)
	{
		uint16_t *input_row;
		uint16_t *input_row2;
		uint16_t *old_row;
		uint16_t *output_row1 = (uint16_t*)output->get_row_ptr(i);
		uint16_t *output_row2 = (uint16_t*)output->get_row_ptr(i + 1);
		int pixel, below, old, above;

		input_row = (uint16_t*)input->get_row_ptr(i);
		input_row2 = (uint16_t*)input->get_row_ptr(i + 1);
		old_row = (uint16_t*)prevframe->get_row_ptr(i);

		for(int j = 0; j < count; j++)
		{
			pixel = input_row[j];
			below = input_row2[j];
			old = old_row[j];
			above = row_above[j];

			if((FABS(pixel - old) <= noise_threshold) ||
					((pixel + old != 0) &&
					(((FABS((double)pixel - old)) / ((double)pixel + old))
						>= exp_threshold )) ||
					((above + below != 0) &&
					(((FABS((double)pixel - old)) / ((double)above + below))
						>= exp_threshold )))
				pixel = (above + below) / 2;
			output_row1[j] = pixel;
			output_row2[j] = below;
		}
		row_above = input_row2;
	}
}

VFrame *DeInterlaceMain::process_tmpframe(VFrame *frame)
{
	VFrame *output = 0;
	VFrame *res;
	VFrame *temp_prevframe = 0;
	int cmodel = frame->get_color_model();

	switch(cmodel)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(cmodel);
		return frame;
	}

	changed_rows = frame->get_h();
	load_configuration();

	switch(config.mode)
	{
	case DEINTERLACE_NONE:
		break;
	case DEINTERLACE_KEEP:
		output = clone_vframe(frame);
		deinterlace_top(frame, output, config.dominance);
		break;
	case DEINTERLACE_AVG:
		output = clone_vframe(frame);
		deinterlace_avg(frame, output);
		break;
	case DEINTERLACE_AVG_1F:
		output = clone_vframe(frame);
		res = deinterlace_avg_top(frame, output, config.dominance);
		if(res == frame)
		{
			frame = output;
			output = res;
		}
		break;
	case DEINTERLACE_SWAP:
		output = clone_vframe(frame);
		deinterlace_swap(frame, output, config.dominance);
		break;
	case DEINTERLACE_BOBWEAVE:
		output = clone_vframe(frame);
		temp_prevframe = clone_vframe(frame);
		temp_prevframe->set_pts(frame->get_pts() - frame->get_duration());
		temp_prevframe = get_frame(temp_prevframe);
		deinterlace_bobweave(frame, temp_prevframe, output, config.dominance);
		break;
	case DEINTERLACE_TEMPORALSWAP:
		output = clone_vframe(frame);
		temp_prevframe = clone_vframe(frame);
		temp_prevframe->set_pts(frame->get_pts() - frame->get_duration());
		temp_prevframe = get_frame(temp_prevframe);
		deinterlace_temporalswap(frame, temp_prevframe, output, config.dominance);
		break;
	}
	release_vframe(temp_prevframe);
	if(output)
	{
		release_vframe(frame);
		frame = output;
	}
	render_gui(&changed_rows);
	return frame;
}

void DeInterlaceMain::render_gui(void *data)
{
	if(thread)
	{
		char string[BCTEXTLEN];
		thread->window->get_status_string(string, changed_rows);
		thread->window->status->update(string);
		thread->window->flush();
	}
}

void DeInterlaceMain::load_defaults()
{
	defaults = load_defaults_file("deinterlace.rc");

	config.mode = defaults->get("MODE", config.mode);
	config.dominance = defaults->get("DOMINANCE", config.dominance);
	config.adaptive = defaults->get("ADAPTIVE", config.adaptive);
	config.threshold = defaults->get("THRESHOLD", config.threshold);
}

void DeInterlaceMain::save_defaults()
{
	defaults->update("MODE", config.mode);
	defaults->update("DOMINANCE", config.dominance);
	defaults->update("ADAPTIVE", config.adaptive);
	defaults->update("THRESHOLD", config.threshold);
	defaults->save();
}

void DeInterlaceMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("DEINTERLACE");
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("DOMINANCE", config.dominance);
	output.tag.set_property("ADAPTIVE", config.adaptive);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.append_tag();
	output.tag.set_title("/DEINTERLACE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void DeInterlaceMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("DEINTERLACE"))
		{
			config.mode = input.tag.get_property("MODE", config.mode);
			config.dominance = input.tag.get_property("DOMINANCE", config.dominance);
			config.adaptive = input.tag.get_property("ADAPTIVE", config.adaptive);
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
		}
	}
}
