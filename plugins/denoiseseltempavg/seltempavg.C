
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

REGISTER_PLUGIN(SelTempAvgMain)


//////////////////////////////////////////////////////////////////////
SelTempAvgConfig::SelTempAvgConfig()
{
	frames = 1;
	method = SelTempAvgConfig::METHOD_SELTEMPAVG;
	offsetmode = SelTempAvgConfig::OFFSETMODE_RESTARTMARKERSYS;
	paranoid = 0;
	nosubtract = 0;
	offset_restartmarker_keyframe = 0;
	offset_fixed_value = -15;
	gain = 1.00;

	avg_threshold_RY = 0; avg_threshold_GU = 0; avg_threshold_BV = 0;
	std_threshold_RY = 0; std_threshold_GU = 0; std_threshold_BV = 0;
	mask_RY = 0; mask_GU = 0; mask_BV = 0;
}

void SelTempAvgConfig::copy_from(SelTempAvgConfig *src)
{
	this->frames = src->frames;
	this->method = src->method;
	this->offsetmode = src->offsetmode;
	this->paranoid = src->paranoid;
	this->nosubtract = src->nosubtract;
	this->offset_restartmarker_keyframe = src->offset_restartmarker_keyframe;
	this->offset_fixed_value = src->offset_fixed_value;
	this->gain = src->gain;
	this->avg_threshold_RY = src->avg_threshold_RY; this->avg_threshold_GU = src->avg_threshold_GU; 
	this->avg_threshold_BV = src->avg_threshold_BV; this->std_threshold_RY = src->std_threshold_RY; 
	this->std_threshold_GU = src->std_threshold_GU; this->std_threshold_BV = src->std_threshold_BV;

	this->mask_BV = src->mask_BV; this->mask_RY = src->mask_RY; this->mask_GU = src->mask_GU;
}

int SelTempAvgConfig::equivalent(SelTempAvgConfig *src)
{
  return frames == src->frames &&
	  method == src->method &&
          offsetmode == src->offsetmode &&
	  paranoid == src->paranoid &&
	  offset_restartmarker_keyframe == src->offset_restartmarker_keyframe &&
	  offset_fixed_value == src->offset_fixed_value &&
	  gain == src->gain &&
	  this->avg_threshold_RY == src->avg_threshold_RY && this->avg_threshold_GU == src->avg_threshold_GU &&
	  this->avg_threshold_BV == src->avg_threshold_BV && this->std_threshold_RY == src->std_threshold_RY &&
	  this->std_threshold_GU == src->std_threshold_GU && this->std_threshold_BV == src->std_threshold_BV &&
	  this->mask_RY == src->mask_RY && this->mask_GU == src->mask_GU && this->mask_BV == src->mask_BV;
}


//////////////////////////////////////////////////////////////////////
SelTempAvgMain::SelTempAvgMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	accumulation = 0;
	history = 0;
	history_size = 0;
	history_start = -0x7fffffff;
	history_frame = 0;
	history_valid = 0;
	prev_frame = -1;
}

SelTempAvgMain::~SelTempAvgMain()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(accumulation) 
	{
		delete [] accumulation;
		delete [] accumulation_sq;
	}
	if(history)
	{
		for(int i = 0; i < config.frames; i++)
			delete history[i];
		delete [] history;
	}
	if(history_frame) delete [] history_frame;
	if(history_valid) delete [] history_valid;
}

char* SelTempAvgMain::plugin_title() { return N_("Selective Temporal Averaging"); }
int SelTempAvgMain::is_realtime() { return 1; }


NEW_PICON_MACRO(SelTempAvgMain)

SHOW_GUI_MACRO(SelTempAvgMain, SelTempAvgThread)

SET_STRING_MACRO(SelTempAvgMain)

RAISE_WINDOW_MACRO(SelTempAvgMain);

int SelTempAvgMain::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	int h = frame->get_h();
	int w = frame->get_w();
	int color_model = frame->get_color_model();
	load_configuration();

