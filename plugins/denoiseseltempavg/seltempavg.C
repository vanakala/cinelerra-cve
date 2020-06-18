
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

#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "seltempavg.h"
#include "seltempavgwindow.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN


//////////////////////////////////////////////////////////////////////
SelTempAvgConfig::SelTempAvgConfig()
{
	duration = 0.04;
	method = SelTempAvgConfig::METHOD_SELTEMPAVG;
	offsetmode = SelTempAvgConfig::OFFSETMODE_RESTARTMARKERSYS;
	paranoid = 0;
	nosubtract = 0;
	offset_restartmarker_keyframe = 0;
	offset_fixed_pts = -0.5;
	gain = 1.00;

	avg_threshold_RY = 0;
	avg_threshold_GU = 0;
	avg_threshold_BV = 0;
	std_threshold_RY = 0;
	std_threshold_GU = 0;
	std_threshold_BV = 0;
	mask_RY = 0;
	mask_GU = 0;
	mask_BV = 0;
}

void SelTempAvgConfig::copy_from(SelTempAvgConfig *src)
{
	this->duration = src->duration;
	this->method = src->method;
	this->offsetmode = src->offsetmode;
	this->paranoid = src->paranoid;
	this->nosubtract = src->nosubtract;
	this->offset_restartmarker_keyframe = src->offset_restartmarker_keyframe;
	this->offset_fixed_pts = src->offset_fixed_pts;
	this->gain = src->gain;
	this->avg_threshold_RY = src->avg_threshold_RY;
	this->avg_threshold_GU = src->avg_threshold_GU;
	this->avg_threshold_BV = src->avg_threshold_BV;
	this->std_threshold_RY = src->std_threshold_RY;
	this->std_threshold_GU = src->std_threshold_GU;
	this->std_threshold_BV = src->std_threshold_BV;
	this->mask_BV = src->mask_BV;
	this->mask_RY = src->mask_RY;
	this->mask_GU = src->mask_GU;
}

int SelTempAvgConfig::equivalent(SelTempAvgConfig *src)
{
	return PTSEQU(duration, src->duration) &&
		method == src->method &&
		offsetmode == src->offsetmode &&
		paranoid == src->paranoid &&
		offset_restartmarker_keyframe == src->offset_restartmarker_keyframe &&
		EQUIV(offset_fixed_pts, src->offset_fixed_pts) &&
		EQUIV(gain, src->gain) &&
		EQUIV(avg_threshold_RY, src->avg_threshold_RY) &&
		EQUIV(avg_threshold_GU, src->avg_threshold_GU) &&
		EQUIV(avg_threshold_BV, src->avg_threshold_BV) &&
		EQUIV(std_threshold_RY, src->std_threshold_RY) &&
		EQUIV(std_threshold_GU, src->std_threshold_GU) &&
		EQUIV(std_threshold_BV, src->std_threshold_BV) &&
		this->mask_RY == src->mask_RY &&
		this->mask_GU == src->mask_GU &&
		this->mask_BV == src->mask_BV;
}


