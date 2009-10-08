
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

#include "asset.h"
#include "guicast.h"
#include "file.h"
#include "formattools.h"
#include "language.h"
#include "maxchannels.h"
#include "mwindow.h"
#include "preferences.h"
#include "quicktime.h"
#include "theme.h"
#include "videodevice.inc"
#include <string.h>
#include "pipe.h"


FormatTools::FormatTools(MWindow *mwindow,
				BC_WindowBase *window, 
				Asset *asset)
{
	this->mwindow = mwindow;
	this->window = window;
	this->asset = asset;
	this->plugindb = mwindow->plugindb;

	aparams_button = 0;
	vparams_button = 0;
	aparams_thread = 0;
	vparams_thread = 0;
	channels_tumbler = 0;
	path_textbox = 0;
	path_button = 0;
	path_recent = 0;
	w = 0;
}

FormatTools::~FormatTools()
{
	delete path_recent;
	delete path_button;
	delete path_textbox;
	delete format_button;

	if(aparams_button) delete aparams_button;
	if(vparams_button) delete vparams_button;
	if(aparams_thread) delete aparams_thread;
	if(vparams_thread) delete vparams_thread;
	if(channels_tumbler) delete channels_tumbler;
}

int FormatTools::create_objects(int &init_x, 
						int &init_y, 
						int do_audio,    // Include support for audio
						int do_video,   // Include support for video
						int prompt_audio,  // Include checkbox for audio
						int prompt_video,
						int prompt_audio_channels,
						int prompt_video_compression,
						char *locked_compressor,
						int recording,
						int *strategy,
						int brender)
{
	int x = init_x;
	int y = init_y;

	this->locked_compressor = locked_compressor;
	this->recording = recording;
	this->use_brender = brender;
	this->do_audio = do_audio;
	this->do_video = do_video;
	this->prompt_audio = prompt_audio;
	this->prompt_audio_channels = prompt_audio_channels;
	this->prompt_video = prompt_video;
	this->prompt_video_compression = prompt_video_compression;
	this->strategy = strategy;

//printf("FormatTools::create_objects 1\n");

// Modify strategy depending on render farm
	if(strategy)
	{
		if(mwindow->preferences->use_renderfarm)
		{
			if(*strategy == FILE_PER_LABEL)
				*strategy = FILE_PER_LABEL_FARM;
			else
			if(*strategy == SINGLE_PASS)
				*strategy = SINGLE_PASS_FARM;
		}
		else
		{
			if(*strategy == FILE_PER_LABEL_FARM)
				*strategy = FILE_PER_LABEL;
			else
			if(*strategy == SINGLE_PASS_FARM)
				*strategy = SINGLE_PASS;
		}
	}

//printf("FormatTools::create_objects 1\n");
	if(!recording)
	{
		window->add_subwindow(path_textbox = new FormatPathText(x, y, this));
		x += 305;
		path_recent = new BC_RecentList("PATH", mwindow->defaults,
					path_textbox, 10, x, y, 300, 100);
		window->add_subwindow(path_recent);
		path_recent->load_items(FILE_FORMAT_PREFIX(asset->format));

		x += 18;
		window->add_subwindow(path_button = new BrowseButton(
			mwindow,
			window,
			path_textbox, 
			x, 
			y, 
			asset->path,
			_("Output to file"),
			_("Select a file to write to:"),
			0));

// Set w for user.
		w = x + path_button->get_w() + 5;
		x -= 305;

		y += 35;
	}
	else
		w = x + 305;

	window->add_subwindow(format_title = new BC_Title(x, y, _("File Format:")));
	x += 90;
	window->add_subwindow(format_text = new BC_TextBox(x, 
		y, 
		200, 
		1, 
		File::formattostr(asset->format)));
	x += format_text->get_w();
	window->add_subwindow(format_button = new FormatFormat(x, 
		y, 
		this));
	format_button->create_objects();

	x = init_x;
	y += format_button->get_h() + 10;
	if(do_audio)
	{
		window->add_subwindow(audio_title = new BC_Title(x, y, _("Audio:"), LARGEFONT,  BC_WindowBase::get_resources()->audiovideo_color));
		x += 80;
		window->add_subwindow(aparams_button = new FormatAParams(mwindow, this, x, y));
		x += aparams_button->get_w() + 10;
		if(prompt_audio) 
		{
			window->add_subwindow(audio_switch = new FormatAudio(x, y, this, asset->audio_data));
		}
		x = init_x;
		y += aparams_button->get_h() + 20;

// Audio channels only used for recording.
// 		if(prompt_audio_channels)
// 		{
// 			window->add_subwindow(channels_title = new BC_Title(x, y, _("Number of audio channels to record:")));
// 			x += 260;
// 			window->add_subwindow(channels_button = new FormatChannels(x, y, this));
// 			x += channels_button->get_w() + 5;
// 			window->add_subwindow(channels_tumbler = new BC_ITumbler(channels_button, 1, MAXCHANNELS, x, y));
// 			y += channels_button->get_h() + 20;
// 			x = init_x;
// 		}

//printf("FormatTools::create_objects 6\n");
		aparams_thread = new FormatAThread(this);
	}

//printf("FormatTools::create_objects 7\n");
	if(do_video)
	{

//printf("FormatTools::create_objects 8\n");
		window->add_subwindow(video_title = new BC_Title(x, y, _("Video:"), LARGEFONT,  BC_WindowBase::get_resources()->audiovideo_color));
		x += 80;
		if(prompt_video_compression)
		{
			window->add_subwindow(vparams_button = new FormatVParams(mwindow, this, x, y));
			x += vparams_button->get_w() + 10;
		}

//printf("FormatTools::create_objects 9\n");
		if(prompt_video)
		{
			window->add_subwindow(video_switch = new FormatVideo(x, y, this, asset->video_data));
			y += video_switch->get_h();
		}
		else
		{
			y += vparams_button->get_h();
		}

//printf("FormatTools::create_objects 10\n");
		y += 10;
		vparams_thread = new FormatVThread(this);
	}

//printf("FormatTools::create_objects 11\n");

	x = init_x;
	if(strategy)
	{
		window->add_subwindow(multiple_files = new FormatMultiple(mwindow, x, y, strategy));
		y += multiple_files->get_h() + 10;
	}

//printf("FormatTools::create_objects 12\n");

	init_y = y;
	return 0;
}

