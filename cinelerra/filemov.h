#ifndef FILEMOV_H
#define FILEMOV_H

#include "bitspopup.inc"
#include "file.inc"
#include "filebase.h"
#include "mutex.h"
#include "thread.h"

// ./Quicktime
#include "libmjpeg.h"
#include "quicktime.h"

class FileMOVThread;

class ThreadStruct
{
public:
	ThreadStruct();
	~ThreadStruct();

	void load_output(mjpeg_t *mjpeg);

	VFrame *input;
	unsigned char *output;  // Output buffer
	int64_t output_size;        // Size of output buffer
	int64_t output_allocated;  // Allocation of output buffer
	Mutex completion_lock;
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
		int audio_options,
		int video_options,
		int lock_compressor);
	static int check_sig(Asset *asset);

	int open_file(int rd, int wr);
	int close_file();
	void asset_to_format();
	void format_to_asset();
	int64_t get_video_length();
	int64_t get_audio_length();
	int set_video_position(int64_t x);
	int set_audio_position(int64_t x);
	int write_samples(double **buffer, 
			int64_t len);
	int write_frames(VFrame ***frames, int len);
	int64_t compressed_frame_size();
	int read_compressed_frame(VFrame *buffer);
	int write_compressed_frame(VFrame *buffer);

	int read_frame(VFrame *frame);
	int read_samples(double *buffer, int64_t len);

// Direct copy routines
	static int get_best_colormodel(Asset *asset, int driver);
	int colormodel_supported(int colormodel);
	int can_copy_from(Edit *edit, int64_t position); // This file can copy frames directly from the asset
	static char *strtocompression(char *string);
	static char *compressiontostr(char *string);

// Fix codec to what AVI or MOV support
	static void fix_codecs(Asset *asset);

private:
	void new_audio_temp(int64_t len);
// read raw audio data
	int read_raw(char *buffer, int64_t samples, int track);  
// overlay raw frame from the current layer and position
	int read_raw(VFrame *frame, 
		float in_x1, float in_y1, float in_x2, float in_y2,
		float out_x1, float out_y1, float out_x2, float out_y2, 
		int use_float, int interpolate);
	int reset_parameters_derived();
	int quicktime_atracks;
	int quicktime_vtracks;
// current positions for when the file descriptor doesn't have the right position
	quicktime_t *fd;
// frame to return through read_frame
	VFrame *frame; 
// Temporary storage for color conversions
	VFrame *temp_frame;
	int depth;        // Depth in bits per pixel
	int64_t frames_correction;  // Correction after 32bit overflow
	int64_t samples_correction;  // Correction after 32bit overflow

// An array of frames for threads to look up and compress on their own.
	ArrayList<ThreadStruct*> threadframes;

	int total_threadframes;     // Number of thread frames in this buffer
	int current_threadframe;    // Next threadframe to compress
	Mutex threadframe_lock;     // Lock threadframe array.

	FileMOVThread **threads;   // One thread for every CPU
	char prefix_path[1024];    // Prefix for new file when 2G limit is exceeded
	int suffix_number;         // Number for new file

// Temp buffers for converting from double to float
	float **temp_float;
	int64_t temp_allocated;
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
	Mutex input_lock;     // Wait for new array of threads or completion.
	Mutex quicktime_lock;  // Lock out reopen_file
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

	int create_objects();
	int close_event();
	void update_parameters();

	MOVConfigAudioPopup *compression_popup;
	ArrayList<BC_ListBoxItem*> compression_items;
	BC_WindowBase *parent_window;
	BitsPopup *bits_popup;
	BC_Title *bits_title;
	BC_CheckBox *dither;
	Asset *asset;
	MOVConfigAudioNum *vorbis_min_bitrate;
	MOVConfigAudioNum *vorbis_bitrate;
	MOVConfigAudioNum *vorbis_max_bitrate;
	MOVConfigAudioToggle *vorbis_vbr;

	MOVConfigAudioNum *mp3_bitrate;
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
		int lock_compressor);
	~MOVConfigVideo();

	int create_objects();
	int close_event();
	void reset();

	void update_parameters();

	ArrayList<BC_ListBoxItem*> compression_items;
	MOVConfigVideoPopup *compression_popup;
	BC_WindowBase *parent_window;
	Asset *asset;
	int param_x, param_y;
	int lock_compressor;
	
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
