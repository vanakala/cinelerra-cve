// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "timeavg.h"
#include "timeavgwindow.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN

TimeAvgConfig::TimeAvgConfig()
{
	duration = 0.04;
	mode = TimeAvgConfig::AVERAGE;
	paranoid = 0;
	nosubtract = 0;
}

void TimeAvgConfig::copy_from(TimeAvgConfig *src)
{
	this->duration = src->duration;
	this->mode = src->mode;
	this->paranoid = src->paranoid;
	this->nosubtract = src->nosubtract;
}

int TimeAvgConfig::equivalent(TimeAvgConfig *src)
{
	return PTSEQU(duration, src->duration) &&
		mode == src->mode &&
		paranoid == src->paranoid &&
		nosubtract == src->nosubtract;
}


TimeAvgMain::TimeAvgMain(PluginServer *server)
 : PluginVClient(server)
{
	accumulation = 0;
	history = 0;
	history_valid = 0;
	prev_frame_pts = -1;
	max_num_frames = 0;
	frames_accum = 0;
	max_denominator = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

TimeAvgMain::~TimeAvgMain()
{
	if(accumulation)
		delete [] accumulation;
	if(history)
	{
		for(int i = 0; i < max_num_frames; i++)
			release_vframe(history[i]);
		delete [] history;
	}
	if(history_valid) delete [] history_valid;
	PLUGIN_DESTRUCTOR_MACRO
}

void TimeAvgMain::reset_plugin()
{
	if(accumulation)
	{
		delete [] accumulation;
		accumulation = 0;
	}
	if(history)
	{
		for(int i = 0; i < max_num_frames; i++)
			release_vframe(history[i]);
		delete [] history;
		history = 0;
		frames_accum = 0;
	}
	if(history_valid)
	{
		delete [] history_valid;
		history_valid = 0;
	}
}

PLUGIN_CLASS_METHODS

VFrame *TimeAvgMain::process_tmpframe(VFrame *frame)
{
	int h = frame->get_h();
	int w = frame->get_w();
	int color_model = frame->get_color_model();
	ptstime history_start, history_end, cpts;
	int got_frame = 0;

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return frame;
	}

	if(load_configuration())
		update_gui();

// Allocate accumulation
	if(!accumulation)
	{
		accumulation = new int[w * h *
			ColorModels::components(color_model)];
		clear_accum(w, h, color_model);
	}

	if(!config.nosubtract)
	{
// Reallocate history
		if(history)
		{
			int new_num_frames = round(config.duration *
				get_project_framerate() * 2);
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

				for(i = 0, j = 0; i < max_num_frames; i++)
				{
					if(history_valid[i])
					{
						history2[j] = history[i];
						history_valid2[j] = 1;
						if(++j >= new_num_frames)
							break;
					} else if(history[i])
						release_vframe(history[i]);
				}

// Delete extra previous frames and subtract from accumulation
				for(; i < max_num_frames; i++)
				{
					if(history_valid[i])
						subtract_accum(history[i]);
					if(history[i])
						release_vframe(history[j]);
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
		history_start = source_pts - config.duration;
		history_end = source_pts;
		if(history_start < get_start())
			history_start = get_start();
// Subtract old history frames which are not in the new vector
		int no_change = 1;
		for(int i = 0; i < max_num_frames; i++)
		{
// Old frame is valid
			if(history_valid[i])
			{
				cpts = history[i]->get_pts();
				if(cpts < history_start || cpts > history_end)
				{
					subtract_accum(history[i]);
					history_valid[i] = 0;
					no_change = 0;
// Correct history_start to frame boundary
					if(cpts < history_start && history[i]->next_pts() > history_start)
						history_start = history[i]->next_pts();
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
// Find new frame in old vector
			int got_it = 0;
			for(int j = 0; j < max_num_frames; j++)
			{
				if(history_valid[j] && history[j]->pts_in_frame(cpts))
				{
					got_it = 1;
					cpts = history[j]->next_pts();
					break;
				}
			}

// Didn't find new frame in old vector
			if(!got_it)
			{
// Get first unused entry
				for(int j = 0; j < max_num_frames; j++)
				{
					if(!history_valid[j])
					{
// Load new frame into it
						if(!history[j])
							history[j] = clone_vframe(frame);
						history[j]->set_pts(cpts);
						history_valid[j] = 1;
						history[j] = get_frame(history[j]);
						add_accum(history[j]);
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

		VFrame *temp_frame = clone_vframe(frame);
		for(cpts = prev_frame_pts;; cpts = temp_frame->next_pts())
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
				temp_frame->set_pts(cpts);
				temp_frame = get_frame(temp_frame);
				add_accum(temp_frame);
			}
		}
		release_vframe(temp_frame);
		prev_frame_pts = frame->get_pts();
	}

	if(!got_frame)
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
	}
// Transfer accumulation to output with division if average is desired.
	transfer_accum(frame);
	return frame;
}

void TimeAvgMain::clear_accum(int w, int h, int color_model)
{
	frames_accum = 0;
	int len = w * h;
	int *row = accumulation;

	switch(color_model)
	{
	case BC_RGBA16161616:
		memset(row, 0, len * sizeof(uint16_t) * 4);
		break;

	case BC_AYUV16161616:
		for(int i = 0; i < w * h; i++)
		{
			*row++ = 0x0;
			*row++ = 0x0;
			*row++ = 0x8000;
			*row++ = 0x8000;
		}
		break;
	}
}

void TimeAvgMain::subtract_accum(VFrame *frame)
{
// Just accumulate
	if(config.nosubtract) return;
	int w = frame->get_w();
	int h = frame->get_h();

	if(--frames_accum < 0)
	{
		clear_accum(w, h, frame->get_color_model());
		return;
	}

	switch(frame->get_color_model())
	{
	case BC_RGBA16161616:
		if(config.mode == TimeAvgConfig::OR)
		{
			for(int i = 0; i < h; i++)
			{
				int *accum_row = accumulation + i * w * 4;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					frame_row += 3;
					if(*frame_row++)
					{
						*accum_row++ = 0;
						*accum_row++ = 0;
						*accum_row++ = 0;
						*accum_row++ = 0;
					}
				}
			}
		}
		else
		{
			for(int i = 0; i < h; i++)
			{
				int *accum_row = accumulation + i * w * 4;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					*accum_row++ -= *frame_row++;
					*accum_row++ -= *frame_row++;
					*accum_row++ -= *frame_row++;
					*accum_row++ -= *frame_row++;
				}
			}
		}
		break;
	case BC_AYUV16161616:
		if(config.mode == TimeAvgConfig::OR)
		{
			for(int i = 0; i < h; i++)
			{
				int *accum_row = accumulation + i * w * 4;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					frame_row += 3;
					if(*frame_row++)
					{
						*accum_row++ = 0;
						*accum_row++ = 0;
						*accum_row++ = 0x8000;
						*accum_row++ = 0x8000;
					}
				}
			}
		}
		else
		{
			for(int i = 0; i < h; i++)
			{
				int *accum_row = (int *)accumulation + i * w * 4;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					*accum_row++ -= *frame_row++;
					*accum_row++ -= *frame_row++;
					*accum_row++ -= (int)*frame_row++ - 0x8000;
					*accum_row++ -= (int)*frame_row++ - 0x8000;
				}
			}
		}
		break;
	}
}

void TimeAvgMain::add_accum(VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();

	frames_accum++;
	switch(frame->get_color_model())
	{
	case BC_RGBA16161616:
		if(config.mode == TimeAvgConfig::OR)
		{
			for(int i = 0; i < h; i++)
			{
				int *accum_row = accumulation + i * w * 4;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					int opacity = frame_row[3];
					int transparency = 0xffff - opacity;

					*accum_row = (opacity * *frame_row + transparency * *accum_row) / 0xffff;
					accum_row++;
					frame_row++;
					*accum_row = (opacity * *frame_row + transparency * *accum_row) / 0xffff;
					accum_row++;
					frame_row++;
					*accum_row = (opacity * *frame_row + transparency * *accum_row) / 0xffff;
					accum_row++;
					frame_row++;
					*accum_row = MAX(*frame_row, *accum_row);
					accum_row++;
					frame_row++;
				}
			}
		}
		else
		{
			for(int i = 0; i < h; i++)
			{
				int *accum_row = accumulation + i * w * 4;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					*accum_row++ += *frame_row++;
					*accum_row++ += *frame_row++;
					*accum_row++ += *frame_row++;
					*accum_row++ += *frame_row++;
				}
			}
		}
		break;

	case BC_AYUV16161616:
		if(config.mode == TimeAvgConfig::OR)
		{
			for(int i = 0; i < h; i++)
			{
				int *accum_row = (int*)accumulation + i * w * 4;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					int opacity = *frame_row;
					int transparency = 0xffff - opacity;
					*accum_row = MAX(*frame_row, *accum_row);
					accum_row++;
					frame_row++;
					*accum_row = (opacity * *frame_row + transparency * *accum_row) / 0xffff;
					accum_row++;
					frame_row++;
					*accum_row = 0x8000 + (opacity * (*frame_row - 0x8000) + transparency * (*accum_row - 0x8000)) / 0xffff;
					accum_row++;
					frame_row++;
					*accum_row = 0x8000 + (opacity * (*frame_row - 0x8000) + transparency * (*accum_row - 0x8000)) / 0xffff;
					accum_row++;
					frame_row++;
				}
			}
		}
		else
		{
			for(int i = 0; i < h; i++)
			{
				int *accum_row = (int*)accumulation + i * w * 4;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					*accum_row++ += *frame_row++;
					*accum_row++ += *frame_row++;
					*accum_row++ += (int)*frame_row++ - 0x8000;
					*accum_row++ += (int)*frame_row++ - 0x8000;
				}
			}
		}
		break;
	}
}

