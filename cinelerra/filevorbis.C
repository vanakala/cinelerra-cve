
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
#include "byteorder.h"
#include "clip.h"
#include "file.h"
#include "filevorbis.h"
#include "guicast.h"
#include "language.h"
#include "mwindow.inc"
#include "mainerror.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

FileVorbis::FileVorbis(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset_parameters();
	if(asset->format == FILE_UNKNOWN) asset->format = FILE_VORBIS;
	asset->byte_order = 0;
}

FileVorbis::~FileVorbis()
{
	close_file();
}

void FileVorbis::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int audio_options,
	int video_options)
{
	if(audio_options)
	{
		VorbisConfigAudio *window = new VorbisConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}

int FileVorbis::check_sig(Asset *asset)
{
// FILEVORBIS DECODING IS DISABLED
	return 0;
	FILE *fd = fopen(asset->path, "rb");
	OggVorbis_File vf;

// Test for Quicktime since OGG misinterprets it
	fseek(fd, 4, SEEK_SET);
	char data[4];
	fread(data, 4, 1, fd);
	if(data[0] == 'm' &&
		data[1] == 'd' &&
		data[2] == 'a' &&
		data[3] == 't')
	{
		fclose(fd);
		return 0;
	}
	
	fseek(fd, 0, SEEK_SET);

	if(ov_open(fd, &vf, NULL, 0) < 0)
	{
// OGG failed.  Close file handle manually.
		ov_clear(&vf);
		if(fd) fclose(fd);
		return 0;
	}
	else
	{
		ov_clear(&vf);
		return 1;
	}
}

int FileVorbis::reset_parameters_derived()
{
	fd = 0;
	bzero(&vf, sizeof(vf));
	pcm_history = 0;
	pcm_history_float = 0;
}


// Just create the Quicktime objects since this routine is also called
// for reopening.
int FileVorbis::open_file(int rd, int wr)
{
	int result = 0;
	this->rd = rd;
	this->wr = wr;

//printf("FileVorbis::open_file 1\n");
	if(rd)
	{
//printf("FileVorbis::open_file 1\n");
		if(!(fd = fopen(asset->path, "rb")))
		{
			eprintf("Error while opening \"%s\" for reading. \n%m\n", asset->path);
			result = 1;
		}
		else
		{
//printf("FileVorbis::open_file 2 %p %p\n", fd, vf);
			if(ov_open(fd, &vf, NULL, 0) < 0)
			{
				eprintf("Invalid bitstream in %s\n", asset->path);
				result = 1;
			}
			else
			{
//printf("FileVorbis::open_file 1\n");
				vorbis_info *vi = ov_info(&vf, -1);
				asset->channels = vi->channels;
				if(!asset->sample_rate)
					asset->sample_rate = vi->rate;
//printf("FileVorbis::open_file 1\n");
				asset->audio_length = ov_pcm_total(&vf,-1);
//printf("FileVorbis::open_file 1\n");
				asset->audio_data = 1;
// printf("FileVorbis::open_file 1 %d %d %d\n", 
// asset->channels, 
// asset->sample_rate, 
// asset->audio_length);
			}
		}
	}

	if(wr)
	{
		if(!(fd = fopen(asset->path, "wb")))
		{
			eprintf("Error while opening \"%s\" for writing. \n%m\n", asset->path);
			result = 1;
		}
		else
		{
			vorbis_info_init(&vi);
			if(!asset->vorbis_vbr)
				result = vorbis_encode_init(&vi, 
					asset->channels, 
					asset->sample_rate, 
					asset->vorbis_max_bitrate, 
					asset->vorbis_bitrate, 
					asset->vorbis_min_bitrate);
			else
			{
				result = vorbis_encode_setup_managed(&vi,
					asset->channels, 
					asset->sample_rate, 
					asset->vorbis_max_bitrate, 
					asset->vorbis_bitrate, 
					asset->vorbis_min_bitrate);
				result |= vorbis_encode_ctl(&vi, OV_ECTL_RATEMANAGE_AVG, NULL);
				result |= vorbis_encode_setup_init(&vi);
			}

			if(!result)
			{
				vorbis_analysis_init(&vd, &vi);
				vorbis_block_init(&vd, &vb);
				vorbis_comment_init(&vc);
				srand(time(NULL));
				ogg_stream_init(&os, rand());

				ogg_packet header;
				ogg_packet header_comm;
				ogg_packet header_code;
				vorbis_analysis_headerout(&vd, 
					&vc,
					&header,
					&header_comm,
					&header_code);
				ogg_stream_packetin(&os,
					&header);
				ogg_stream_packetin(&os, 
					&header_comm);
				ogg_stream_packetin(&os,
					&header_code);

				while(1)
				{
					int result = ogg_stream_flush(&os, &og);
					if(result == 0) break;
					fwrite(og.header, 1, og.header_len, fd);
					fwrite(og.body, 1, og.body_len, fd);
				}
			}
		}
	}

//printf("FileVorbis::open_file 2\n");
	return result;
}

#define FLUSH_VORBIS \
while(vorbis_analysis_blockout(&vd, &vb) == 1) \
{ \
	vorbis_analysis(&vb, NULL); \
	vorbis_bitrate_addblock(&vb); \
	while(vorbis_bitrate_flushpacket(&vd, &op)) \
	{ \
		ogg_stream_packetin(&os, &op); \
		int done = 0; \
		while(1) \
		{ \
			int result = ogg_stream_pageout(&os, &og); \
			if(!result) break; \
			fwrite(og.header, 1, og.header_len, fd); \
			fwrite(og.body, 1, og.body_len, fd); \
			if(ogg_page_eos(&og)) break; \
		} \
	} \
}


int FileVorbis::close_file()
{
	if(fd)
	{
		if(wr)
		{
			vorbis_analysis_wrote(&vd, 0);
			FLUSH_VORBIS

			ogg_stream_clear(&os);
			vorbis_block_clear(&vb);
			vorbis_dsp_clear(&vd);
			vorbis_comment_clear(&vc);
			vorbis_info_clear(&vi);
			fclose(fd);
		}
		
		if(rd)
		{
// This also closes the file handle.
			ov_clear(&vf);
		}
		fd = 0;
	}

	if(pcm_history)
	{
		for(int i = 0; i < asset->channels; i++)
			delete [] pcm_history[i];
		delete [] pcm_history;
	}
	if(pcm_history_float)
	{
		for(int i = 0; i < asset->channels; i++)
			delete [] pcm_history_float[i];
		delete [] pcm_history_float;
	}

	reset_parameters();
	FileBase::close_file();
	return 0;
}


int FileVorbis::write_samples(double **buffer, int64_t len)
{
	if(!fd) return 0;
	int result = 0;

	float **vorbis_buffer = vorbis_analysis_buffer(&vd, len);
	for(int i = 0; i < asset->channels; i++)
	{
		float *output = vorbis_buffer[i];
		double *input = buffer[i];
		for(int j = 0; j < len; j++)
		{
			output[j] = input[j];
		}
	}
    vorbis_analysis_wrote(&vd, len);

	FLUSH_VORBIS

	return result;
}

int FileVorbis::read_samples(double *buffer, int64_t len)
{
	if(!fd) return 0;

// printf("FileVorbis::read_samples 1 %d %d %d %d\n", 
// history_start, 
// history_size,
// file->current_sample,
// len);
	float **vorbis_output;
	int bitstream;
	int accumulation = 0;
//printf("FileVorbis::read_samples 1\n");
	int decode_start = 0;
	int decode_len = 0;

	if(len > 0x100000)
	{
		eprintf("FileVorbis::read_samples max samples=%d\n", HISTORY_MAX);
		return 1;
	}

	if(!pcm_history)
	{
		pcm_history = new double*[asset->channels];
		for(int i = 0; i < asset->channels; i++)
			pcm_history[i] = new double[HISTORY_MAX];
		history_start = 0;
		history_size = 0;
	}

// Restart history.  Don't bother shifting history back.
	if(file->current_sample < history_start ||
		file->current_sample > history_start + history_size)
	{
		history_size = 0;
		history_start = file->current_sample;
		decode_start = file->current_sample;
		decode_len = len;
	}
	else
// Shift history forward to make room for new samples
	if(file->current_sample + len > history_start + history_size)
	{
		if(file->current_sample + len > history_start + HISTORY_MAX)
		{
			int diff = file->current_sample + len - (history_start + HISTORY_MAX);
			for(int i = 0; i < asset->channels; i++)
			{
				double *temp = pcm_history[i];
				for(int j = 0; j < HISTORY_MAX - diff; j++)
				{
					temp[j] = temp[j + diff];
				}
			}
			history_start += diff;
			history_size -= diff;
		}

// Decode more data
		decode_start = history_start + history_size;
		decode_len = file->current_sample + len - (history_start + history_size);
	}


// Fill history buffer
	if(history_start + history_size != ov_pcm_tell(&vf))
	{
//printf("FileVorbis::read_samples %d %d\n", history_start + history_size, ov_pcm_tell(&vf));
		ov_pcm_seek(&vf, history_start + history_size);
	}

	while(accumulation < decode_len)
	{
		int result = ov_read_float(&vf,
			&vorbis_output,
			decode_len - accumulation,
			&bitstream);
//printf("FileVorbis::read_samples 1 %d %d %d\n", result, len, accumulation);
		if(!result) break;

		for(int i = 0; i < asset->channels; i++)
		{
			double *output = pcm_history[i] + history_size;
			float *input = vorbis_output[i];
			for(int j = 0; j < result; j++)
				output[j] = input[j];
		}
		history_size += result;
		accumulation += result;
	}


// printf("FileVorbis::read_samples 1 %d %d\n", 
// file->current_sample,
// history_start);

	double *input = pcm_history[file->current_channel] + 
		file->current_sample - 
		history_start;
	for(int i = 0; i < len; i++)
		buffer[i] = input[i];

// printf("FileVorbis::read_samples 2 %d %d %d %d\n", 
// history_start, 
// history_size,
// file->current_sample,
// len);

	return 0;
}

int FileVorbis::prefer_samples_float() 
{
	return 1;
}

int FileVorbis::read_samples_float(float *buffer, int64_t len)
{
	if(!fd) return 0;

// printf("FileVorbis::read_samples 1 %d %d %d %d\n", 
// history_start, 
// history_size,
// file->current_sample,
// len);
	float **vorbis_output;
	int bitstream;
	int accumulation = 0;
//printf("FileVorbis::read_samples 1\n");
	int decode_start = 0;
	int decode_len = 0;

	if(len > 0x100000)
	{
		eprintf("FileVorbis::read_samples max samples=%d\n", HISTORY_MAX);
		return 1;
	}

	if(!pcm_history_float)
	{
		pcm_history_float = new float*[asset->channels];
		for(int i = 0; i < asset->channels; i++)
			pcm_history_float[i] = new float[HISTORY_MAX];
		history_start = 0;
		history_size = 0;
	}

// Restart history.  Don't bother shifting history back.
	if(file->current_sample < history_start ||
		file->current_sample > history_start + history_size)
	{
		history_size = 0;
		history_start = file->current_sample;
		decode_start = file->current_sample;
		decode_len = len;
	}
	else
// Shift history forward to make room for new samples
	if(file->current_sample + len > history_start + history_size)
	{
		if(file->current_sample + len > history_start + HISTORY_MAX)
		{
			int diff = file->current_sample + len - (history_start + HISTORY_MAX);
			for(int i = 0; i < asset->channels; i++)
			{
				float *temp = pcm_history_float[i];
//				for(int j = 0; j < HISTORY_MAX - diff; j++)
//				{
//					temp[j] = temp[j + diff];
//				}
				bcopy(temp, temp + diff, (HISTORY_MAX - diff) * sizeof(float));
			}
			history_start += diff;
			history_size -= diff;
		}

// Decode more data
		decode_start = history_start + history_size;
		decode_len = file->current_sample + len - (history_start + history_size);
	}


// Fill history buffer
	if(history_start + history_size != ov_pcm_tell(&vf))
	{
//printf("FileVorbis::read_samples %d %d\n", history_start + history_size, ov_pcm_tell(&vf));
		ov_pcm_seek(&vf, history_start + history_size);
	}

	while(accumulation < decode_len)
	{
		int result = ov_read_float(&vf,
			&vorbis_output,
			decode_len - accumulation,
			&bitstream);
//printf("FileVorbis::read_samples 1 %d %d %d\n", result, len, accumulation);
		if(!result) break;

		for(int i = 0; i < asset->channels; i++)
		{
			float *output = pcm_history_float[i] + history_size;
			float *input = vorbis_output[i];
//			for(int j = 0; j < result; j++)
//				output[j] = input[j];
			bcopy(input, output, result * sizeof(float));
		}
		history_size += result;
		accumulation += result;
	}


// printf("FileVorbis::read_samples 1 %d %d\n", 
// file->current_sample,
// history_start);

	float *input = pcm_history_float[file->current_channel] + 
		file->current_sample - 
		history_start;
//	for(int i = 0; i < len; i++)
//		buffer[i] = input[i];
	bcopy(input, buffer, len * sizeof(float));

// printf("FileVorbis::read_samples 2 %d %d %d %d\n", 
// history_start, 
// history_size,
// file->current_sample,
// len);

	return 0;
}










VorbisConfigAudio::VorbisConfigAudio(BC_WindowBase *parent_window, 
	Asset *asset)
 : BC_Window(PROGRAM_NAME ": Audio Compression",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	350,
	170,
	-1,
	-1,
	0,
	0,
	1)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

VorbisConfigAudio::~VorbisConfigAudio()
{
}

int VorbisConfigAudio::create_objects()
{
	int x = 10, y = 10;
	int x1 = 150;
	char string[BCTEXTLEN];

	add_tool(fixed_bitrate = new VorbisFixedBitrate(x, y, this));
	add_tool(variable_bitrate = new VorbisVariableBitrate(x1, y, this));

	y += 30;
	sprintf(string, "%d", asset->vorbis_min_bitrate);
	add_tool(new BC_Title(x, y, _("Min bitrate:")));
	add_tool(new VorbisMinBitrate(x1, y, this, string));

	y += 30;
	add_tool(new BC_Title(x, y, _("Avg bitrate:")));
	sprintf(string, "%d", asset->vorbis_bitrate);
	add_tool(new VorbisAvgBitrate(x1, y, this, string));

	y += 30;
	add_tool(new BC_Title(x, y, _("Max bitrate:")));
	sprintf(string, "%d", asset->vorbis_max_bitrate);
	add_tool(new VorbisMaxBitrate(x1, y, this, string));


	add_subwindow(new BC_OKButton(this));
	show_window();
	flush();
	return 0;
}

int VorbisConfigAudio::close_event()
{
	set_done(0);
	return 1;
}





VorbisFixedBitrate::VorbisFixedBitrate(int x, int y, VorbisConfigAudio *gui)
 : BC_Radial(x, y, !gui->asset->vorbis_vbr, _("Fixed bitrate"))
{
	this->gui = gui;
}
int VorbisFixedBitrate::handle_event()
{
	gui->asset->vorbis_vbr = 0;
	gui->variable_bitrate->update(0);
	return 1;
}

VorbisVariableBitrate::VorbisVariableBitrate(int x, int y, VorbisConfigAudio *gui)
 : BC_Radial(x, y, gui->asset->vorbis_vbr, _("Variable bitrate"))
{
	this->gui = gui;
}
int VorbisVariableBitrate::handle_event()
{
	gui->asset->vorbis_vbr = 1;
	gui->fixed_bitrate->update(0);
	return 1;
}


VorbisMinBitrate::VorbisMinBitrate(int x, 
	int y, 
	VorbisConfigAudio *gui, 
	char *text)
 : BC_TextBox(x, y, 180, 1, text)
{
	this->gui = gui;
}
int VorbisMinBitrate::handle_event()
{
	gui->asset->vorbis_min_bitrate = atol(get_text());
	return 1;
}



VorbisMaxBitrate::VorbisMaxBitrate(int x, 
	int y, 
	VorbisConfigAudio *gui,
	char *text)
 : BC_TextBox(x, y, 180, 1, text)
{
	this->gui = gui;
}
int VorbisMaxBitrate::handle_event()
{
	gui->asset->vorbis_max_bitrate = atol(get_text());
	return 1;
}



VorbisAvgBitrate::VorbisAvgBitrate(int x, int y, VorbisConfigAudio *gui, char *text)
 : BC_TextBox(x, y, 180, 1, text)
{
	this->gui = gui;
}
int VorbisAvgBitrate::handle_event()
{
	gui->asset->vorbis_bitrate = atol(get_text());
	return 1;
}




