
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
#include "clip.h"
#include "fileac3.h"
#include "language.h"
#include "mwindow.inc"
#include "mainerror.h"



#include <string.h>

FileAC3::FileAC3(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset_parameters();
}

FileAC3::~FileAC3()
{
	close_file();
}

int FileAC3::reset_parameters_derived()
{
	codec = 0;
	codec_context = 0;
	fd = 0;
	temp_raw = 0;
	temp_raw_size = 0;
	temp_raw_allocated = 0;
	temp_compressed = 0;
	compressed_allocated = 0;
}

void FileAC3::get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options)
{
	if(audio_options)
	{

		AC3ConfigAudio *window = new AC3ConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}

int FileAC3::check_sig()
{
// Libmpeg3 does this in FileMPEG.
	return 0;
}

int FileAC3::open_file(int rd, int wr)
{
	this->wr = wr;
	this->rd = rd;


	if(wr)
	{
  		avcodec_init();
		avcodec_register_all();
		codec = avcodec_find_encoder(CODEC_ID_AC3);
		if(!codec)
		{
			eprintf("codec not found.\n");
			return 1;
		}
		codec_context = avcodec_alloc_context();
		codec_context->bit_rate = asset->ac3_bitrate * 1000;
		codec_context->sample_rate = asset->sample_rate;
		codec_context->channels = asset->channels;
		if(avcodec_open(codec_context, codec))
		{
			eprintf("failed to open codec.\n");
			return 1;
		}

		if(!(fd = fopen(asset->path, "w")))
		{
			eprintf("Error while opening \"%s\" for writing. \n%m\n", asset->path);
			return 1;
		}
	}
	else
	{
		if(!(fd = fopen(asset->path, "r")))
		{
			eprintf("Error while opening \"%s\" for reading. \n%m\n", asset->path);
			return 1;
		}
	}




	return 0;
}

int FileAC3::close_file()
{
	if(codec_context)
	{
		avcodec_close(codec_context);
		free(codec_context);
		codec_context = 0;
		codec = 0;
	}
	if(fd)
	{
		fclose(fd);
		fd = 0;
	}
	if(temp_raw)
	{
		delete [] temp_raw;
		temp_raw = 0;
	}
	if(temp_compressed)
	{
		delete [] temp_compressed;
		temp_compressed = 0;
	}
	reset_parameters();
	FileBase::close_file();
}

// Channel conversion matrices because ffmpeg encodes a
// different channel order than liba52 decodes.
// Each row is an output channel.
// Each column is an input channel.
// static int channels5[] = 
// {
// 	{ }
// };
// 
// static int channels6[] = 
// {
// 	{ }
// };


int FileAC3::write_samples(double **buffer, int64_t len)
{
// Convert buffer to encoder format
	if(temp_raw_size + len > temp_raw_allocated)
	{
		int new_allocated = temp_raw_size + len;
		int16_t *new_raw = new int16_t[new_allocated * asset->channels];
		if(temp_raw)
		{
			memcpy(new_raw, 
				temp_raw, 
				sizeof(int16_t) * temp_raw_size * asset->channels);
			delete [] temp_raw;
		}
		temp_raw = new_raw;
		temp_raw_allocated = new_allocated;
	}

// Allocate compressed data buffer
	if(temp_raw_allocated * asset->channels * 2 > compressed_allocated)
	{
		compressed_allocated = temp_raw_allocated * asset->channels * 2;
		delete [] temp_compressed;
		temp_compressed = new unsigned char[compressed_allocated];
	}

// Append buffer to temp raw
	int16_t *out_ptr = temp_raw + temp_raw_size * asset->channels;
	for(int i = 0; i < len; i++)
	{
		for(int j = 0; j < asset->channels; j++)
		{
			int sample = (int)(buffer[j][i] * 32767);
			CLAMP(sample, -32768, 32767);
			*out_ptr++ = sample;
		}
	}
	temp_raw_size += len;

	int frame_size = codec_context->frame_size;
	int output_size = 0;
	int current_sample = 0;
	for(current_sample = 0; 
		current_sample + frame_size <= temp_raw_size; 
		current_sample += frame_size)
	{
		int compressed_size = avcodec_encode_audio(
			codec_context, 
			temp_compressed + output_size, 
			compressed_allocated - output_size, 
            temp_raw + current_sample * asset->channels);
		output_size += compressed_size;
	}

// Shift buffer back
	memcpy(temp_raw,
		temp_raw + current_sample * asset->channels,
		(temp_raw_size - current_sample) * sizeof(int16_t) * asset->channels);
	temp_raw_size -= current_sample;

	int bytes_written = fwrite(temp_compressed, 1, output_size, fd);
	if(bytes_written < output_size)
	{
		eprintf("Error while writing samples. \n%m\n");
		return 1;
	}
	return 0;
}







AC3ConfigAudio::AC3ConfigAudio(BC_WindowBase *parent_window,
	Asset *asset)
 : BC_Window(PROGRAM_NAME ": Audio Compression",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	500,
	BC_OKButton::calculate_h() + 100,
	500,
	BC_OKButton::calculate_h() + 100,
	0,
	0,
	1)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

void AC3ConfigAudio::create_objects()
{
	int x = 10, y = 10;
	int x1 = 150;
	add_tool(new BC_Title(x, y, "Bitrate (kbps):"));
	AC3ConfigAudioBitrate *bitrate;
	add_tool(bitrate = 
		new AC3ConfigAudioBitrate(this,
			x1, 
			y));
	bitrate->create_objects();

	add_subwindow(new BC_OKButton(this));
	show_window();
	flush();
}

int AC3ConfigAudio::close_event()
{
	set_done(0);
	return 1;
}






AC3ConfigAudioBitrate::AC3ConfigAudioBitrate(AC3ConfigAudio *gui, 
	int x, 
	int y)
 : BC_PopupMenu(x,
 	y,
	150,
	AC3ConfigAudioBitrate::bitrate_to_string(gui->string, gui->asset->ac3_bitrate))
{
	this->gui = gui;
}

char* AC3ConfigAudioBitrate::bitrate_to_string(char *string, int bitrate)
{
	sprintf(string, "%d", bitrate);
	return string;
}

void AC3ConfigAudioBitrate::create_objects()
{
	add_item(new BC_MenuItem("32"));
	add_item(new BC_MenuItem("40"));
	add_item(new BC_MenuItem("48"));
	add_item(new BC_MenuItem("56"));
	add_item(new BC_MenuItem("64"));
	add_item(new BC_MenuItem("80"));
	add_item(new BC_MenuItem("96"));
	add_item(new BC_MenuItem("112"));
	add_item(new BC_MenuItem("128"));
	add_item(new BC_MenuItem("160"));
	add_item(new BC_MenuItem("192"));
	add_item(new BC_MenuItem("224"));
	add_item(new BC_MenuItem("256"));
	add_item(new BC_MenuItem("320"));
	add_item(new BC_MenuItem("384"));
	add_item(new BC_MenuItem("448"));
	add_item(new BC_MenuItem("512"));
	add_item(new BC_MenuItem("576"));
	add_item(new BC_MenuItem("640"));
}

int AC3ConfigAudioBitrate::handle_event()
{
	gui->asset->ac3_bitrate = atol(get_text());
	return 1;
}



