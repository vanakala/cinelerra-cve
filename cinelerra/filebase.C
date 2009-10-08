
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

#include "asset.h"
#include "assets.h"
#include "byteorder.h"
#include "colormodels.h"
#include "file.h"
#include "filebase.h"
#include "overlayframe.h"
#include "sizes.h"

#include <stdlib.h>

FileBase::FileBase(Asset *asset, File *file)
{
	this->file = file;
	this->asset = asset;
	internal_byte_order = get_byte_order();
	init_ima4();
	reset_parameters();
	overlayer = new OverlayFrame;
}

FileBase::~FileBase()
{
	if(audio_buffer_in) delete [] audio_buffer_in;
	if(audio_buffer_out) delete [] audio_buffer_out;
	if(video_buffer_in) delete [] video_buffer_in;
	if(video_buffer_out) delete [] video_buffer_out;
	if(row_pointers_in) delete [] row_pointers_in;
	if(row_pointers_out) delete [] row_pointers_out;
	if(float_buffer) delete [] float_buffer;
	delete overlayer;
	delete_ima4();
}

int FileBase::close_file()
{
	if(audio_buffer_in) delete [] audio_buffer_in;
	if(audio_buffer_out) delete [] audio_buffer_out;
	if(video_buffer_in) delete [] video_buffer_in;
	if(video_buffer_out) delete [] video_buffer_out;
	if(row_pointers_in) delete [] row_pointers_in;
	if(row_pointers_out) delete [] row_pointers_out;
	if(float_buffer) delete [] float_buffer;
	close_file_derived();
	reset_parameters();
	delete_ima4();
}

int FileBase::set_dither()
{
	dither = 1;
	return 0;
}

int FileBase::reset_parameters()
{
	dither = 0;
	audio_buffer_in = 0;
	video_buffer_in = 0;
	audio_buffer_out = 0;
	video_buffer_out = 0;
	float_buffer = 0;
	row_pointers_in = 0;
	row_pointers_out = 0;
	prev_buffer_position = -1;
	prev_frame_position = -1;
	prev_len = 0;
	prev_bytes = 0;
	prev_track = -1;
	prev_layer = -1;
	ulawtofloat_table = 0;
	floattoulaw_table = 0;
	rd = wr = 0;

	delete_ulaw_tables();
	reset_parameters_derived();
}

int FileBase::get_mode(char *mode, int rd, int wr)
{
	if(rd && !wr) sprintf(mode, "rb");
	else
	if(!rd && wr) sprintf(mode, "wb");
	else
	if(rd && wr)
	{
		int exists = 0;
		FILE *stream;

		if(stream = fopen(asset->path, "rb")) 
		{
			exists = 1; 
			fclose(stream); 
		}

		if(exists) sprintf(mode, "rb+");
		else
		sprintf(mode, "wb+");
	}
}










// ======================================= audio codecs

int FileBase::get_video_buffer(unsigned char **buffer, int depth)
{
// get a raw video buffer for writing or compression by a library
	if(!*buffer)
	{
// Video compression is entirely done in the library.
		int64_t bytes = asset->width * asset->height * depth;
		*buffer = new unsigned char[bytes];
	}
	return 0;
}

int FileBase::get_row_pointers(unsigned char *buffer, unsigned char ***pointers, int depth)
{
// This might be fooled if a new VFrame is created at the same address with a different height.
	if(*pointers && (*pointers)[0] != &buffer[0])
	{
		delete [] *pointers;
		*pointers = 0;
	}

	if(!*pointers)
	{
		*pointers = new unsigned char*[asset->height];
		for(int i = 0; i < asset->height; i++)
		{
			(*pointers)[i] = &buffer[i * asset->width * depth / 8];
		}
	}
}

int FileBase::match4(char *in, char *out)
{
	if(in[0] == out[0] &&
		in[1] == out[1] &&
		in[2] == out[2] &&
		in[3] == out[3])
		return 1;
	else
		return 0;
}

int FileBase::search_render_strategies(ArrayList<int>* render_strategies, int render_strategy)
{
	int i;
	for(i = 0; i < render_strategies->total; i++)
		if(render_strategies->values[i] == render_strategy) return 1;

	return 0;
}
