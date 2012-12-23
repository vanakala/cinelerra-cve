
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

#ifndef FILEMOV_H
#define FILEMOV_H

#include "bitspopup.inc"
#include "condition.inc"
#include "filebase.h"
#include "file.inc"

#ifdef HAVE_STDLIB_H
#undef HAVE_STDLIB_H
#endif

#include "libmjpeg.h"
#include "mutex.inc"
#include "quicktime.h"
#include "thread.h"

class FileMOVThread;

class ThreadStruct
{
public:
	ThreadStruct();
	~ThreadStruct();

	void load_output(mjpeg_t *mjpeg);

	VFrame *input;
	unsigned char *output;  // Output buffer
	int output_size;        // Size of output buffer
	int output_allocated;  // Allocation of output buffer
	Condition *completion_lock;
};

class FileMOV : public FileBase
{
public:
	FileMOV(Asset *asset, File *file);
	~FileMOV();

	friend class FileMOVThread;

	static void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int options,
		const char *locked_compressor);
	static int check_sig(Asset *asset);
	static int supports(int format);

	int open_file(int rd, int wr);
	void close_file();
	void asset_to_format();
	void format_to_asset();
	framenum get_video_length();
	samplenum get_audio_length();
	int write_aframes(AFrame **frames);
	int write_frames(VFrame ***frames, int len);

	int read_frame(VFrame *frame);
	int read_aframe(AFrame *aframe);
	int prefer_samples_float();

// Direct copy routines
	static int get_best_colormodel(Asset *asset, int driver);
	int64_t get_memory_usage();
	int colormodel_supported(int colormodel);
	static const char *strtocompression(const char *string);
	static const char *compressiontostr(const char *string);

// Fix codec to what AVI or MOV support
	static void fix_codecs(Asset *asset);
// Check if resolutions match the DV codec when used
	int check_codec_params(Asset *asset);

// set programme timecode
	void set_frame_start(int64_t offset);

private:
	void new_audio_temp(int len);
	int quicktime_atracks;
	int quicktime_vtracks;

	framenum current_frame[MAX_CHANNELS];
	quicktime_t *fd;
	int depth;        // Depth in bits per pixel
	int64_t samples_correction;  // Correction after 32bit overflow

// An array of frames for threads to look up and compress on their own.
	ArrayList<ThreadStruct*> threadframes;

	int total_threadframes;     // Number of thread frames in this buffer
	int current_threadframe;    // Next threadframe to compress
	Mutex *threadframe_lock;     // Lock threadframe array.

	FileMOVThread **threads;   // One thread for every CPU
	char prefix_path[1024];    // Prefix for new file when 2G limit is exceeded
	int suffix_number;         // Number for new file

// Temp buffers for converting from double to float
	float **temp_float;
	int temp_allocated;

	float *channel_ptr[MAX_CHANNELS];
	int reading;
	int writing;
};


// Encoder thread to parallelize certain compression formats, mainly JPEG.
// Only works when no alpha.

class FileMOVThread : public Thread
{
public:
	FileMOVThread(FileMOV *filemov, int fields);
	~FileMOVThread();

	int start_encoding();
	int stop_encoding();
	int encode_buffer();
	void run();

	ThreadStruct *threadframe;    // The frame currently being processed.
	int done;
	FileMOV *filemov;
	Condition *input_lock;     // Wait for new array of threads or completion.
	mjpeg_t *mjpeg;
	int fields;
};


class MOVConfigAudioNum;
class MOVConfigAudioPopup;
class MOVConfigAudioToggle;

class MOVConfigAudio : public BC_Window
{
public:
	MOVConfigAudio(BC_WindowBase *parent_window, Asset *asset);
	~MOVConfigAudio();

	void create_objects();
	void close_event();
	void update_parameters();
	void reset();

	MOVConfigAudioPopup *compression_popup;
	ArrayList<BC_ListBoxItem*> compression_items;
	BitsPopup *bits_popup;
	BC_Title *bits_title;
	BC_CheckBox *dither;
	Asset *asset;
	MOVConfigAudioNum *vorbis_min_bitrate;
	MOVConfigAudioNum *vorbis_bitrate;
	MOVConfigAudioNum *vorbis_max_bitrate;
	MOVConfigAudioToggle *vorbis_vbr;

	MOVConfigAudioNum *mp3_bitrate;
	MOVConfigAudioNum *mp4a_bitrate;
	MOVConfigAudioNum *mp4a_quantqual;
};


