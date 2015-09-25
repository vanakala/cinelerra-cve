
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

#include "aframe.h"
#include "asset.h"
#include "bcsignals.h"
#include "clip.h"
#include "fileac3.h"
#include "language.h"
#include "mainerror.h"
#include "theme.h"

#include <string.h>

extern Theme *theme_global;

FileAC3::FileAC3(Asset *asset, File *file)
 : FileBase(asset, file)
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

FileAC3::~FileAC3()
{
	close_file();
}

void FileAC3::get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int options)
{
	if(options & SUPPORTS_AUDIO)
	{

		AC3ConfigAudio *window = new AC3ConfigAudio(parent_window, asset);
		format_window = window;
		window->run_window();
		delete window;
	}
}

int FileAC3::supports(int format)
{
	if(format == FILE_AC3)
		return SUPPORTS_AUDIO;
	return 0;
}

int FileAC3::open_file(int rd, int wr)
{
	if(wr)
	{
		avcodec_init();
		avcodec_register_all();
		codec = avcodec_find_encoder(CODEC_ID_AC3);
		if(!codec)
		{
			errorbox("AC3 Codec not found.");
			return 1;
		}
		codec_context = avcodec_alloc_context();
		codec_context->bit_rate = asset->ac3_bitrate * 1000;
		codec_context->sample_rate = asset->sample_rate;
		codec_context->channels = asset->channels;
#if LIBAVCODEC_VERSION_INT < ((52<<16)+(0<<8)+0)
		codec_context->sample_fmt = SAMPLE_FMT_S16;
#else
		codec_context->sample_fmt = AV_SAMPLE_FMT_S16;
		switch(asset->channels)
		{
		case 1:
			codec_context->channel_layout = AV_CH_LAYOUT_MONO;
			break;
		case 2:
			codec_context->channel_layout = AV_CH_LAYOUT_STEREO;
			break;
		case 3:
			codec_context->channel_layout = AV_CH_LAYOUT_SURROUND;
			break;
		case 4:
			codec_context->channel_layout = AV_CH_LAYOUT_QUAD;
			break;
		case 5:
			codec_context->channel_layout = AV_CH_LAYOUT_5POINT0;
			break;
		case 6:
			codec_context->channel_layout = AV_CH_LAYOUT_5POINT1;
			break;
		case 7:
			codec_context->channel_layout = AV_CH_LAYOUT_7POINT0;
			break;
		case 8:
			codec_context->channel_layout = AV_CH_LAYOUT_7POINT1;
			break;
		default:
			codec_context->channel_layout = AV_CH_LAYOUT_NATIVE;
			break;
		}
#endif
		if(avcodec_open(codec_context, codec))
		{
			errorbox("Failed to open AC3 codec.");
			return 1;
		}

		if(!(fd = fopen(asset->path, "w")))
		{
			errormsg("Error while opening \"%s\" for writing. \n%m", asset->path);
			return 1;
		}
	}
	else
	{
		// Should not reach here
		errorbox("FileAC3 does not support decoding");
		return 1;
	}
	return 0;
}

void FileAC3::close_file()
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
}

int FileAC3::write_aframes(AFrame **frames)
{
// Convert buffer to encoder format
	int len = frames[0]->length;

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
			int sample = (int)(frames[j]->buffer[i] * 32767);
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
		errorbox("Failed to write AC3 samples.");
		return 1;
	}
	return 0;
}


AC3ConfigAudio::AC3ConfigAudio(BC_WindowBase *parent_window,
	Asset *asset)
 : BC_Window("Audio Compression - " PROGRAM_NAME,
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
	AC3ConfigAudioBitrate *bitrate;
	int x = 10, y = 10;
	int x1 = 150;
	set_icon(theme_global->get_image("mwindow_icon"));
	this->asset = asset;
	add_tool(new BC_Title(x, y, "Bitrate (kbps):"));
	add_tool(bitrate = new AC3ConfigAudioBitrate(this,
			x1, 
			y));
	add_subwindow(new BC_OKButton(this));
	show_window();
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

char* AC3ConfigAudioBitrate::bitrate_to_string(char *string, int bitrate)
{
	sprintf(string, "%d", bitrate);
	return string;
}

int AC3ConfigAudioBitrate::handle_event()
{
	gui->asset->ac3_bitrate = atol(get_text());
	return 1;
}