void FormatTools::update_driver(int driver)
{
	this->video_driver = driver;

	switch(driver)
	{
		case CAPTURE_DVB:
// Just give the user information about how the stream is going to be
// stored but don't change the asset.
// Want to be able to revert to user settings.
			if(asset->format != FILE_MPEG)
			{
				format_text->update(_("MPEG transport stream"));
				asset->format = FILE_MPEG;
			}
			locked_compressor = 0;
			audio_switch->update(1);
			video_switch->update(1);
			break;

		case CAPTURE_IEC61883:
		case CAPTURE_FIREWIRE:
			if(asset->format != FILE_AVI &&
				asset->format != FILE_MOV)
			{
				format_text->update(MOV_NAME);
				asset->format = FILE_MOV;
			}
			else
				format_text->update(File::formattostr(asset->format));
			locked_compressor = QUICKTIME_DVSD;
			strcpy(asset->vcodec, QUICKTIME_DVSD);
			audio_switch->update(asset->audio_data);
			video_switch->update(asset->video_data);
			break;

		case CAPTURE_BUZ:
		case VIDEO4LINUX2JPEG:
			if(asset->format != FILE_AVI &&
				asset->format != FILE_MOV)
			{
				format_text->update(MOV_NAME);
				asset->format = FILE_MOV;
			}
			else
				format_text->update(File::formattostr(asset->format));
			locked_compressor = QUICKTIME_MJPA;
			audio_switch->update(asset->audio_data);
			video_switch->update(asset->video_data);
			break;

		default:
			format_text->update(File::formattostr(asset->format));
			locked_compressor = 0;
			audio_switch->update(asset->audio_data);
			video_switch->update(asset->video_data);
			break;
	}
	close_format_windows();
}



int FormatTools::handle_event()
{
	return 0;
}

Asset* FormatTools::get_asset()
{
	return asset;
}