//////////////////////////////////////////////////////////////////////
SelTempAvgMain::SelTempAvgMain(PluginServer *server)
 : PluginVClient(server)
{
	accumulation = 0;
	history = 0;
	history_valid = 0;
	prev_frame_pts = -1;
	frames_accum = 0;
	max_num_frames = 0;
	max_denominator = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

SelTempAvgMain::~SelTempAvgMain()
{
	if(accumulation) 
	{
		delete [] accumulation;
		delete [] accumulation_sq;
	}
	if(history)
	{
		for(int i = 0; i < max_num_frames; i++)
			if(history[i])
				delete history[i];
		delete [] history;
	}
	if(history_valid)
		delete [] history_valid;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

VFrame *SelTempAvgMain::process_tmpframe(VFrame *frame)
{
	int h = frame->get_h();
	int w = frame->get_w();
	int color_model = frame->get_color_model();
	ptstime history_start, history_end, cpts;
	int got_frame = 0;

	load_configuration();

// Allocate accumulation
	if(!accumulation)
	{
		accumulation = new unsigned char[w * h *
			ColorModels::components(color_model) * sizeof(float)];

		accumulation_sq = new unsigned char[w * h *
			3 * sizeof(float)];
		clear_accum(w, h, color_model);
	}
	if(!config.nosubtract)
	{
// Reallocate history
		if(history)
		{
			int new_num_frames = round(2 * config.duration *
				get_project_framerate());

			if(max_num_frames != new_num_frames)
			{
				VFrame **history2;
				int *history_valid2;

				history2 = new VFrame*[new_num_frames];
				history_valid2 = new int[new_num_frames];
				memset(history2, 0, new_num_frames * sizeof(VFrame*));
				memset(history_valid2, 0, new_num_frames * sizeof(int));
// Copy existing frames over
				int i, j;
				j = 0;
				for(i = 0; i < max_num_frames; i++)
				{
					if(history_valid[i])
					{
						history2[j] = history[i];
						history_valid2[j] = 1;
						if(++j >= new_num_frames)
							break;
					}
					else if(history[i])
						delete history[i];
				}

// Delete extra previous frames and subtract from accumulation
				for(; i < max_num_frames; i++)
				{
					if(history_valid[i])
						subtract_accum(history[i]);
					if(history[i])
						delete history[i];
				}
				delete [] history;
				delete [] history_valid;

				history = history2;
				history_valid = history_valid2;

				max_num_frames = new_num_frames;
			}
		}
		else
// Allocate history
		{
			max_num_frames = round(2 * config.duration *
				get_project_framerate());
			history = new VFrame*[max_num_frames];
			memset(history, 0, max_num_frames * sizeof(VFrame*));

			history_valid = new int[max_num_frames];
			memset(history_valid, 0, sizeof(int) * max_num_frames);
		}

		ptstime theoffset = config.offset_fixed_pts;
		if (config.offsetmode == SelTempAvgConfig::OFFSETMODE_RESTARTMARKERSYS)
			theoffset = restartoffset;
		history_start = source_pts + restartoffset;

		if(history_start < EPSILON)
			history_start = 0;
		history_end = history_start + config.duration;
// Remove frames not in history any more
		int no_change = 1;
		for(int i = 0; i < max_num_frames; i++)
		{
			if(history_valid[i])
			{
				cpts = history[i]->get_pts();
				if(cpts < history_start || cpts > history_end)
				{
					subtract_accum(history[i]);
					history_valid[i] = 0;
					no_change = 0;
				}
			}
		}
// If all frames are still valid, assume tweek occurred upstream and reload.
		if(config.paranoid && no_change)
		{
			for(int i = 0; i < max_num_frames; i++)
				history_valid[i] = 0;
			clear_accum(w, h, color_model);
		}
// Add new history frames which are not in the old vector
		cpts = history_start;
		int got_it = 0;

		for(int i = 0; i < max_num_frames; i++)
		{
			for(int j = 0; j < max_num_frames; j++)
			{
				if(history_valid[j])
				{
					if(history[j]->pts_in_frame(cpts))
					{
						got_it = 1;
						cpts = history[j]->next_pts();
						break;
					}
				}
			}
			if(!got_it)
			{
				for(int j = 0; j < max_num_frames; j++)
				{
					if(!history_valid[j])
					{
						if(!history[j])
							history[j] = new VFrame(0, w, h, color_model);
						history[j]->set_pts(cpts);
						get_frame(history[j]);
						add_accum(history[j]);
						history_valid[j] = 1;
						cpts = history[j]->next_pts();
						break;
					}
				}
			}
			got_it = 0;
			if(cpts >= history_end)
				break;
		}
	}
	else
// No subtraction
	{
// Force reload if not repositioned or just started
		if(config.paranoid && PTSEQU(prev_frame_pts, source_pts) ||
			prev_frame_pts < 0)
		{
			prev_frame_pts = source_pts - config.duration;
			prev_frame_pts = MAX(0, prev_frame_pts);
			clear_accum(w, h, color_model);
			max_denominator = round(config.duration * get_project_framerate());
		} else
			prev_frame_pts = source_pts;

		VFrame *temp_frame = 0;

		for(cpts = prev_frame_pts;;cpts = temp_frame->next_pts())
		{
			if(frames_accum >= max_denominator)
				frames_accum = max_denominator - 1;
			if((source_pts - cpts) < EPSILON)
			{
				got_frame = 1;
				add_accum(frame);
				break;
			}
			else
			{
				if(!temp_frame)
					temp_frame = clone_vframe(frame);
				temp_frame->set_pts(cpts);
				get_frame(temp_frame);
				add_accum(temp_frame);
			}
		}
		release_vframe(temp_frame);
		prev_frame_pts = source_pts;
	}
	if(!got_frame && config.method == SelTempAvgConfig::METHOD_SELTEMPAVG)
	{
		if(frames_accum)
		{
			cpts = frame->get_pts();
			for(int i = 0; i < max_num_frames; i++)
			{
				if(history_valid[i] && history[i]->pts_in_frame(cpts))
				{
					frame->copy_from(history[i]);
					got_frame = 1;
					break;
				}
			}
		}
		if(!got_frame)
		{
			got_frame = 1;
		}
	}
	if(frames_accum && config.method != SelTempAvgConfig::METHOD_NONE)
		transfer_accum(frame);
	return frame;
}

// Reset accumulation
void SelTempAvgMain::clear_accum(int w, int h, int color_model)
{
	frames_accum = 0;
	memset(accumulation, 0,
		w * h * ColorModels::components(color_model) * sizeof(float));
	memset(accumulation_sq, 0, w * h * 3 * sizeof(float));
}

#define C2_IS(frame_row,chroma,max)  (float)(frame_row)/max 

#define SUBTRACT_ACCUM(type, \
	components, \
	chroma, \
	max) \
{ \
	for(int i = 0; i < h; i++) \
	{ \
		float *accum_row = (float*)accumulation + \
			i * w * components; \
		float *accum_row_sq = (float*)accumulation_sq + \
			i * w *3; \
		type *frame_row = (type*)frame->get_row_ptr(i); \
		float c1, c2, c3; \
		for(int j = 0; j < w; j++) \
		{ \
			c1 = ((float)*frame_row) / max; \
				frame_row++; \
			c2 = ((float)*frame_row) / max; \
			frame_row++; \
			c3 = ((float)(*frame_row)) / max; \
			frame_row++; \
\
			*accum_row -= c1; \
			accum_row++; \
			*accum_row -= c2; \
			accum_row++; \
			*accum_row -= c3; \
			accum_row++; \
			if(components == 4) \
			{ \
				*accum_row -= ((float)*frame_row++) / max; \
				accum_row++; \
			} \
\
			*accum_row_sq++ -= c1 * c1; \
			*accum_row_sq++ -= c2 * c2; \
			*accum_row_sq++ -= c3 * c3; \
		} \
	} \
}


void SelTempAvgMain::subtract_accum(VFrame *frame)
{
// Just accumulate
	if(config.nosubtract)
		return;

	int w = frame->get_w();
	int h = frame->get_h();

	if(--frames_accum < 0)
	{
		clear_accum(w, h, frame->get_color_model());
		return;
	}

	switch(frame->get_color_model())
	{
	case BC_RGB888:
		SUBTRACT_ACCUM(unsigned char, 3, 0x0, 0xff)
		break;
	case BC_RGB_FLOAT:
		SUBTRACT_ACCUM(float, 3, 0x0, 1.0)
		break;
	case BC_RGBA8888:
		SUBTRACT_ACCUM(unsigned char, 4, 0x0, 0xff)
		break;
	case BC_RGBA_FLOAT:
		SUBTRACT_ACCUM(float, 4, 0x0, 1.0)
		break;
	case BC_YUV888:
		SUBTRACT_ACCUM(unsigned char, 3, 0x80, 0xff)
		break;
	case BC_YUVA8888:
		SUBTRACT_ACCUM(unsigned char, 4, 0x80, 0xff)
		break;
	case BC_YUV161616:
		SUBTRACT_ACCUM(uint16_t, 3, 0x8000, 0xffff)
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
		SUBTRACT_ACCUM(uint16_t, 4, 0x8000, 0xffff)
		break;
	case BC_AYUV16161616:
		for(int i = 0; i < h; i++) \
		{
			float *accum_row = (float*)accumulation +
				i * w * 4;
			float *accum_row_sq = (float*)accumulation_sq +
				i * w * 3;
			uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);
			float c1, c2, c3, c4;

			for(int j = 0; j < w; j++) \
			{
				c4 = ((float)*frame_row++) / 0xffff;
				c1 = ((float)*frame_row++) / 0xffff;
				c2 = ((float)*frame_row++) / 0xffff;
				c3 = ((float)*frame_row++) / 0xffff;

				*accum_row++ -= c1;
				*accum_row++ -= c2;
				*accum_row++ -= c3;
				*accum_row++ -= c4;

				*accum_row_sq++ -= c1 * c1;
				*accum_row_sq++ -= c2 * c2;
				*accum_row_sq++ -= c3 * c3;
			}
		}
		break;
	}
}