class MOVConfigAudioPopup : public BC_PopupTextBox
{
public:
	MOVConfigAudioPopup(MOVConfigAudio *popup, int x, int y);
	int handle_event();
	MOVConfigAudio *popup;
};


class MOVConfigAudioToggle : public BC_CheckBox
{
public:
	MOVConfigAudioToggle(MOVConfigAudio *popup,
		char *title_text,
		int x,
		int y,
		int *output);
	int handle_event();
	int *output;
	MOVConfigAudio *popup;
};


class MOVConfigAudioNum : public BC_TumbleTextBox
{
public:
	MOVConfigAudioNum(MOVConfigAudio *popup, 
		char *title_text, 
		int x, 
		int y, 
		int *output);
	~MOVConfigAudioNum();
	
	void create_objects();
	int handle_event();
	int *output;
	MOVConfigAudio *popup;
	BC_Title *title;
	char *title_text;
	int x, y;
};


class MOVConfigVideoPopup;
class MOVConfigVideoNum;
class MOVConfigVideoFix;
class MOVConfigVideoFixBitrate;
class MOVConfigVideoFixQuant;
class MOVConfigVideoCheckBox;


class MOVConfigVideo : public BC_Window
{
public:
	MOVConfigVideo(BC_WindowBase *parent_window, 
		Asset *asset, 
		const char *locked_compressor);
	~MOVConfigVideo();

	void create_objects();
	void close_event();
	void reset();

	void update_parameters();

	ArrayList<BC_ListBoxItem*> compression_items;
	MOVConfigVideoPopup *compression_popup;
	Asset *asset;
	int param_x, param_y;
	const char *locked_compressor;

	BC_ISlider *jpeg_quality;
	BC_Title *jpeg_quality_title;

	MOVConfigVideoNum *ms_bitrate;
	MOVConfigVideoNum *ms_bitrate_tolerance;
	MOVConfigVideoNum *ms_quantization;
	MOVConfigVideoNum *ms_gop_size;
	MOVConfigVideoCheckBox *ms_interlaced;
	MOVConfigVideoFixBitrate *ms_fix_bitrate;
	MOVConfigVideoFixQuant *ms_fix_quant;

	MOVConfigVideoNum *divx_bitrate;
	MOVConfigVideoNum *divx_quantizer;
	MOVConfigVideoNum *divx_rc_period;
	MOVConfigVideoNum *divx_rc_reaction_ratio;
	MOVConfigVideoNum *divx_rc_reaction_period;
	MOVConfigVideoNum *divx_max_key_interval;
	MOVConfigVideoNum *divx_max_quantizer;
	MOVConfigVideoNum *divx_min_quantizer;
	MOVConfigVideoNum *divx_quality;
	MOVConfigVideoFixBitrate *divx_fix_bitrate;
	MOVConfigVideoFixQuant *divx_fix_quant;

	MOVConfigVideoNum *h264_bitrate;
	MOVConfigVideoNum *h264_quantizer;
	MOVConfigVideoFixBitrate *h264_fix_bitrate;
	MOVConfigVideoFixQuant *h264_fix_quant;
};

class MOVConfigVideoPopup : public BC_PopupTextBox
{
public:
	MOVConfigVideoPopup(MOVConfigVideo *popup, int x, int y);
	int handle_event();
	MOVConfigVideo *popup;
};

class MOVConfigVideoFixBitrate : public BC_Radial
{
public:
	MOVConfigVideoFixBitrate(int x, 
		int y,
		int *output,
		int value);
	int handle_event();
	BC_Radial *opposite;
	int *output;
	int value;
};

class MOVConfigVideoFixQuant : public BC_Radial
{
public:
	MOVConfigVideoFixQuant(int x, 
		int y,
		int *output,
		int value);
	int handle_event();
	BC_Radial *opposite;
	int *output;
	int value;
};

class MOVConfigVideoCheckBox : public BC_CheckBox
{
public:
	MOVConfigVideoCheckBox(char *title_text,
		int x,
		int y,
		int *output);
	int handle_event();
	int *output;
};

class MOVConfigVideoNum : public BC_TumbleTextBox
{
public:
	MOVConfigVideoNum(MOVConfigVideo *popup, 
		char *title_text, 
		int x, 
		int y, 
		int *output);
	MOVConfigVideoNum(MOVConfigVideo *popup, 
		char *title_text, 
		int x, 
		int y, 
		int min,
		int max,
		int *output);
	~MOVConfigVideoNum();
	
	void create_objects();
	int handle_event();
	int *output;
	MOVConfigVideo *popup;
	BC_Title *title;
	char *title_text;
	int x, y;
};

#endif
