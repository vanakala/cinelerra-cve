#ifndef FILEMPEG_H
#define FILEMPEG_H

#include "bitspopup.inc"
#include "file.inc"
#include "filebase.h"
#include "libmpeg3.h"
#include "mutex.h"
#include "resample.inc"
#include "thread.h"
#include "threadfork.inc"






class FileMPEG : public FileBase
{
public:
	FileMPEG(Asset *asset, File *file, ArrayList<PluginServer*> *plugindb);
	~FileMPEG();

	static void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options);

	static int check_sig(Asset *asset);
	int open_file(int rd, int wr);
	int close_file();
	int set_video_position(long x);
	int set_audio_position(long x);
	int write_samples(double **buffer, 
			long len);
	int write_frames(VFrame ***frames, int len);

	int read_frame(VFrame *frame);
	int read_samples(double *buffer, long len);

// Direct copy routines
	static int get_best_colormodel(Asset *asset, int driver);
	int colormodel_supported(int colormodel);
// This file can copy frames directly from the asset
	int can_copy_from(Edit *edit, long position); 
	static char *strtocompression(char *string);
	static char *compressiontostr(char *string);

private:
	void to_streamchannel(int channel, int &stream_out, int &channel_out);
	int reset_parameters_derived();
// File descriptor for decoder
	mpeg3_t *fd;

// Fork for video encoder
	ThreadFork *video_out;

// Fork for audio encoder
	ThreadFork *audio_out;
	
	ArrayList<PluginServer*> *plugindb;

// Temporary for color conversion
	VFrame *temp_frame;
//	long last_sample;
	
	unsigned char *audio_temp;
	long audio_allocation;
};


class MPEGConfigAudioPopup;
class MPEGABitrate;


class MPEGConfigAudio : public BC_Window
{
public:
	MPEGConfigAudio(BC_WindowBase *parent_window, Asset *asset);
	~MPEGConfigAudio();

	int create_objects();
	int close_event();

	BC_WindowBase *parent_window;
	MPEGABitrate *bitrate;
	char string[BCTEXTLEN];
	Asset *asset;
};


class MPEGLayer : public BC_PopupMenu
{
public:
	MPEGLayer(int x, int y, MPEGConfigAudio *gui);
	void create_objects();
	int handle_event();
	static int string_to_layer(char *string);
	static char* layer_to_string(int derivative);
	
	MPEGConfigAudio *gui;
};

class MPEGABitrate : public BC_PopupMenu
{
public:
	MPEGABitrate(int x, int y, MPEGConfigAudio *gui);

	void create_objects();
	void set_layer(int layer);

	int handle_event();
	static int string_to_bitrate(char *string);
	static char* bitrate_to_string(char *string, int bitrate);
	
	MPEGConfigAudio *gui;
};



class MPEGFixedBitrate;
class MPEGFixedQuant;

class MPEGConfigVideo : public BC_Window
{
public:
	MPEGConfigVideo(BC_WindowBase *parent_window, 
		Asset *asset);
	~MPEGConfigVideo();

	int create_objects();
	int close_event();

	BC_WindowBase *parent_window;
	Asset *asset;
	MPEGFixedBitrate *fixed_bitrate;
	MPEGFixedQuant *fixed_quant;
	
};

class MPEGDerivative : public BC_PopupMenu
{
public:
	MPEGDerivative(int x, int y, MPEGConfigVideo *gui);
	void create_objects();
	int handle_event();
	static int string_to_derivative(char *string);
	static char* derivative_to_string(int derivative);
	
	MPEGConfigVideo *gui;
};

class MPEGColorModel : public BC_PopupMenu
{
public:
	MPEGColorModel(int x, int y, MPEGConfigVideo *gui);
	void create_objects();
	int handle_event();
	static int string_to_cmodel(char *string);
	static char* cmodel_to_string(int cmodel);
	
	MPEGConfigVideo *gui;
};

class MPEGBitrate : public BC_TextBox
{
public:
	MPEGBitrate(int x, int y, MPEGConfigVideo *gui);
	int handle_event();
	MPEGConfigVideo *gui;
};

class MPEGQuant : public BC_TumbleTextBox
{
public:
	MPEGQuant(int x, int y, MPEGConfigVideo *gui);
	int handle_event();
	MPEGConfigVideo *gui;
};

class MPEGIFrameDistance : public BC_TumbleTextBox
{
public:
	MPEGIFrameDistance(int x, int y, MPEGConfigVideo *gui);
	int handle_event();
	MPEGConfigVideo *gui;
};

class MPEGFixedBitrate : public BC_Radial
{
public:
	MPEGFixedBitrate(int x, int y, MPEGConfigVideo *gui);
	int handle_event();
	MPEGConfigVideo *gui;
};

class MPEGFixedQuant : public BC_Radial
{
public:
	MPEGFixedQuant(int x, int y, MPEGConfigVideo *gui);
	int handle_event();
	MPEGConfigVideo *gui;
};

class MPEGSeqCodes : public BC_CheckBox
{
public:
	MPEGSeqCodes(int x, int y, MPEGConfigVideo *gui);
	int handle_event();
	MPEGConfigVideo *gui;
};



#endif
