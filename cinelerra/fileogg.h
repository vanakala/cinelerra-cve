
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

typedef struct
{
	ogg_page audiopage;
	ogg_page videopage;

	double audiotime;
	double videotime;
	ogg_int64_t audio_bytesout;
	ogg_int64_t video_bytesout;

	ogg_page og;    /* one Ogg bitstream page.  Vorbis packets are inside */
	ogg_packet op;  /* one raw packet of data for decode */

	theora_info ti;
	theora_comment tc;
	theora_state td;

	vorbis_info vi;       /* struct that stores all the static vorbis bitstream settings */
	vorbis_comment vc;    /* struct that stores all the user comments */
	vorbis_dsp_state vd; /* central working state for the packet<->PCM encoder/decoder */
	vorbis_block vb;     /* local working space for packet<->PCM encode/decode */

	/* used for muxing */
	ogg_stream_state to;    /* take physical pages, weld into a logical stream of packets */
	ogg_stream_state vo;    /* take physical pages, weld into a logical stream of packets */

	int apage_valid;
	int vpage_valid;
	unsigned char *apage;
	unsigned char *vpage;
	int vpage_len;
	int apage_len;
	int vpage_buffer_length;
	int apage_buffer_length;


// stuff needed for reading only
	sync_window_t *audiosync;
	sync_window_t *videosync;

// to do some manual page flusing
	int v_pkg;
	int a_pkg;

} theoraframes_info_t;

class FileOGG : public FileBase
{
friend class PackagingEngineOGG;
public:
	FileOGG(Asset *asset, File *file);
	~FileOGG();

	static void get_parameters(BC_WindowBase *parent_window,
		Asset *asset,
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options);

	int open_file(int rd, int wr);
	static int check_sig(Asset *asset);
	void close_file();

	void set_audio_position(samplenum x);
	int colormodel_supported(int colormodel);
	int get_best_colormodel(Asset *asset, int driver);
	int write_samples(double **buffer, int len);
	int write_frames(VFrame ***frames, int len);
	int read_samples(double *buffer, int len);
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
	void write_samples_vorbis(double **buffer, int len, int e_o_s);
	void write_frames_theora(VFrame ***frames, int len, int e_o_s);
	void flush_ogg(int e_o_s);
	int write_audio_page();
	int write_video_page();

	void move_pcmsamples_position();
	int copy_to_pcmsamples(float **smpls, int start, int smplen);
	int fill_pcm_samples(int len);

	FILE *stream;
	FileTOC *tocfile;

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

	theoraframes_info_t *tf;
	VFrame *temp_frame;
	Mutex *flush_lock;

	off_t filedata_begin;

	ogg_int64_t start_sample; // first and last sample inside this file
	ogg_int64_t last_sample;
	ogg_int64_t start_frame; // first and last frame inside this file
	ogg_int64_t last_frame;

	samplenum sample_position;  // what will be the next sample taken from vorbis decoder
	samplenum next_sample_position; // what is the next sample read_samples must deliver

	int theora_cmodel;
	framenum frame_position;    // LAST decoded frame position
	char theora_keyframe_granule_shift;
	int final_write;
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

class PackagingEngineOGG : public PackagingEngine
{
public:
	PackagingEngineOGG();
	~PackagingEngineOGG();
	int create_packages_single_farm(
		EDL *edl,
		Preferences *preferences,
		Asset *default_asset, 
		ptstime total_start,
		ptstime total_end);
	RenderPackage* get_package_single_farm(double frames_per_second, 
		int client_number,
		int use_local_rate);
	ptstime get_progress_max();
	void get_package_paths(ArrayList<char*> *path_list);
	int packages_are_done();

private:
	EDL *edl;

	RenderPackage **packages;
	int total_packages;
	double video_package_len;    // Target length of a single package

	Asset *default_asset;
	Preferences *preferences;
	int current_package;
	ptstime total_start;
	ptstime total_end;
	ptstime audio_pts;
	ptstime audio_start_pts;
	ptstime audio_end_pts;
	ptstime video_pts;
	ptstime video_start_pts;
	ptstime video_end_pts;
};

#endif
