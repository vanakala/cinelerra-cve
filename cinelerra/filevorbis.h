
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

#ifndef FILEVORBIS_H
#define FILEVORBIS_H

#include "file.inc"
#include "filebase.h"
#include "vorbis/vorbisenc.h"
#include "vorbis/vorbisfile.h"
#include "resample.inc"






class FileVorbis : public FileBase
{
public:
	FileVorbis(Asset *asset, File *file);
	~FileVorbis();

	static void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options);
	int reset_parameters_derived();

	static int check_sig(Asset *asset);
	int open_file(int rd, int wr);
	int close_file();
	int write_samples(double **buffer, 
			int64_t len);

	int read_samples(double *buffer, int64_t len);
	int read_samples_float(float *buffer, int64_t len);
	int prefer_samples_float();
	
// Decoding
	OggVorbis_File vf;
	FILE *fd;
	double **pcm_history;
	float **pcm_history_float;
#define HISTORY_MAX 0x100000
	int history_size;
	int history_start;

// Encoding
	vorbis_info vi;
	vorbis_comment vc;
	vorbis_dsp_state vd;
	vorbis_block vb;
	ogg_stream_state os;
	ogg_page og;
	ogg_packet op;
};


class VorbisConfigAudio;


class VorbisFixedBitrate : public BC_Radial
{
public:
	VorbisFixedBitrate(int x, int y, VorbisConfigAudio *gui);
	int handle_event();
	VorbisConfigAudio *gui;
};

class VorbisVariableBitrate : public BC_Radial
{
public:
	VorbisVariableBitrate(int x, int y, VorbisConfigAudio *gui);
	int handle_event();
	VorbisConfigAudio *gui;
};

class VorbisMinBitrate : public BC_TextBox
{
public:
	VorbisMinBitrate(int x, 
		int y, 
		VorbisConfigAudio *gui, 
		char *text);
	int handle_event();
	VorbisConfigAudio *gui;
};

class VorbisMaxBitrate : public BC_TextBox
{
public:
	VorbisMaxBitrate(int x, 
		int y, 
		VorbisConfigAudio *gui, 
		char *text);
	int handle_event();
	VorbisConfigAudio *gui;
};

class VorbisAvgBitrate : public BC_TextBox
{
public:
	VorbisAvgBitrate(int x, 
		int y, 
		VorbisConfigAudio *gui, 
		char *text);
	int handle_event();
	VorbisConfigAudio *gui;
};

class VorbisConfigAudio : public BC_Window
{
public:
	VorbisConfigAudio(BC_WindowBase *parent_window, Asset *asset);
	~VorbisConfigAudio();

	int create_objects();
	int close_event();

	VorbisFixedBitrate *fixed_bitrate;
	VorbisVariableBitrate *variable_bitrate;
	BC_WindowBase *parent_window;
	char string[BCTEXTLEN];
	Asset *asset;
};



#endif