// Allocate accumulation
	if(!accumulation)
	{
		accumulation = new unsigned char[w * 
						 h * 
						 cmodel_components(color_model) *
						 sizeof(float)];

		accumulation_sq = new unsigned char[w * 
						    h *
						    3 *
						    sizeof(float)];
		clear_accum(w, h, color_model);
	}

	if(!config.nosubtract)
	{
// Reallocate history
		if(history)
		{
			if(config.frames != history_size)
			{
				VFrame **history2;
				int64_t *history_frame2;
				int *history_valid2;
				history2 = new VFrame*[config.frames];
				history_frame2 = new int64_t[config.frames];
				history_valid2 = new int[config.frames];

// Copy existing frames over
				int i, j;
				for(i = 0, j = 0; i < config.frames && j < history_size; i++, j++)
				{
					history2[i] = history[j];
					history_frame2[i] = history_frame[i];
					history_valid2[i] = history_valid[i];
				}

// Delete extra previous frames and subtract from accumulation
				for( ; j < history_size; j++)
				{
					subtract_accum(history[j]);
					delete history[j];
				}
				delete [] history;
				delete [] history_frame;
				delete [] history_valid;


// Create new frames
				for( ; i < config.frames; i++)
				{
					history2[i] = new VFrame(0, w, h, color_model);
					history_frame2[i] = -0x7fffffff;
					history_valid2[i] = 0;
				}

				history = history2;
				history_frame = history_frame2;
				history_valid = history_valid2;

				history_size = config.frames;
			}
		}
		else
// Allocate history
		{
			history = new VFrame*[config.frames];
			for(int i = 0; i < config.frames; i++)
				history[i] = new VFrame(0, w, h, color_model);
			history_size = config.frames;
			history_frame = new int64_t[config.frames];
			bzero(history_frame, sizeof(int64_t) * config.frames);
			history_valid = new int[config.frames];
			bzero(history_valid, sizeof(int) * config.frames);
		}






// Create new history frames based on current frame
		int64_t *new_history_frames = new int64_t[history_size];

		int64_t theoffset = (int64_t) config.offset_fixed_value;
		if (config.offsetmode == SelTempAvgConfig::OFFSETMODE_RESTARTMARKERSYS)
			theoffset = (int64_t) restartoffset;

		for(int i = 0; i < history_size; i++)
		{
			new_history_frames[history_size - i - 1] = start_position + theoffset + i;
		}

// Subtract old history frames which are not in the new vector
		int no_change = 1;
		for(int i = 0; i < history_size; i++)
		{
// Old frame is valid
			if(history_valid[i])
			{
				int got_it = 0;
				for(int j = 0; j < history_size; j++)
				{
// Old frame is equal to a new frame
					if(history_frame[i] == new_history_frames[j]) 
					{
						got_it = 1;
						break;
					}
				}

// Didn't find old frame in new frames
				if(!got_it)
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
			for(int i = 0; i < history_size; i++)
			{
				history_valid[i] = 0;
			}
			clear_accum(w, h, color_model);
		}

// Add new history frames which are not in the old vector
		for(int i = 0; i < history_size; i++)
		{
// Find new frame in old vector
			int got_it = 0;
			for(int j = 0; j < history_size; j++)
			{
				if(history_valid[j] && history_frame[j] == new_history_frames[i])
				{
					got_it = 1;
					break;
				}
			}

// Didn't find new frame in old vector
			if(!got_it)
			{
// Get first unused entry
				for(int j = 0; j < history_size; j++)
				{
					if(!history_valid[j])
					{
// Load new frame into it
						history_frame[j] = new_history_frames[i];
						history_valid[j] = 1;
						read_frame(history[j],
							0,
							history_frame[j],
							frame_rate);
						add_accum(history[j]);
						break;
					}
				}
			}
		}
		delete [] new_history_frames;
	}
	else
// No subtraction
	{
// Force reload if not repositioned or just started
		if(config.paranoid && prev_frame == start_position ||
			prev_frame < 0)
		{
			prev_frame = start_position - config.frames + 1;
			prev_frame = MAX(0, prev_frame);
			clear_accum(w, h, color_model);
		}

		for(int64_t i = prev_frame; i <= start_position; i++)
		{
			read_frame(frame,
				0,
				i,
				frame_rate);
			add_accum(frame);
//printf("SelTempAvgMain::process_buffer 1 %lld %lld %lld\n", prev_frame, start_position, i);
		}

		prev_frame = start_position;
	}



	// Read current frame into buffer (needed for the std-deviation tool)
	read_frame(frame,
			0,
			start_position,
			frame_rate);


// Transfer accumulation to output with division if average is desired.
	transfer_accum(frame);

//printf("SelTempAvgMain::process_buffer 2\n");


	return 0;
}