// The behavior has to be very specific to the color model because we rely on
// the value of full black to determine what pixel to show.
#define ADD_ACCUM(type, components, chroma, max) \
{ \
	float c1, c2, c3; \
 \
	for(int i = 0; i < h; i++) \
	{ \
		float *accum_row = (float*)accumulation + \
			i * w * components; \
		float *accum_row_sq = (float*)accumulation_sq + \
			i * w *3; \
		type *frame_row = (type*)frame->get_row_ptr(i); \
		for(int j = 0; j < w; j++) \
		{ \
			c1 = ((float)*frame_row) / max; \
			frame_row++; \
			c2 = ((float)*frame_row) /max; \
			frame_row++; \
			c3 = ((float)*frame_row ) / max; \
			frame_row++; \
\
			*accum_row += c1; \
			accum_row++; \
			*accum_row += c2; \
			accum_row++; \
			*accum_row += c3; \
			accum_row++; \
			if(components == 4) \
			{ \
				*accum_row += ((float)*frame_row++) / max; \
				accum_row++; \
			} \
			*accum_row_sq++ += c1 * c1; \
			*accum_row_sq++ += c2 * c2; \
			*accum_row_sq++ += c3 * c3; \
		} \
	} \
}

void SelTempAvgMain::add_accum(VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();
	frames_accum++;

	switch(frame->get_color_model())
	{
	case BC_RGB888:
		ADD_ACCUM(unsigned char, 3, 0x0, 0xff)
		break;
	case BC_RGB_FLOAT:
		ADD_ACCUM(float, 3, 0x0, 1.0)
		break;
	case BC_RGBA8888:
		ADD_ACCUM(unsigned char, 4, 0x0, 0xff)
		break;
	case BC_RGBA_FLOAT:
		ADD_ACCUM(float, 4, 0x0, 1.0)
		break;
	case BC_YUV888:
		ADD_ACCUM(unsigned char, 3, 0x80, 0xff)
		break;
	case BC_YUVA8888:
		ADD_ACCUM(unsigned char, 4, 0x80, 0xff)
		break;
	case BC_YUV161616:
		ADD_ACCUM(uint16_t, 3, 0x8000, 0xffff)
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
		ADD_ACCUM(uint16_t, 4, 0x8000, 0xffff)
		break;
	case BC_AYUV16161616:
		for(int i = 0; i < h; i++)
		{
			float *accum_row = (float*)accumulation +
				i * w * 4;
			float *accum_row_sq = (float*)accumulation_sq +
				i * w * 3;
			uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);
			for(int j = 0; j < w; j++)
			{
				float c1, c2, c3, c4;
				c4 = ((float)*frame_row++) / 0xffff;
				c1 = ((float)*frame_row++) / 0xffff;
				c2 = ((float)*frame_row++) / 0xffff;
				c3 = ((float)*frame_row++) / 0xffff;

				*accum_row++ += c1;
				*accum_row++ += c2;
				*accum_row++ += c3;
				*accum_row++ += c4;

				*accum_row_sq++ += c1 * c1;
				*accum_row_sq++ += c2 * c2;
				*accum_row_sq++ += c3 * c3;
			}
		}
		break;
	}
}

