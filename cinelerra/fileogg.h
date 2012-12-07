
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

#ifndef FILEOGG_H
#define FILEOGG_H

#include "config.h"
#include "filebase.h"
#include "maxchannels.h"
#include "maxbuffers.h"
#include "file.inc"

#include <theora/theora.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

#define OGG_INDEX_INTERVAL 2

typedef struct 
{
	ogg_sync_state sync;
	off_t file_bufpos; // position of the start of the buffer inside the file
	off_t file_pagepos; // position of the page that will be next read 
	off_t file_pagepos_found; // position of last page that was returned (in seeking operations)
	int wlen;
} sync_window_t;

typedef struct {
	ogg_stream_state state;
	vorbis_info *vi;
	vorbis_comment *vc;
	theora_info *ti;
	theora_comment *tc;
	theora_state ts;
	vorbis_dsp_state vs;
	yuv_buffer frame;
	vorbis_block block;
	ogg_page page;
	ptstime page_pts;
	int page_valid;
	int headers;
	int eos;
	int dec_init;
	int toc_streamno;
	int max_pcm_samples; // Number of channels allocated
	float *pcm_samples[MAXCHANNELS];
	samplenum pcm_start;
	int pcm_max;
	int pcm_size;
} media_stream_t;


class FileOGG : public FileBase
{
friend class PackagingEngineOGG;
public:
	FileOGG(Asset *asset, File *file);
	~FileOGG();

	static void get_parameters(BC_WindowBase *parent_window,
		Asset *asset,
		BC_WindowBase* &format_window,
		int options);

	int open_file(int rd, int wr);
	static int check_sig(Asset *asset);
	static int supports(int format);
	void close_file();

	int colormodel_supported(int colormodel);
	int get_best_colormodel(Asset *asset, int driver);
	int write_aframes(AFrame **buffer);
	int write_frames(VFrame ***frames, int len);
	int read_aframe(AFrame *aframe);
	int read_frame(VFrame *frame);

// Callbacks for TOC generation
	int get_streamcount();
	stream_params *get_track_data(int trx);

private:
	int read_buffer();
	int read_buffer_at(off_t filepos);
	int next_oggpage(ogg_page *op);
	int next_oggstreampage(ogg_page *op);
	int sync_page(ogg_page *page);
	int write_page(ogg_page *page);
	int write_file();
	void flush_ogg(int e_o_s);
	void set_audio_stream();
	void set_video_stream(VFrame *frame);

	void move_pcmsamples_position();
	int copy_to_pcmsamples(float **smpls, int start, int smplen);
	int fill_pcm_samples(int len);

	FILE *stream;
	FileTOC *tocfile;
	int page_write_error;
	int reading;
	int writing;

	ogg_sync_state sync_state;
	vorbis_info vrb_info;
	vorbis_comment vrb_comment;
	theora_info the_info;
	theora_comment the_comment;
	int free_stream;
	ogg_page page;
	ogg_packet pkt;
	int open_streams;
	int configured_streams;
	off_t file_bufpos;
	off_t file_pagepos;
	off_t file_pagestart;
	media_stream_t *cur_stream;
	media_stream_t streams[MAXCHANNELS];
	stream_params track_data;
	VFrame *temp_frame;
	Mutex *flush_lock;

	off_t filedata_begin;

	samplenum sample_position;  // what will be the next sample taken from vorbis decoder
	samplenum next_sample_position; // what is the next sample read_samples must deliver

	int theora_cmodel;
	framenum frame_position;    // LAST decoded frame position
};

class OGGConfigAudio;
class OGGConfigVideo;

class OGGVorbisFixedBitrate : public BC_Radial
{
public:
	OGGVorbisFixedBitrate(int x, int y, OGGConfigAudio *gui);
	int handle_event();
	OGGConfigAudio *gui;
};

class OGGVorbisVariableBitrate : public BC_Radial
{
public:
	OGGVorbisVariableBitrate(int x, int y, OGGConfigAudio *gui);
	int handle_event();
	OGGConfigAudio *gui;
};

class OGGVorbisMinBitrate : public BC_TextBox
{
public:
	OGGVorbisMinBitrate(int x, 
		int y, 
		OGGConfigAudio *gui, 
		char *text);
	int handle_event();
	OGGConfigAudio *gui;
};

class OGGVorbisMaxBitrate : public BC_TextBox
{
public:
	OGGVorbisMaxBitrate(int x, 
		int y, 
		OGGConfigAudio *gui, 
		char *text);
	int handle_event();
	OGGConfigAudio *gui;
};

class OGGVorbisAvgBitrate : public BC_TextBox
{
public:
	OGGVorbisAvgBitrate(int x, 
		int y, 
		OGGConfigAudio *gui, 
		char *text);
	int handle_event();
	OGGConfigAudio *gui;
};

class OGGConfigAudio: public BC_Window
{
public:
	OGGConfigAudio(BC_WindowBase *parent_window, Asset *asset);
	~OGGConfigAudio();

	void create_objects();
	void close_event();

	Asset *asset;
	OGGVorbisFixedBitrate *fixed_bitrate;
	OGGVorbisVariableBitrate *variable_bitrate;
private:
	BC_WindowBase *parent_window;
	char string[BCTEXTLEN];
};


class OGGTheoraBitrate : public BC_TextBox
{
public:
	OGGTheoraBitrate(int x, int y, OGGConfigVideo *gui);
	int handle_event();
	OGGConfigVideo *gui;
};

class OGGTheoraKeyframeFrequency : public BC_TumbleTextBox
{
public:
	OGGTheoraKeyframeFrequency(int x, int y, OGGConfigVideo *gui);
	int handle_event();
	OGGConfigVideo *gui;
};

class OGGTheoraKeyframeForceFrequency : public BC_TumbleTextBox
{
public:
	OGGTheoraKeyframeForceFrequency(int x, int y, OGGConfigVideo *gui);
	int handle_event();
	OGGConfigVideo *gui;
};

class OGGTheoraSharpness : public BC_TumbleTextBox
{
public:
	OGGTheoraSharpness(int x, int y, OGGConfigVideo *gui);
	int handle_event();
	OGGConfigVideo *gui;
};

class OGGTheoraFixedBitrate : public BC_Radial
{
public:
	OGGTheoraFixedBitrate(int x, int y, OGGConfigVideo *gui);
	int handle_event();
	OGGConfigVideo *gui;
};

class OGGTheoraFixedQuality : public BC_Radial
{
public:
	OGGTheoraFixedQuality(int x, int y, OGGConfigVideo *gui);
	int handle_event();
	OGGConfigVideo *gui;
};


class OGGConfigVideo: public BC_Window
{
public:
	OGGConfigVideo(BC_WindowBase *parent_window, Asset *asset);
	~OGGConfigVideo();

	void create_objects();
	void close_event();

	OGGTheoraFixedBitrate *fixed_bitrate;
	OGGTheoraFixedQuality *fixed_quality;
	Asset *asset;
private:
	BC_WindowBase *parent_window;
};

#endif