// Reset accumulation
#define CLEAR_ACCUM(type, components, chroma) \
{ \
	float *row = (float*)accumulation; \
	float *row_sq = (float*)accumulation_sq; \
	if(chroma) \
	{ \
		for(int i = 0; i < w * h; i++) \
		{ \
			*row++ = 0x0; \
			*row++ = 0x0; \
			*row++ = 0x0; \
			if(components == 4) *row++ = 0x0; \
			*row_sq++ = 0x0; \
			*row_sq++ = 0x0; \
			*row_sq++ = 0x0; \
		} \
	} \
	else \
	{ \
		bzero(row, w * h * sizeof(type) * components); \
		bzero(row_sq, w * h * 3 * sizeof(float)); \
	} \
}


void SelTempAvgMain::clear_accum(int w, int h, int color_model)
{
	switch(color_model)
	{
		case BC_RGB888:
			CLEAR_ACCUM(int, 3, 0x0)
			break;
		case BC_RGB_FLOAT:
			CLEAR_ACCUM(float, 3, 0x0)
			break;
		case BC_RGBA8888:
			CLEAR_ACCUM(int, 4, 0x0)
			break;
		case BC_RGBA_FLOAT:
			CLEAR_ACCUM(float, 4, 0x0)
			break;
		case BC_YUV888:
			CLEAR_ACCUM(int, 3, 0x80)
			break;
		case BC_YUVA8888:
			CLEAR_ACCUM(int, 4, 0x80)
			break;
		case BC_YUV161616:
			CLEAR_ACCUM(int, 3, 0x8000)
			break;
		case BC_YUVA16161616:
			CLEAR_ACCUM(int, 4, 0x8000)
			break;
	}
}
#define C2_IS(frame_row,chroma,max)  (float)(frame_row)/max 


#define SUBTRACT_ACCUM(type, \
	components, \
	chroma, \
	max) \
{ \
	if(1) \
	{ \
		for(int i = 0; i < h; i++) \
		{ \
			float *accum_row = (float*)accumulation + \
				i * w * components; \
			float *accum_row_sq = (float*)accumulation_sq + \
				i * w *3; \
			type *frame_row = (type*)frame->get_rows()[i]; \
			float c1, c2, c3; \
			for(int j = 0; j < w; j++) \
			{ \
				c1 = ( (float)*frame_row )/max; \
				frame_row++; \
                                c2 = ( (float)*frame_row )/max; \
				frame_row++; \
				c3 = ( (float)(*frame_row -0) )/max; \
				frame_row++; \
\
				*accum_row -= c1; \
                                accum_row++; \
				*accum_row -= c2; \
                                accum_row++; \
				*accum_row -= c3; \
                                accum_row++; \
				if(components == 4) { *accum_row -= ((float)*frame_row++)/max; accum_row++; } \
\
				*accum_row_sq++ -= c1*c1; \
				*accum_row_sq++ -= c2*c2; \
				*accum_row_sq++ -= c3*c3; \
			} \
		} \
	} \
}


