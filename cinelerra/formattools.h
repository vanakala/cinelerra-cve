
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

#ifndef FORMATTOOLS_H
#define FORMATTOOLS_H

#include "asset.inc"
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
class FormatMultiple;

class FormatTools
{
public:
	FormatTools(MWindow *mwindow,
				BC_WindowBase *window, 
				Asset *asset);
	virtual ~FormatTools();

	int create_objects(int &init_x, 
						int &init_y, 
						int do_audio,    // Include tools for audio
						int do_video,   // Include tools for video
						int prompt_audio,  // Include checkbox for audio
						int prompt_video,  // Include checkbox for video
						int prompt_audio_channels,
						int prompt_video_compression,
						char *locked_compressor,  // Select compressors to be offered
						int recording, // Change captions for recording
						int *strategy,  // If nonzero, prompt for insertion strategy
						int brender); // Supply file formats for background rendering
// In recording preferences, aspects of the format are locked 
// depending on the driver used.
	void update_driver(int driver);


	void reposition_window(int &init_x, int &init_y);
// Put new asset's parameters in and change asset.
	void update(Asset *asset, int *strategy);
// Update filename extension when format is changed.
	void update_extension();
	void close_format_windows();
	Asset* get_asset();

// Handle change in path text.  Used in BatchRender.
	virtual int handle_event();

	int set_audio_options();
	int set_video_options();
	int get_w();

	BC_WindowBase *window;
	Asset *asset;

	FormatAParams *aparams_button;
	FormatVParams *vparams_button;
	FormatAThread *aparams_thread;
	FormatVThread *vparams_thread;
	BrowseButton *path_button;
	FormatPathText *path_textbox;
	BC_RecentList *path_recent;
	BC_Title *format_title;
	FormatFormat *format_button;
	BC_TextBox *format_text;
	BC_ITumbler *channels_tumbler;

	BC_Title *audio_title;
	BC_Title *channels_title;
	FormatChannels *channels_button;
	FormatAudio *audio_switch;

	BC_Title *video_title;
	FormatVideo *video_switch;

	FormatMultiple *multiple_files;

	ArrayList<PluginServer*> *plugindb;
	MWindow *mwindow;
	char *locked_compressor;
	int recording;
	int use_brender;
	int do_audio;
	int do_video;
	int prompt_audio;
	int prompt_audio_channels;
	int prompt_video;
	int prompt_video_compression;
	int *strategy;
	int w;
// Determines what the configuration buttons do.
	int video_driver;
};



class FormatPathText : public BC_TextBox
{
public:
	FormatPathText(int x, int y, FormatTools *format);
	~FormatPathText();
	int handle_event();
	
	FormatTools *format;
};



class FormatFormat : public FormatPopup
{
public:
	FormatFormat(int x, int y, FormatTools *format);
	~FormatFormat();
	
	int handle_event();
	FormatTools *format;
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
	FormatVThread(FormatTools *format);
	~FormatVThread();
	
	void run();

	FormatTools *format;
	File *file;
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
	FormatChannels(int x, int y, FormatTools *format);
	~FormatChannels();
	int handle_event();
	FormatTools *format;
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
	void update(int *output);
	int *output;
	MWindow *mwindow;
};


#endif