#define MASKER(type, avg_thresh, std_thresh, c_now, c_mean, c_stddev, mask, gain, frame_rowp, max) \
{ \
	if((avg_thresh > fabs(c_now - c_mean)) &&  (std_thresh > c_stddev)) \
		if(mask) \
			frame_rowp = max; \
		else \
			frame_rowp = (type)(c_mean * max * gain); \
	else \
	if(mask) \
		frame_rowp = 0; \
	else \
		frame_rowp = (type)(c_now * max * gain); \
}

#define TRANSFER_ACCUM(type, components, chroma, max, c1_gain, c2_gain, c3_gain) \
{ \
	if(config.method == SelTempAvgConfig::METHOD_SELTEMPAVG) \
	{ \
		float denominator = frames_accum; \
		float c1_now, c2_now, c3_now, c4_now; \
		float c1_mean, c2_mean, c3_mean, c4_mean; \
		float c1_stddev, c2_stddev, c3_stddev; \
 \
		for(int i = 0; i < h; i++) \
		{ \
			float *accum_row = (float*)accumulation + i * w * components; \
			float *accum_row_sq = (float*)accumulation_sq + i * w * 3; \
\
			type *frame_row = (type*)frame->get_row_ptr(i); \
			for(int j = 0; j < w; j++) \
			{ \
				c1_now = (float)(*frame_row) / max; \
				*frame_row++; \
				c2_now = (float)(*frame_row) / max; \
				*frame_row++; \
				c3_now = (float)(*frame_row) / max; \
				frame_row -= 2; \
\
				c1_mean = *accum_row / denominator; \
				accum_row++; \
				c2_mean = *accum_row / denominator; \
				accum_row++; \
				c3_mean = *accum_row / denominator; \
				accum_row++; \
				if(components == 4) \
				{ \
					c4_mean = *accum_row / denominator; \
					accum_row++; \
				} \
\
				c1_stddev = (*accum_row_sq++) / denominator - \
					c1_mean * c1_mean; \
				c2_stddev = (*accum_row_sq++) / denominator - \
					c2_mean*c2_mean; \
				c3_stddev = (*accum_row_sq++) / denominator - \
					c3_mean*c3_mean; \
\
				MASKER(type,  \
					config.avg_threshold_RY, \
					config.std_threshold_RY, \
					c1_now, c1_mean, \
					c1_stddev, config.mask_RY, c1_gain,\
					*frame_row++, max)\
				MASKER(type,  \
					config.avg_threshold_GU, \
					config.std_threshold_GU, \
					c2_now, c2_mean, c2_stddev, \
					config.mask_GU, c2_gain,\
					*frame_row++, max)\
				MASKER(type,  \
					config.avg_threshold_BV, \
					config.std_threshold_BV, \
					c3_now, c3_mean, c3_stddev, \
					config.mask_BV, c3_gain,\
					*frame_row++, max)\
				if(components == 4) \
					*frame_row++ = max; \
			} \
		} \
	} \
	else \
	if(config.method == SelTempAvgConfig::METHOD_AVERAGE) \
	{ \
		float denominator = frames_accum; \
		for(int i = 0; i < h; i++) \
		{ \
			float *accum_row = (float*)accumulation + i * w * components; \
			type *frame_row = (type*)frame->get_row_ptr(i); \
 \
			for(int j = 0; j < w; j++) \
			{ \
				*frame_row++ = (type)((*accum_row++ / denominator) * \
					c1_gain * max); \
				*frame_row++ = (type)((*accum_row++ / denominator) * \
					c2_gain * max); \
				*frame_row++ = (type)((*accum_row++ / denominator) * \
					c3_gain * max); \
				if(components == 4) \
					*frame_row++ = (type)((*accum_row++ / \
						denominator) * max ); \
			} \
		} \
	} \
	else \
	if(config.method == SelTempAvgConfig::METHOD_STDDEV) \
	{ \
		float c1_mean, c2_mean, c3_mean, c4_mean; \
		float c1_stddev, c2_stddev, c3_stddev; \
		float denominator = frames_accum; \
 \
		for(int i = 0; i < h; i++) \
		{ \
			float *accum_row = (float*)accumulation + i * w * components; \
			float *accum_row_sq = (float*)accumulation_sq + i * w * 3; \
			type *frame_row = (type*)frame->get_row_ptr(i); \
			for(int j = 0; j < w; j++) \
			{ \
\
				c1_mean = *accum_row / denominator; \
				accum_row++; \
				c2_mean = *accum_row / denominator; \
				accum_row++; \
				c3_mean = *accum_row / denominator; \
				accum_row++; \
				if(components == 4) \
				{ \
					c4_mean = *accum_row / denominator; \
					accum_row++; \
				} \
\
				c1_stddev = (*accum_row_sq++) / denominator - \
					c1_mean * c1_mean; \
				c2_stddev = (*accum_row_sq++) / denominator - \
					c2_mean * c2_mean; \
				c3_stddev = (*accum_row_sq++) / denominator - \
					c3_mean * c3_mean; \
\
				*frame_row++ = (type)(c1_stddev * c1_gain * max ); \
				*frame_row++ = (type)(c2_stddev * c2_gain * max ); \
				*frame_row++ = (type)(c3_stddev * c3_gain * max ); \
				if(components == 4) \
					*frame_row++ = max; \
			} \
		} \
	} \
}

