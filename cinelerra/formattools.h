#ifndef FORMATTOOLS_H
#define FORMATTOOLS_H

#include "assets.inc"
#include "guicast.h"
#include "bitspopup.h"
#include "browsebutton.h"
#include "compresspopup.h"
#include "file.inc"
#include "formatpopup.h"
#include "mwindow.inc"

class FormatAParams;
class FormatVParams;
class FormatAThread;
class FormatVThread;
class FormatChannels;
class FormatPathButton;
class FormatPathText;
class FormatFormat;
class FormatAudio;
class FormatVideo;

class FormatTools
{
public:
	FormatTools(MWindow *mwindow,
				BC_WindowBase *window, 
				Asset *asset);
	~FormatTools();

	int create_objects(int &init_x, 
						int &init_y, 
						int do_audio,    // Include tools for audio
						int do_video,   // Include tools for video
						int prompt_audio,  // Include checkbox for audio
						int prompt_video,  // Include checkbox for video
						int prompt_audio_channels,
						int prompt_video_compression,
						int lock_compressor,  // Select compressors to be offered
						int recording, // Change captions for recording
						int *strategy,  // If nonzero, prompt for insertion strategy
						int brender); // Supply file formats for background rendering

	int set_audio_options();
	int set_video_options();

	BC_WindowBase *window;
	Asset *asset;

	FormatAParams *aparams_button;
	FormatVParams *vparams_button;
	FormatAThread *aparams_thread;
	FormatVThread *vparams_thread;
	BrowseButton *path_button;
	FormatPathText *path_textbox;
	FormatFormat *format_button;
	BC_TextBox *format_text;
	BC_ITumbler *channels_tumbler;

	FormatChannels *channels_button;
	FormatAudio *audio_switch;
	FormatVideo *video_switch;
	ArrayList<PluginServer*> *plugindb;
	MWindow *mwindow;
	int lock_compressor;
	int recording;
	int use_brender;
};



class FormatPathText : public BC_TextBox
{
public:
	FormatPathText(int x, int y, FormatTools *format, Asset *asset);
	~FormatPathText();
	int handle_event();
	
	Asset *asset;
	FormatTools *format;
};



class FormatFormat : public FormatPopup
{
public:
	FormatFormat(int x, int y, FormatTools *format, Asset *asset);
	~FormatFormat();
	
	int handle_event();
	FormatTools *format;
	Asset *asset;
};

class FormatAParams : public BC_Button
{
public:
	FormatAParams(MWindow *mwindow, FormatTools *format, int x, int y);
	~FormatAParams();
	int handle_event();
	FormatTools *format;
};

class FormatVParams : public BC_Button
{
public:
	FormatVParams(MWindow *mwindow, FormatTools *format, int x, int y);
	~FormatVParams();
	int handle_event();
	FormatTools *format;
};


class FormatAThread : public Thread
{
public:
	FormatAThread(FormatTools *format);
	~FormatAThread();
	
	void run();

	FormatTools *format;
	File *file;
};

class FormatVThread : public Thread
{
public:
	FormatVThread(FormatTools *format, int lock_compressor);
	~FormatVThread();
	
	void run();

	FormatTools *format;
	File *file;
	int lock_compressor;
};

class FormatAudio : public BC_CheckBox
{
public:
	FormatAudio(int x, int y, FormatTools *format, int default_);
	~FormatAudio();
	int handle_event();
	FormatTools *format;
};

class FormatVideo : public BC_CheckBox
{
public:
	FormatVideo(int x, int y, FormatTools *format, int default_);
	~FormatVideo();
	int handle_event();
	FormatTools *format;
};


class FormatChannels : public BC_TextBox
{
public:
	FormatChannels(int x, int y, Asset *asset);
	~FormatChannels();
	int handle_event();

	Asset *asset;
};

class FormatToTracks : public BC_CheckBox
{
public:
	FormatToTracks(int x, int y, int *output);
	~FormatToTracks();
	int handle_event();
	int *output;
};

class FormatMultiple : public BC_CheckBox
{
public:
	FormatMultiple(MWindow *mwindow, int x, int y, int *output);
	~FormatMultiple();
	int handle_event();
	int *output;
	MWindow *mwindow;
};


#endif
