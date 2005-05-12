#ifndef FILEOGG_H
#define FILEOGG_H

#include "../config.h"
#include "filebase.h"
#include "file.inc"

#include <theora/theora.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>


#include <libdv/dv.h>

/* This code was aspired by ffmpeg2theora */
/* Special thanks for help on this code goes out to j@v2v.cc */


typedef struct
{
    ogg_page videopage;
    ogg_page audiopage;
    int audioflag;
    int videoflag;
    double audiotime;
    double videotime;
    ogg_int64_t audio_bytesout;
    ogg_int64_t video_bytesout;

    ogg_stream_state to;    /* take physical pages, weld into a logical
                             * stream of packets */
    ogg_stream_state vo;    /* take physical pages, weld into a logical
                             * stream of packets */
    ogg_page og;    /* one Ogg bitstream page.  Vorbis packets are inside */
    ogg_packet op;  /* one raw packet of data for decode */

    theora_info ti;
    theora_comment tc;
    
    theora_state td;


    vorbis_info vi;       /* struct that stores all the static vorbis bitstream settings */
    vorbis_comment vc;    /* struct that stores all the user comments */

    vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
    vorbis_block vb;     /* local working space for packet->PCM decode */
}
theoraframes_info_t;

class FileOGG : public FileBase
{
public:
	FileOGG(Asset *asset, File *file);
	~FileOGG();

	static void get_parameters(BC_WindowBase *parent_window,
		Asset *asset,
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options);
	
	int reset_parameters_derived();
	int open_file(int rd, int wr);
	static int check_sig(Asset *asset);
	int close_file();
	int close_file_derived();
	int64_t get_video_position();
	int64_t get_audio_position();
	int set_video_position(int64_t x);
	int set_audio_position(int64_t x);
	int write_samples(double **buffer, int64_t len);
	int write_frames(VFrame ***frames, int len);
private:
	int write_samples_vorbis(double **buffer, int64_t len, int e_o_s);
	int write_frames_theora(VFrame ***frames, int len, int e_o_s);
	int flush_ogg(int e_o_s);
	
	FILE *stream;
	
	theoraframes_info_t *tf;
	VFrame *temp_frame;
	Mutex *flush_lock;	
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

	int create_objects();
	int close_event();

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

	int create_objects();
	int close_event();

	OGGTheoraFixedBitrate *fixed_bitrate;
	OGGTheoraFixedQuality *fixed_quality;
	Asset *asset;
private:
	BC_WindowBase *parent_window;
};


#endif