void SelTempAvgMain::transfer_accum(VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();

	switch(frame->get_color_model())
	{
	case BC_RGB888:
		TRANSFER_ACCUM(unsigned char, 3, 0x0, 0xff,
			config.gain, config.gain, config.gain)
		break;
	case BC_RGB_FLOAT:
		TRANSFER_ACCUM(float, 3, 0x0, 1,
			config.gain, config.gain, config.gain)
		break;
	case BC_RGBA8888:
		TRANSFER_ACCUM(unsigned char, 4, 0x0, 0xff,
			config.gain, config.gain, config.gain)
		break;
	case BC_RGBA_FLOAT:
		TRANSFER_ACCUM(float, 4, 0x0, 1,
			config.gain, config.gain, config.gain)
		break;
	case BC_RGBA16161616:
		TRANSFER_ACCUM(uint16_t, 4, 0x0, 0xffff,
			config.gain, config.gain, config.gain)
		break;
	case BC_YUV888:
		TRANSFER_ACCUM(unsigned char, 3, 0x80, 0xff,
			config.gain, 1.0, 1.0)
		break;
	case BC_YUVA8888:
		TRANSFER_ACCUM(unsigned char, 4, 0x80, 0xff,
			config.gain, 1.0, 1.0)
		break;
	case BC_YUV161616:
		TRANSFER_ACCUM(uint16_t, 3, 0x8000, 0xffff,
			config.gain, 1.0, 1.0)
		break;
	case BC_YUVA16161616:
		TRANSFER_ACCUM(uint16_t, 4, 0x8000, 0xffff,
			config.gain, 1.0, 1.0)
		break;
	case BC_AYUV16161616:
		if(config.method == SelTempAvgConfig::METHOD_SELTEMPAVG)
		{
			float denominator = frames_accum;
			float c1_now, c2_now, c3_now, c4_now;
			float c1_mean, c2_mean, c3_mean, c4_mean;
			float c1_stddev, c2_stddev, c3_stddev;

			for(int i = 0; i < h; i++)
			{
				float *accum_row = (float*)accumulation + i * w * 4;
				float *accum_row_sq = (float*)accumulation_sq + i * w * 3;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					*frame_row++ = 0xffff;
					c1_now = (float)(frame_row[0]) / 0xffff;
					c2_now = (float)(frame_row[1]) / 0xffff;
					c3_now = (float)(frame_row[2]) / 0xffff;

					c1_mean = *accum_row++ / denominator;
					c2_mean = *accum_row++ / denominator;
					c3_mean = *accum_row++ / denominator;
					c4_mean = *accum_row++ / denominator;

					c1_stddev = (*accum_row_sq++) / denominator -
						c1_mean * c1_mean;
					c2_stddev = (*accum_row_sq++) / denominator -
						c2_mean * c2_mean;
					c3_stddev = (*accum_row_sq++) / denominator -
						c3_mean * c3_mean;
\
					MASKER(uint16_t,
						config.avg_threshold_RY,
						config.std_threshold_RY,
						c1_now, c1_mean, c1_stddev,
						config.mask_RY, config.gain,
						*frame_row++, 0xffff)
					MASKER(uint16_t,
						config.avg_threshold_GU,
						config.std_threshold_GU,
						c2_now, c2_mean, c2_stddev,
						config.mask_GU, 1.0,
						*frame_row++, 0xffff)
					MASKER(uint16_t,
						config.avg_threshold_BV,
						config.std_threshold_BV,
						c3_now, c3_mean, c3_stddev,
						config.mask_BV, 1.0,
						*frame_row++, 0xffff)
				}
			}
		}
		else
		if(config.method == SelTempAvgConfig::METHOD_AVERAGE)
		{
			float denominator = frames_accum;

			for(int i = 0; i < h; i++)
			{
				float *accum_row = (float*)accumulation + i * w * 4;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					*frame_row++ = (uint16_t)((accum_row[3] /
						denominator) * 0xffff );
					*frame_row++ = (uint16_t)((*accum_row++ /
						denominator) * config.gain * 0xffff);
					*frame_row++ = (uint16_t)((*accum_row++ /
						denominator) * 1 * 0xffff);
					*frame_row++ = (uint16_t)((*accum_row++ /
						denominator) * 1 * 0xffff);
					accum_row++;
				}
			}
		}
		else
		if(config.method == SelTempAvgConfig::METHOD_STDDEV)
		{
			float c1_mean, c2_mean, c3_mean, c4_mean;
			float c1_stddev, c2_stddev, c3_stddev;
			float denominator = frames_accum;

			for(int i = 0; i < h; i++)
			{
				float *accum_row = (float*)accumulation + i * w * 4;
				float *accum_row_sq = (float*)accumulation_sq + i * w * 3;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					c1_mean = *accum_row++ / denominator;
					c2_mean = *accum_row++ / denominator;
					c3_mean = *accum_row++ / denominator;
					c4_mean = *accum_row++ / denominator;

					c1_stddev = *accum_row_sq++ / denominator -
						c1_mean * c1_mean;
					c2_stddev = *accum_row_sq++ / denominator -
						c2_mean * c2_mean;
					c3_stddev = *accum_row_sq++ / denominator -
						c3_mean * c3_mean;

					*frame_row++ = 0xffff;
					*frame_row++ = (uint16_t)(c1_stddev *
						config.gain * 0xffff);
					*frame_row++ = (uint16_t)(c2_stddev * 1 * 0xffff);
					*frame_row++ = (uint16_t)(c3_stddev * 1 * 0xffff);
				}
			}
		}
		break;
	}
}