void SelTempAvgMain::subtract_accum(VFrame *frame)
{
// Just accumulate
	if(config.nosubtract) return;
	int w = frame->get_w();
	int h = frame->get_h();

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
		case BC_YUVA16161616:
			SUBTRACT_ACCUM(uint16_t, 4, 0x8000, 0xffff)
			break;
	}
}


// The behavior has to be very specific to the color model because we rely on
// the value of full black to determine what pixel to show.
#define ADD_ACCUM(type, components, chroma, max) \
{ \
	float c1, c2, c3; \
	if(1) \
	{ \
		for(int i = 0; i < h; i++) \
		{ \
			float *accum_row = (float*)accumulation + \
				i * w * components; \
			float *accum_row_sq = (float*)accumulation_sq + \
				i * w *3; \
			type *frame_row = (type*)frame->get_rows()[i]; \
			for(int j = 0; j < w; j++) \
			{ \
				c1 = ( (float)*frame_row )/max; \
				frame_row++; \
                                c2 = ( (float)*frame_row )/max; \
				frame_row++; \
				c3 = ( (float)*frame_row )/max; \
				frame_row++; \
\
				*accum_row += c1; \
                                accum_row++; \
				*accum_row += c2; \
                                accum_row++; \
				*accum_row += c3; \
                                accum_row++; \
				if(components == 4) { *accum_row += ((float)*frame_row++)/max; accum_row++; } \
\
				*accum_row_sq++ += c1*c1; \
				*accum_row_sq++ += c2*c2; \
				*accum_row_sq++ += c3*c3; \
			} \
		} \
	} \
}


void SelTempAvgMain::add_accum(VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();

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
		case BC_YUVA16161616:
			ADD_ACCUM(uint16_t, 4, 0x8000, 0xffff)
			break;
	}
}

#define MASKER(type, avg_thresh, std_thresh, c_now, c_mean, c_stddev, mask, gain, frame_rowp, max) \
{ \
      if ( (avg_thresh > fabs(c_now - c_mean)) &&  (std_thresh > c_stddev) ) \
	     if (mask) \
                   frame_rowp = max; \
             else \
                   frame_rowp = (type)(c_mean*max*gain); \
      else \
	     if (mask) \
                   frame_rowp = 0; \
             else \
                   frame_rowp = (type)(c_now*max*gain); \
}