void FormatTools::update_extension()
{
	char *extension = File::get_tag(asset->format);
	if(extension)
	{
		char *ptr = strrchr(asset->path, '.');
		if(!ptr)
		{
			ptr = asset->path + strlen(asset->path);
			*ptr = '.';
		}
		ptr++;
		sprintf(ptr, extension);

		int character1 = ptr - asset->path;
		int character2 = ptr - asset->path + strlen(extension);
		*(asset->path + character2) = 0;
		if(path_textbox) 
		{
			path_textbox->update(asset->path);
			path_textbox->set_selection(character1, character2, character2);
		}
	}
}

void FormatTools::update(Asset *asset, int *strategy)
{
	this->asset = asset;
	this->strategy = strategy;

	if(path_textbox) 
		path_textbox->update(asset->path);
	format_text->update(File::formattostr(plugindb, asset->format));
	if(do_audio && audio_switch) audio_switch->update(asset->audio_data);
	if(do_video && video_switch) video_switch->update(asset->video_data);
	if(strategy)
	{
		multiple_files->update(strategy);
	}
	close_format_windows();
}

void FormatTools::close_format_windows()
{
	if(aparams_thread) aparams_thread->file->close_window();
	if(vparams_thread) vparams_thread->file->close_window();
}

int FormatTools::get_w()
{
	return w;
}

void FormatTools::reposition_window(int &init_x, int &init_y)
{
	int x = init_x;
	int y = init_y;

	if(path_textbox) 
	{
		path_textbox->reposition_window(x, y);
		x += 305;
		path_button->reposition_window(x, y);
		x -= 305;
		y += 35;
	}

	format_title->reposition_window(x, y);
	x += 90;
	format_text->reposition_window(x, y);
	x += format_text->get_w();
	format_button->reposition_window(x, y);

	x = init_x;
	y += format_button->get_h() + 10;

	if(do_audio)
	{
		audio_title->reposition_window(x, y);
		x += 80;
		aparams_button->reposition_window(x, y);
		x += aparams_button->get_w() + 10;
		if(prompt_audio) audio_switch->reposition_window(x, y);

		x = init_x;
		y += aparams_button->get_h() + 20;
		if(prompt_audio_channels)
		{
			channels_title->reposition_window(x, y);
			x += 260;
			channels_button->reposition_window(x, y);
			x += channels_button->get_w() + 5;
			channels_tumbler->reposition_window(x, y);
			y += channels_button->get_h() + 20;
			x = init_x;
		}
	}


	if(do_video)
	{
		video_title->reposition_window(x, y);
		x += 80;
		if(prompt_video_compression)
		{
			vparams_button->reposition_window(x, y);
			x += vparams_button->get_w() + 10;
		}

		if(prompt_video)
		{
			video_switch->reposition_window(x, y);
			y += video_switch->get_h();
		}
		else
		{
			y += vparams_button->get_h();
		}

		y += 10;
		x = init_x;
	}

	if(strategy)
	{
		multiple_files->reposition_window(x, y);
		y += multiple_files->get_h() + 10;
	}

	init_y = y;
}


int FormatTools::set_audio_options()
{
//	if(video_driver == CAPTURE_DVB)
//	{
//		return 0;
//	}

	if(!aparams_thread->running())
	{
		aparams_thread->start();
	}
	else
	{
		aparams_thread->file->raise_window();
	}
	return 0;
}

int FormatTools::set_video_options()
{
//	if(video_driver == CAPTURE_DVB)
//	{
//		return 0;
//	}

	if(!vparams_thread->running())
	{
		vparams_thread->start();
	}
	else
	{
		vparams_thread->file->raise_window();
	}

	return 0;
}





FormatAParams::FormatAParams(MWindow *mwindow, FormatTools *format, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("wrench"))
{
	this->format = format;
	set_tooltip(_("Configure audio compression"));
}
FormatAParams::~FormatAParams() 
{
}
int FormatAParams::handle_event() 
{
	format->set_audio_options(); 
}

FormatVParams::FormatVParams(MWindow *mwindow, FormatTools *format, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("wrench"))
{ 
	this->format = format; 
	set_tooltip(_("Configure video compression"));
}
FormatVParams::~FormatVParams() 
{
}
int FormatVParams::handle_event() 
{ 
	format->set_video_options(); 
}