void SelTempAvgMain::load_defaults()
{
	framenum frames;
	defaults = load_defaults_file("denoiseseltempavg.rc");

	// backward compatibility
	frames = defaults->get("FRAMES", 0);
	if(frames)
		config.duration = frames / get_project_framerate();
	config.duration = defaults->get("DURATION", config.duration);
	config.method = defaults->get("METHOD", config.method);
	config.offsetmode = defaults->get("OFFSETMODE", config.offsetmode);
	config.paranoid = defaults->get("PARANOID", config.paranoid);
	config.nosubtract = defaults->get("NOSUBTRACT", config.nosubtract);
	config.offset_restartmarker_keyframe = defaults->get("OFFSETMODE_RESTARTMODE_KEYFRAME", config.offset_restartmarker_keyframe);
	frames = defaults->get("OFFSETMODE_FIXED_VALUE", 0);
	if(frames)
		config.offset_fixed_pts = frames / get_project_framerate();
	config.offset_fixed_pts = defaults->get("OFFSETMODE_FIXED_PTS", config.offset_fixed_pts);
	config.gain = defaults->get("GAIN", config.gain);

	config.avg_threshold_RY = defaults->get("AVG_THRESHOLD_RY", config.avg_threshold_GU); 
	config.avg_threshold_GU = defaults->get("AVG_THRESHOLD_GU", config.avg_threshold_GU); 
	config.avg_threshold_BV = defaults->get("AVG_THRESHOLD_BV", config.avg_threshold_BV); 
	config.std_threshold_RY = defaults->get("STD_THRESHOLD_RY", config.std_threshold_RY); 
	config.std_threshold_GU = defaults->get("STD_THRESHOLD_GU", config.std_threshold_GU); 
	config.std_threshold_BV = defaults->get("STD_THRESHOLD_BV", config.std_threshold_BV);
	config.mask_RY = defaults->get("MASK_RY", config.mask_GU); 
	config.mask_GU = defaults->get("MASK_GU", config.mask_GU); 
	config.mask_BV = defaults->get("MASK_BV", config.mask_BV); 
}