#define TRANSFER_ACCUM(type, components, chroma, max, c1_gain, c2_gain, c3_gain) \
{ \
	if(config.method == SelTempAvgConfig::METHOD_SELTEMPAVG) \
	{ \
		float denominator = config.frames; \
		float c1_now, c2_now, c3_now, c4_now; \
		float c1_mean, c2_mean, c3_mean, c4_mean; \
		float c1_stddev, c2_stddev, c3_stddev; \
		for(int i = 0; i < h; i++) \
		{ \
			float *accum_row = (float*)accumulation + i * w * components; \
			float *accum_row_sq = (float*)accumulation_sq + i * w * 3; \
\
			type *frame_row = (type*)frame->get_rows()[i]; \
			for(int j = 0; j < w; j++) \
			{ \
				c1_now = (float)(*frame_row)/max; \
				*frame_row++; \
				c2_now = (float)(*frame_row)/max; \
				*frame_row++; \
				c3_now = (float)(*frame_row)/max; \
				frame_row -= 2; \
\
				c1_mean = *accum_row/denominator; \
                                accum_row++; \
				c2_mean = *accum_row/denominator; \
                                accum_row++; \
				c3_mean = *accum_row/denominator; \
                                accum_row++; \
				if(components == 4) { c4_mean = *accum_row/denominator; accum_row++; } \
\
				c1_stddev = (*accum_row_sq++)/denominator - c1_mean*c1_mean; \
				c2_stddev = (*accum_row_sq++)/denominator - c2_mean*c2_mean; \
				c3_stddev = (*accum_row_sq++)/denominator - c3_mean*c3_mean; \
\
                                MASKER(type,  \
				       config.avg_threshold_RY, \
				       config.std_threshold_RY, \
				       c1_now, c1_mean, c1_stddev, config.mask_RY, c1_gain,\
				       *frame_row++, max)\
                                MASKER(type,  \
				       config.avg_threshold_GU, \
				       config.std_threshold_GU, \
				       c2_now, c2_mean, c2_stddev, config.mask_GU, c2_gain,\
				       *frame_row++, max)\
                                MASKER(type,  \
				       config.avg_threshold_BV, \
				       config.std_threshold_BV, \
				       c3_now, c3_mean, c3_stddev, config.mask_BV, c3_gain,\
				       *frame_row++, max)\
				if(components == 4)  *frame_row++ = max; \
			} \
		} \
	} \
	else \
	if(config.method == SelTempAvgConfig::METHOD_AVERAGE) \
	{ \
		float denominator = config.frames; \
		for(int i = 0; i < h; i++) \
		{ \
			float *accum_row = (float*)accumulation + i * w * components; \
			type *frame_row = (type*)frame->get_rows()[i]; \
			for(int j = 0; j < w; j++) \
			{ \
\
				*frame_row++ = (type)( (*accum_row++ / denominator)*c1_gain*max ); \
				*frame_row++ = (type)( (*accum_row++ / denominator)*c2_gain*max ); \
				*frame_row++ = (type)( (*accum_row++ / denominator)*c3_gain*max ); \
				if(components == 4) *frame_row++ = (type)((*accum_row++/denominator)*max ); \
			} \
		} \
	} \
	else \
	if(config.method == SelTempAvgConfig::METHOD_STDDEV) \
	{ \
		float c1_mean, c2_mean, c3_mean, c4_mean; \
		float c1_stddev, c2_stddev, c3_stddev; \
		float denominator = config.frames; \
		for(int i = 0; i < h; i++) \
		{ \
			float *accum_row = (float*)accumulation + i * w * components; \
			float *accum_row_sq = (float*)accumulation_sq + i * w * 3; \
			type *frame_row = (type*)frame->get_rows()[i]; \
			for(int j = 0; j < w; j++) \
			{ \
\
				c1_mean = *accum_row/denominator; \
                                accum_row++; \
				c2_mean = *accum_row/denominator; \
                                accum_row++; \
				c3_mean = *accum_row/denominator; \
                                accum_row++; \
				if(components == 4) { c4_mean = *accum_row/denominator; accum_row++; } \
\
				c1_stddev = (*accum_row_sq++)/denominator - c1_mean*c1_mean; \
				c2_stddev = (*accum_row_sq++)/denominator - c2_mean*c2_mean; \
				c3_stddev = (*accum_row_sq++)/denominator - c3_mean*c3_mean; \
\
				*frame_row++ = (type)( c1_stddev*c1_gain*max ); \
				*frame_row++ = (type)( c2_stddev*c2_gain*max ); \
				*frame_row++ = (type)( c3_stddev*c3_gain*max ); \
				if(components == 4) *frame_row++ = max; \
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
			TRANSFER_ACCUM(unsigned char, 3  , 0x0   , 0xff  , config.gain, config.gain, config.gain)
			break;
		case BC_RGB_FLOAT:
			TRANSFER_ACCUM(float        , 3  , 0x0   , 1     , config.gain, config.gain, config.gain)
			break;
		case BC_RGBA8888:
			TRANSFER_ACCUM(unsigned char, 4  , 0x0   , 0xff  , config.gain, config.gain, config.gain)
			break;
		case BC_RGBA_FLOAT:
			TRANSFER_ACCUM(float        , 4  , 0x0   , 1     , config.gain, config.gain, config.gain)
			break;
		case BC_YUV888:
			TRANSFER_ACCUM(unsigned char, 3  , 0x80  , 0xff  , config.gain, 1.0        , 1.0)
			break;
		case BC_YUVA8888:
			TRANSFER_ACCUM(unsigned char, 4  , 0x80  , 0xff  , config.gain, 1.0        , 1.0)
			break;
		case BC_YUV161616:
			TRANSFER_ACCUM(uint16_t     , 3  , 0x8000, 0xffff, config.gain, 1.0        , 1.0)
			break;
		case BC_YUVA16161616:
			TRANSFER_ACCUM(uint16_t     , 4  , 0x8000, 0xffff, config.gain, 1.0        , 1.0)
			break;
	}
}


int SelTempAvgMain::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sdenoiseseltempavg.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.frames = defaults->get("FRAMES", config.frames);
	config.method = defaults->get("METHOD", config.method);
	config.offsetmode = defaults->get("OFFSETMODE", config.offsetmode);
	config.paranoid = defaults->get("PARANOID", config.paranoid);
	config.nosubtract = defaults->get("NOSUBTRACT", config.nosubtract);
	config.offset_restartmarker_keyframe = defaults->get("OFFSETMODE_RESTARTMODE_KEYFRAME", config.offset_restartmarker_keyframe);
	config.offset_fixed_value = defaults->get("OFFSETMODE_FIXED_VALUE", config.offset_fixed_value);
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
	return 0;
}

int SelTempAvgMain::save_defaults()
{
	defaults->update("FRAMES", config.frames);
	defaults->update("METHOD", config.method);
	defaults->update("OFFSETMODE", config.offsetmode);
	defaults->update("PARANOID", config.paranoid);
	defaults->update("NOSUBTRACT", config.nosubtract);
	defaults->update("OFFSETMODE_RESTARTMODE_KEYFRAME", config.offset_restartmarker_keyframe);
	defaults->update("OFFSETMODE_FIXED_VALUE", config.offset_fixed_value);
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
	return 0;
}

int SelTempAvgMain::load_configuration()
{
	KeyFrame *prev_keyframe;
	KeyFrame *temp_keyframe;

	SelTempAvgConfig old_config;
	old_config.copy_from(&config);

	int64_t curpos = get_source_position();
	prev_keyframe = get_prev_keyframe(curpos);
	read_data(prev_keyframe);

	if (curpos == prev_keyframe->position) 
		onakeyframe = 1; 
	else 
		onakeyframe = 0;

	int64_t next_restart_keyframe     = curpos + config.frames;
	int64_t prev_restart_keyframe     = curpos - config.frames;

	for (int i = curpos; i < curpos + config.frames; i++) 
	{
		temp_keyframe = get_next_keyframe(i);
		if ( 
			(temp_keyframe->position < curpos + config.frames/2) && 
			(temp_keyframe->position > curpos) &&
			nextkeyframeisoffsetrestart(temp_keyframe) 
			) 
		{
			next_restart_keyframe = temp_keyframe->position; 
			i = curpos + config.frames;
		} else if (temp_keyframe->position > i)
			i = temp_keyframe->position;
	}
	
	for (int i = curpos; i > curpos - config.frames; i--) 
	{
		temp_keyframe = get_prev_keyframe(i);
		if ( 
			(temp_keyframe->position > curpos - config.frames/2) && 
			(temp_keyframe->position < curpos) && 
			nextkeyframeisoffsetrestart(temp_keyframe) 
			) 
		{
			prev_restart_keyframe = temp_keyframe->position; 
			i = curpos - config.frames;
		} else if (temp_keyframe->position < i)
			i = temp_keyframe->position;
	}

	restartoffset = -config.frames/2;
	
	if (onakeyframe && config.offset_restartmarker_keyframe)
		restartoffset = 0;
	else if ((curpos - prev_restart_keyframe) < config.frames/2) 
		restartoffset = prev_restart_keyframe - curpos;
	else if ((next_restart_keyframe - curpos) < config.frames/2) {
		restartoffset = (next_restart_keyframe - curpos) - config.frames;
		// Probably should put another if in here, (when two "restart" keyframes are close together
	}

	return !old_config.equivalent(&config);
}

void SelTempAvgMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("SELECTIVE_TEMPORAL_AVERAGE");
	output.tag.set_property("FRAMES", config.frames);
	output.tag.set_property("METHOD", config.method);
	output.tag.set_property("OFFSETMODE", config.offsetmode);
	output.tag.set_property("PARANOID", config.paranoid);
	output.tag.set_property("NOSUBTRACT", config.nosubtract);
	output.tag.set_property("OFFSETMODE_RESTARTMODE_KEYFRAME", config.offset_restartmarker_keyframe);
	output.tag.set_property("OFFSETMODE_FIXED_VALUE", config.offset_fixed_value);
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
	output.terminate_string();
}

void SelTempAvgMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("SELECTIVE_TEMPORAL_AVERAGE"))
		{
			config.frames = input.tag.get_property("FRAMES", config.frames);
			config.method = input.tag.get_property("METHOD", config.method);
			config.offsetmode = input.tag.get_property("OFFSETMODE", config.offsetmode);
			config.paranoid = input.tag.get_property("PARANOID", config.paranoid);
			config.nosubtract = input.tag.get_property("NOSUBTRACT", config.nosubtract);
			config.offset_restartmarker_keyframe = input.tag.get_property("OFFSETMODE_RESTARTMODE_KEYFRAME", config.offset_restartmarker_keyframe);
			config.offset_fixed_value = input.tag.get_property("OFFSETMODE_FIXED_VALUE", config.offset_fixed_value);
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

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("SELECTIVE_TEMPORAL_AVERAGE"))
		{
			return(input.tag.get_property("OFFSETMODE_RESTARTMODE_KEYFRAME", config.offset_restartmarker_keyframe));
		}
	}
	return (0);
}



void SelTempAvgMain::update_gui()
{
	if(thread) 
	{
		if(load_configuration())
		{
			thread->window->lock_window("SelTempAvgMain::update_gui");
			thread->window->total_frames->update(config.frames);

			thread->window->method_none->update(         config.method == SelTempAvgConfig::METHOD_NONE);
			thread->window->method_seltempavg->update(   config.method == SelTempAvgConfig::METHOD_SELTEMPAVG);
			thread->window->method_average->update(      config.method == SelTempAvgConfig::METHOD_AVERAGE);
			thread->window->method_stddev->update(       config.method == SelTempAvgConfig::METHOD_STDDEV);

			thread->window->offset_fixed->update(        config.offsetmode == SelTempAvgConfig::OFFSETMODE_FIXED);
			thread->window->offset_restartmarker->update(config.offsetmode == SelTempAvgConfig::OFFSETMODE_RESTARTMARKERSYS);


			thread->window->paranoid->update(config.paranoid);
			thread->window->no_subtract->update(config.nosubtract);

			thread->window->offset_fixed_value->update((int64_t)config.offset_fixed_value);
			thread->window->gain->update(config.gain);

			thread->window->avg_threshold_RY->update((float)config.avg_threshold_RY);
			thread->window->avg_threshold_GU->update((float)config.avg_threshold_GU);
			thread->window->avg_threshold_BV->update((float)config.avg_threshold_BV);
			thread->window->std_threshold_RY->update((float)config.std_threshold_RY);
			thread->window->std_threshold_GU->update((float)config.std_threshold_GU);
			thread->window->std_threshold_BV->update((float)config.std_threshold_BV);

			thread->window->mask_RY->update(config.mask_RY);
			thread->window->mask_GU->update(config.mask_GU);
			thread->window->mask_BV->update(config.mask_BV);
			thread->window->unlock_window();
		}
		thread->window->offset_restartmarker_pos->update((int64_t)restartoffset);
		thread->window->offset_restartmarker_keyframe->update((config.offset_restartmarker_keyframe) && (onakeyframe));
	}
}










