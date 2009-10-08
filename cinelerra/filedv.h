
/*
 * CINELERRA
 * Copyright (C) 2004 Richard Baverstock
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

#ifndef FILEDV_H
#define FILEDV_H

#include "../config.h"
#include "filebase.h"
#include "file.inc"

#ifdef DV_USE_FFMPEG
#include <avcodec.h>
#endif

#include <libdv/dv.h>


class FileDV : public FileBase
{
public:
	FileDV(Asset *asset, File *file);
	~FileDV();

	static void get_parameters(BC_WindowBase *parent_window,
		Asset *asset,
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options);
	
	int reset_parameters_derived();
	int open_file(int rd, int wr);

	static int check_sig(Asset *asset);
	int close_file_derived();
	
	int64_t get_video_position();
	int64_t get_audio_position();
	
	int set_video_position(int64_t x);
	int set_audio_position(int64_t x);

	int audio_samples_copy(double **buffer, int64_t len);
	
	int write_samples(double **buffer, int64_t len);
	int write_frames(VFrame ***frames, int len);
	
	int read_compressed_frame(VFrame *buffer);
	int write_compressed_frame(VFrame *buffers);
	
	int64_t compressed_frame_size();
	
	int read_samples(double *buffer, int64_t len);
	int read_frame(VFrame *frame);
	
	int colormodel_supported(int colormodel);
	
	int can_copy_from(Edit *edit, int64_t position);
	
	static int get_best_colormodel(Asset *asset, int driver);
	
	int get_audio_frame(int64_t pos);
	int get_audio_offset(int64_t pos);

private:
	FILE *stream;

	Mutex *stream_lock;
	Mutex *decoder_lock;
	Mutex *video_position_lock;
	
	dv_decoder_t *decoder;
	dv_encoder_t *encoder;
	dv_encoder_t *audio_encoder;
		
	int64_t audio_position;
	int64_t video_position;
	
	unsigned char *video_buffer;
	unsigned char *audio_buffer;

	int16_t **audio_sample_buffer;
	int audio_sample_buffer_start;
	int audio_sample_buffer_end;
	int audio_sample_buffer_len;
	int audio_sample_buffer_maxsize;
	
	int audio_frames_written;
	
	int output_size;
	int isPAL;
};


class DVConfigAudio: public BC_Window
{
public:
	DVConfigAudio(BC_WindowBase *parent_window, Asset *asset);
	~DVConfigAudio();

	int create_objects();
	int close_event();

private:
	Asset *asset;
	BC_WindowBase *parent_window;
};



class DVConfigVideo: public BC_Window
{
public:
	DVConfigVideo(BC_WindowBase *parent_window, Asset *asset);
	~DVConfigVideo();

	int create_objects();
	int close_event();

private:
	Asset *asset;
	BC_WindowBase *parent_window;
};


#endif