void SelTempAvgMain::save_defaults()
{
	defaults->delete_key("FRAMES");
	defaults->update("DURATION", config.duration);
	defaults->update("METHOD", config.method);
	defaults->update("OFFSETMODE", config.offsetmode);
	defaults->update("PARANOID", config.paranoid);
	defaults->update("NOSUBTRACT", config.nosubtract);
	defaults->update("OFFSETMODE_RESTARTMODE_KEYFRAME", config.offset_restartmarker_keyframe);
	defaults->delete_key("OFFSETMODE_FIXED_VALUE");
	defaults->update("OFFSETMODE_FIXED_PTS", config.offset_fixed_pts);
	defaults->update("GAIN", config.gain);

	defaults->update("AVG_THRESHOLD_RY", config.avg_threshold_RY); 
	defaults->update("AVG_THRESHOLD_GU", config.avg_threshold_GU);
	defaults->update("AVG_THRESHOLD_BV", config.avg_threshold_BV); 
	defaults->update("STD_THRESHOLD_RY", config.std_threshold_RY);
	defaults->update("STD_THRESHOLD_GU", config.std_threshold_GU); 
	defaults->update("STD_THRESHOLD_BV", config.std_threshold_BV);

	defaults->update("MASK_RY", config.mask_RY);
	defaults->update("MASK_GU", config.mask_GU); 
	defaults->update("MASK_BV", config.mask_BV);

	defaults->save();
}

int SelTempAvgMain::load_configuration()
{
// See tuleb ringi teha
	KeyFrame *prev_keyframe;
	KeyFrame *temp_keyframe;

	SelTempAvgConfig old_config;
	old_config.copy_from(&config);

	if(!(prev_keyframe = prev_keyframe_pts(source_pts)))
		return 0;

	read_data(prev_keyframe);

	if(PTSEQU(source_pts, prev_keyframe->pos_time))
	{
		onakeyframe = 1;
		if(config.offset_restartmarker_keyframe)
		{
			restartoffset = 0;
			return !old_config.equivalent(&config);
		}
	}
	else
		onakeyframe = 0;

	ptstime next_restart_pts = source_pts + config.duration;
	ptstime prev_restart_pts = source_pts - config.duration;

	temp_keyframe = next_keyframe_pts(source_pts);
	if(temp_keyframe->pos_time > source_pts 
			&& temp_keyframe->pos_time < source_pts + config.duration / 2
			&& nextkeyframeisoffsetrestart(temp_keyframe))
		next_restart_pts = temp_keyframe->pos_time;

	if(prev_keyframe->pos_time < source_pts
			&& prev_keyframe->pos_time > source_pts - config.duration / 2
			&& nextkeyframeisoffsetrestart(temp_keyframe))
		prev_restart_pts = prev_keyframe->pos_time;

	restartoffset = -config.duration / 2;

	if ((source_pts - prev_restart_pts) < config.duration / 2)
		restartoffset = prev_restart_pts - source_pts;
	else if ((next_restart_pts - source_pts) < config.duration / 2) {
		restartoffset = (next_restart_pts - source_pts) - config.duration;
		// Probably should put another if in here, (when two "restart" keyframes are close together
	}

	return !old_config.equivalent(&config);
}

void SelTempAvgMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("SELECTIVE_TEMPORAL_AVERAGE");
	output.tag.set_property("DURATION", config.duration);
	output.tag.set_property("METHOD", config.method);
	output.tag.set_property("OFFSETMODE", config.offsetmode);
	output.tag.set_property("PARANOID", config.paranoid);
	output.tag.set_property("NOSUBTRACT", config.nosubtract);
	output.tag.set_property("OFFSETMODE_RESTARTMODE_KEYFRAME", config.offset_restartmarker_keyframe);
	output.tag.set_property("OFFSETMODE_FIXED_PTS", config.offset_fixed_pts);
	output.tag.set_property("GAIN", config.gain);

	output.tag.set_property("AVG_THRESHOLD_RY", config.avg_threshold_RY); 
	output.tag.set_property("AVG_THRESHOLD_GU", config.avg_threshold_GU); 
	output.tag.set_property("AVG_THRESHOLD_BV", config.avg_threshold_BV); 
	output.tag.set_property("STD_THRESHOLD_RY", config.std_threshold_RY); 
	output.tag.set_property("STD_THRESHOLD_GU", config.std_threshold_GU); 
	output.tag.set_property("STD_THRESHOLD_BV", config.std_threshold_BV);

	output.tag.set_property("MASK_RY", config.mask_RY); 
	output.tag.set_property("MASK_GU", config.mask_GU); 
	output.tag.set_property("MASK_BV", config.mask_BV);

	output.append_tag();
	output.tag.set_title("/SELECTIVE_TEMPORAL_AVERAGE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void SelTempAvgMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	framenum frames;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("SELECTIVE_TEMPORAL_AVERAGE"))
		{
			frames = input.tag.get_property("FRAMES", 0);
			if(frames)
				config.duration = frames / get_project_framerate();
			config.duration = input.tag.get_property("DURATION", config.duration);
			config.method = input.tag.get_property("METHOD", config.method);
			config.offsetmode = input.tag.get_property("OFFSETMODE", config.offsetmode);
			config.paranoid = input.tag.get_property("PARANOID", config.paranoid);
			config.nosubtract = input.tag.get_property("NOSUBTRACT", config.nosubtract);
			config.offset_restartmarker_keyframe = input.tag.get_property("OFFSETMODE_RESTARTMODE_KEYFRAME", config.offset_restartmarker_keyframe);
			frames = input.tag.get_property("OFFSETMODE_FIXED_VALUE", 0);
			if(frames)
				config.offset_fixed_pts = frames / get_project_framerate();
			config.offset_fixed_pts = input.tag.get_property("OFFSETMODE_FIXED_PTS", config.offset_fixed_pts);
			config.gain = input.tag.get_property("gain", config.gain);

			config.avg_threshold_RY = input.tag.get_property("AVG_THRESHOLD_RY", config.avg_threshold_RY); 
			config.avg_threshold_GU = input.tag.get_property("AVG_THRESHOLD_GU", config.avg_threshold_GU); 
			config.avg_threshold_BV = input.tag.get_property("AVG_THRESHOLD_BV", config.avg_threshold_BV); 
			config.std_threshold_RY = input.tag.get_property("STD_THRESHOLD_RY", config.std_threshold_RY); 
			config.std_threshold_GU = input.tag.get_property("STD_THRESHOLD_GU", config.std_threshold_GU); 
			config.std_threshold_BV = input.tag.get_property("STD_THRESHOLD_BV", config.std_threshold_BV);

			config.mask_RY = input.tag.get_property("MASK_RY", config.mask_RY); 
			config.mask_GU = input.tag.get_property("MASK_GU", config.mask_GU); 
			config.mask_BV = input.tag.get_property("MASK_BV", config.mask_BV);
		}
	}
}

int SelTempAvgMain::nextkeyframeisoffsetrestart(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("SELECTIVE_TEMPORAL_AVERAGE"))
		{
			return(input.tag.get_property("OFFSETMODE_RESTARTMODE_KEYFRAME", 0));
		}
	}
	return 0;
}