void TimeAvgMain::transfer_accum(VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();

	switch(frame->get_color_model())
	{
	case BC_RGBA16161616:
		if(config.mode == TimeAvgConfig::AVERAGE)
		{
			if(!frames_accum)
				break;

			for(int i = 0; i < h; i++)
			{
				int *accum_row = accumulation + i * w * 4;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					*frame_row++ = *accum_row++ / frames_accum;
					*frame_row++ = *accum_row++ / frames_accum;
					*frame_row++ = *accum_row++ / frames_accum;
					*frame_row++ = *accum_row++ / frames_accum;
				}
			}
		}
		else
		if(config.mode == TimeAvgConfig::ACCUMULATE)
		{
			for(int i = 0; i < h; i++)
			{
				int *accum_row = accumulation + i * w * 4;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					int r = *accum_row++;
					int g = *accum_row++;
					int b = *accum_row++;
					int a = *accum_row++;
					*frame_row++ = CLIP(r, 0, 0xffff);
					*frame_row++ = CLIP(g, 0, 0xffff);
					*frame_row++ = CLIP(b, 0, 0xffff);
					*frame_row++ = CLIP(a, 0, 0xffff);
				}
			}
		}
		else
		{
			for(int i = 0; i < h; i++)
			{
				int *accum_row = accumulation + i * w * 4;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					*frame_row++ = *accum_row++;
					*frame_row++ = *accum_row++;
					*frame_row++ = *accum_row++;
					*frame_row++ = *accum_row++;
				}
			}
		}
		break;

	case BC_AYUV16161616:
		if(config.mode == TimeAvgConfig::AVERAGE)
		{
			if(!frames_accum)
				break;
			for(int i = 0; i < h; i++)
			{
				int *accum_row = (int*)accumulation + i * w * 4;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					*frame_row++ = *accum_row++ / frames_accum;
					*frame_row++ = *accum_row++ / frames_accum;
					*frame_row++ = (*accum_row++ - 0x8000) / frames_accum + 0x8000;
					*frame_row++ = (*accum_row++ - 0x8000) / frames_accum + 0x8000;
				}
			}
		}
		else
		if(config.mode == TimeAvgConfig::ACCUMULATE)
		{
			for(int i = 0; i < h; i++)
			{
				int *accum_row = (int*)accumulation + i * w * 4;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					int a = *accum_row++;
					int r = *accum_row++;
					int g = *accum_row++ + 0x8000;
					int b = *accum_row++ + 0x8000;
					*frame_row++ = CLIP(a, 0, 0xffff);
					*frame_row++ = CLIP(r, 0, 0xffff);
					*frame_row++ = CLIP(g, 0, 0xffff);
					*frame_row++ = CLIP(b, 0, 0xffff);
				}
			}
		}
		else
		{
			for(int i = 0; i < h; i++)
			{
				int *accum_row = (int*)accumulation +i * w * 4;
				uint16_t *frame_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					*frame_row++ = *accum_row++;
					*frame_row++ = *accum_row++;
					*frame_row++ = *accum_row++;
					*frame_row++ = *accum_row++;
				}
			}
		}
		break;
	}
}

