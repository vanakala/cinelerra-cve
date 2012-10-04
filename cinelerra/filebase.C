
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
#include "aframe.h"
#include "byteorder.h"
#include "colormodels.h"
#include "file.h"
#include "filebase.h"
#include "overlayframe.h"

#include <stdlib.h>

FileBase::FileBase(Asset *asset, File *file)
{
	this->file = file;
	this->asset = asset;
	internal_byte_order = get_byte_order();
	reset_parameters();
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
}

void FileBase::close_file()
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
}

void FileBase::set_dither()
{
	dither = 1;
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
	prev_len = 0;
	prev_bytes = 0;
	prev_track = -1;
	prev_layer = -1;
	rd = wr = 0;
	reset_parameters_derived();
}


int FileBase::match4(const char *in, const char *out)
{
	if(in[0] == out[0] &&
		in[1] == out[1] &&
		in[2] == out[2] &&
		in[3] == out[3])
		return 1;
	else
		return 0;
}

int FileBase::write_aframes(AFrame **aframes)
{
	double *samples[MAX_CHANNELS];
	int len = 0;

	for(int i = 0; i < asset->channels; i++)
	{
		if(aframes[i])
		{
			samples[i] = aframes[i]->buffer;
			len = aframes[i]->length;
		}
		else
			samples[i] = 0;
	}
	return(write_samples(samples, len));
}