FormatAThread::FormatAThread(FormatTools *format)
 : Thread()
{ 
	this->format = format; 
	file = new File;
}

FormatAThread::~FormatAThread() 
{
	delete file;
}

void FormatAThread::run()
{
	file->get_options(format, 1, 0);
}




FormatVThread::FormatVThread(FormatTools *format)
 : Thread()
{
	this->format = format;
	file = new File;
}

FormatVThread::~FormatVThread() 
{
	delete file;
}

void FormatVThread::run()
{
	file->get_options(format, 0, 1);
}

FormatPathText::FormatPathText(int x, int y, FormatTools *format)
 : BC_TextBox(x, y, 300, 1, format->asset->path) 
{
	this->format = format; 
}
FormatPathText::~FormatPathText() 
{
}
int FormatPathText::handle_event() 
{
	strcpy(format->asset->path, get_text());
	format->handle_event();
}




FormatAudio::FormatAudio(int x, int y, FormatTools *format, int default_)
 : BC_CheckBox(x, 
 	y, 
	default_, 
	(char*)(format->recording ? _("Record audio tracks") : _("Render audio tracks")))
{ 
	this->format = format; 
}
FormatAudio::~FormatAudio() {}
int FormatAudio::handle_event()
{
	format->asset->audio_data = get_value();
}


FormatVideo::FormatVideo(int x, int y, FormatTools *format, int default_)
 : BC_CheckBox(x, 
 	y, 
	default_, 
	(char*)(format->recording ? _("Record video tracks") : _("Render video tracks")))
{
this->format = format; 
}
FormatVideo::~FormatVideo() {}
int FormatVideo::handle_event()
{
	format->asset->video_data = get_value();
}




FormatFormat::FormatFormat(int x, 
	int y, 
	FormatTools *format)
 : FormatPopup(format->plugindb, 
 	x, 
	y,
	format->use_brender)
{ 
	this->format = format; 
}
FormatFormat::~FormatFormat() 
{
}
int FormatFormat::handle_event()
{
	if(get_selection(0, 0) >= 0)
	{
		int new_format = File::strtoformat(format->plugindb, get_selection(0, 0)->get_text());
		if(new_format != format->asset->format)
		{
			format->asset->format = new_format;
			format->format_text->update(get_selection(0, 0)->get_text());
			format->update_extension();
			format->close_format_windows();
			if (format->path_recent)
				format->path_recent->load_items
					(FILE_FORMAT_PREFIX(format->asset->format));
		}
	}
	return 1;
}



FormatChannels::FormatChannels(int x, int y, FormatTools *format)
 : BC_TextBox(x, y, 100, 1, format->asset->channels) 
{ 
 	this->format = format; 
}
FormatChannels::~FormatChannels() 
{
}
int FormatChannels::handle_event() 
{
	format->asset->channels = atol(get_text());
	return 1;
}

FormatToTracks::FormatToTracks(int x, int y, int *output)
 : BC_CheckBox(x, y, *output, _("Overwrite project with output"))
{ 
	this->output = output; 
}
FormatToTracks::~FormatToTracks() 
{
}
int FormatToTracks::handle_event()
{
	*output = get_value();
	return 1;
}


FormatMultiple::FormatMultiple(MWindow *mwindow, int x, int y, int *output)
 : BC_CheckBox(x, 
 	y, 
	(*output == FILE_PER_LABEL) || (*output == FILE_PER_LABEL_FARM), 
	_("Create new file at each label"))
{ 
	this->output = output;
	this->mwindow = mwindow;
}
FormatMultiple::~FormatMultiple() 
{
}
int FormatMultiple::handle_event()
{
	if(get_value())
	{
		if(mwindow->preferences->use_renderfarm)
			*output = FILE_PER_LABEL_FARM;
		else
			*output = FILE_PER_LABEL;
	}
	else
	{
		if(mwindow->preferences->use_renderfarm)
			*output = SINGLE_PASS_FARM;
		else
			*output = SINGLE_PASS;
	}
	return 1;
}

void FormatMultiple::update(int *output)
{
	this->output = output;
	if(*output == FILE_PER_LABEL_FARM ||
		*output ==FILE_PER_LABEL)
		set_value(1);
	else
		set_value(0);
}