void TimeAvgMain::load_defaults()
{
	framenum frames;

	defaults = load_defaults_file("timeavg.rc");

	frames = defaults->get("FRAMES", 0);
	if(frames)
		config.duration = frames / get_project_framerate();
	config.duration = defaults->get("DURATION", config.duration);
	config.mode = defaults->get("MODE", config.mode);
	config.paranoid = defaults->get("PARANOID", config.paranoid);
	config.nosubtract = defaults->get("NOSUBTRACT", config.nosubtract);
}

void TimeAvgMain::save_defaults()
{
	defaults->delete_key("FRAMES");
	defaults->update("DURATION", config.duration);
	defaults->update("MODE", config.mode);
	defaults->update("PARANOID", config.paranoid);
	defaults->update("NOSUBTRACT", config.nosubtract);
	defaults->save();
}

int TimeAvgMain::load_configuration()
{
	KeyFrame *prev_keyframe;
	TimeAvgConfig old_config;
	old_config.copy_from(&config);

	if(!(prev_keyframe = get_prev_keyframe(source_pts)))
		return get_need_reconfigure();

	read_data(prev_keyframe);
	return !old_config.equivalent(&config) || get_need_reconfigure();
}

void TimeAvgMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("TIME_AVERAGE");
	output.tag.set_property("DURATION", config.duration);
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("PARANOID", config.paranoid);
	output.tag.set_property("NOSUBTRACT", config.nosubtract);
	output.append_tag();
	output.tag.set_title("/TIME_AVERAGE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void TimeAvgMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	int frames;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("TIME_AVERAGE"))
		{
			if((frames = input.tag.get_property("FRAMES", 0)) > 0)
				config.duration = frames / get_project_framerate();
			config.duration = input.tag.get_property("DURATION", config.duration);
			config.mode = input.tag.get_property("MODE", config.mode);
			config.paranoid = input.tag.get_property("PARANOID", config.paranoid);
			config.nosubtract = input.tag.get_property("NOSUBTRACT", config.nosubtract);
		}
	}
}
