
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
#include "mwindow.h"
#include "theme.h"

#include <string.h>

extern Theme *theme_global;

FileAC3::FileAC3(Asset *asset, File *file)
 : FileBase(asset, file)
{
	codec = 0;
	codec_context = 0;
	fd = 0;
	avframe = 0;
	swr_context = 0;
	memset(resampled_data, 0, sizeof(resampled_data));
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
		avcodec_register_all();
		codec = avcodec_find_encoder(AV_CODEC_ID_AC3);
		if(!codec)
		{
			errorbox("AC3 Codec not found.");
			return 1;
		}
		codec_context = avcodec_alloc_context3(codec);
		codec_context->bit_rate = asset->ac3_bitrate * 1000;
		codec_context->sample_rate = asset->sample_rate;
		codec_context->channels = asset->channels;
		codec_context->sample_fmt = AV_SAMPLE_FMT_FLTP;
		codec_context->channel_layout = AV_CH_FRONT_LEFT | AV_CH_FRONT_RIGHT;
		codec_context->flags |= CODEC_CAP_SMALL_LAST_FRAME;

		if(avcodec_open2(codec_context, codec, NULL))
		{
			errorbox(_("FileAC3: Failed to open AC3 codec."));
			return 1;
		}
		if(!(swr_context = swr_alloc_set_opts(NULL,
			codec_context->channel_layout,
			codec_context->sample_fmt,
			codec_context->sample_rate,
			AV_CH_FRONT_LEFT | AV_CH_FRONT_RIGHT,
			AV_SAMPLE_FMT_DBLP,
			asset->sample_rate, 0, 0)))
		{
			errorbox(_("FileAC3: Can't allocate resample context."));
			return 1;
		}
		if(swr_init(swr_context))
		{
			errorbox(_("FileAC3: Failed to initalize resample context"));
			return 1;
		}
		if(!(avframe = av_frame_alloc()))
		{
			errorbox(_("FileAC3: Could not allocate audio frame"));
			return 1;
		}
		if(!(fd = fopen(asset->path, "w")))
		{
			errormsg(_("FileAC3: Error while opening \"%s\" for writing. \n%m"), asset->path);
			return 1;
		}
	}
	else
	{
		// Should not reach here
		errorbox(_("FileAC3 does not support decoding"));
		return 1;
	}
	return 0;
}

void FileAC3::close_file()
{
	if(avframe)
	{
		if(fd && codec_context)
		{
			AVPacket pkt;
			int got_output;
			int resampled_length;

			// Get samples kept by resampler
			while(resampled_length = swr_convert(swr_context,
				(uint8_t**)resampled_data, resampled_alloc, 0, 0))
			{
				if(resampled_length < 0)
				{
					errorbox(_("FileAC3: failed to resample last data"));
					break;
				}
				write_samples(resampled_length);
			}
			// Get out samples kept in encoder
			av_init_packet(&pkt);
			pkt.data = 0;
			pkt.size = 0;
			for(got_output = 1; got_output;)
			{
				if(avcodec_encode_audio2(codec_context, &pkt, 0, &got_output))
				{
					errorbox(_("FileAC3: failed to encode last packet"));
					break;
				}
				if(got_output)
				{
					if(fwrite(pkt.data, 1, pkt.size, fd) != pkt.size)
					{
						errorbox(_("FileAC3: Failed to write last packet"));
						break;
					}
				}
			}
		}
		av_frame_free(&avframe);
	}

	for(int i = 0; i < MAXCHANNELS; i++)
	{
		delete [] resampled_data[i];
		resampled_data[i] = 0;
	}

	if(swr_context)
		swr_free(&swr_context);

	if(codec_context)
	{
		avcodec_close(codec_context);
		avcodec_free_context(&codec_context);
		codec = 0;
	}
	if(fd)
	{
		fclose(fd);
		fd = 0;
	}
}

int FileAC3::write_samples(int resampled_length)
{
	AVPacket pkt;
	int got_output, chan;
	int frame_size = codec_context->frame_size;

	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	for(int i = 0; i < resampled_length; i += frame_size)
	{
		avframe->nb_samples = frame_size;
		avframe->format = codec_context->sample_fmt;
		avframe->channel_layout = codec_context->channel_layout;

		for(chan = 0; chan < asset->channels; chan++)
		{
			avframe->data[chan] = (uint8_t*)&resampled_data[chan][i];
			avframe->linesize[chan] = frame_size *
				av_get_bytes_per_sample(codec_context->sample_fmt);
		}

		if(avcodec_encode_audio2(codec_context, &pkt, avframe, &got_output) < 0)
		{
			errorbox(_("Failed encoding audio frame"));
			return 1;
		}

		if(got_output)
		{
			if(fwrite(pkt.data, 1, pkt.size, fd) != pkt.size)
			{
				errorbox(_("Failed to write samples"));
				return 1;
			}
		}
	}
	av_free_packet(&pkt);
	return 0;
}

int FileAC3::write_aframes(AFrame **frames)
{
	int chan;
	int in_length = frames[0]->length;
	double *in_data[MAXCHANNELS];
	int resampled_length;
	int rv;

	if(!resampled_data[0])
	{
		resampled_alloc = av_rescale_rnd(swr_get_delay(swr_context, asset->sample_rate) +
			in_length, codec_context->sample_rate, asset->sample_rate,
			AV_ROUND_UP);
		resampled_alloc = resampled_alloc / codec_context->frame_size * codec_context->frame_size;
		for(chan = 0; chan < asset->channels; chan++)
			resampled_data[chan] = new float[resampled_alloc];
	}

// Resample the whole buffer
	for(chan = 0; chan < asset->channels; chan++)
		in_data[chan] = frames[chan]->buffer;

	if((resampled_length = swr_convert(swr_context, (uint8_t**)resampled_data,
		resampled_alloc, (const uint8_t**)in_data, in_length)) < 0)
	{
		errorbox(_("Failed to resample data"));
		return 1;
	}
	rv = write_samples(resampled_length);

// We put more samples into resampler than we consume,
// get some of them to avoid resampler become too large
	if(!rv && swr_get_out_samples(swr_context, 0) > resampled_alloc)
	{
		if((resampled_length = swr_convert(swr_context, (uint8_t**)resampled_data,
			resampled_alloc, 0, 0)) < 0)
		{
			errorbox(_("Failed to resample data"));
			return 1;
		}
		rv = write_samples(resampled_length);
	}
	return rv;
}


AC3ConfigAudio::AC3ConfigAudio(BC_WindowBase *parent_window,
	Asset *asset)
 : BC_Window(MWindow::create_title(N_("Audio Compression")),
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
