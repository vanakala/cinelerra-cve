#include "assets.h"
#include "guicast.h"
#include "file.h"
#include "formattools.h"
#include "maxchannels.h"
#include "mwindow.h"
#include "preferences.h"
#include "theme.h"
#include <string.h>

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
}

FormatTools::~FormatTools()
{
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
						int lock_compressor,
						int recording,
						int *strategy,
						int brender)
{
	int x = init_x;
	int y = init_y;

	this->lock_compressor = lock_compressor;
	this->recording = recording;
	this->use_brender = brender;

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
	window->add_subwindow(path_textbox = new FormatPathText(x, y, this, asset));
	x += 305;
	window->add_subwindow(path_button = new BrowseButton(
		mwindow,
		window,
		path_textbox, 
		x, 
		y, 
		asset->path,
		"Output to file",
		"Select a file to write to:",
		0));

//printf("FormatTools::create_objects 2\n");
	x -= 305;
	y += 35;

//printf("FormatTools::create_objects 3\n");
	window->add_subwindow(new BC_Title(x, y, "File Format:"));
	x += 90;
	window->add_subwindow(format_text = new BC_TextBox(x, 
		y, 
		200, 
		1, 
		File::formattostr(plugindb, asset->format)));
	x += format_text->get_w();
	window->add_subwindow(format_button = new FormatFormat(x, 
		y, 
		this, 
		asset));
	format_button->create_objects();

//printf("FormatTools::create_objects 4\n");
	x = init_x;
	y += format_button->get_h() + 10;
	if(do_audio)
	{
		window->add_subwindow(new BC_Title(x, y, "Audio:", LARGEFONT, RED));
		x += 80;
		window->add_subwindow(aparams_button = new FormatAParams(mwindow, this, x, y));
		x += aparams_button->get_w() + 10;
		if(prompt_audio) 
		{
			window->add_subwindow(audio_switch = new FormatAudio(x, y, this, asset->audio_data));
		}
		x = init_x;
		y += aparams_button->get_h() + 20;

//printf("FormatTools::create_objects 5\n");
// Audio channels only used for recording.
		if(prompt_audio_channels)
		{
			window->add_subwindow(new BC_Title(x, y, "Number of audio channels to record:"));
			x += 260;
			window->add_subwindow(channels_button = new FormatChannels(x, y, asset));
			x += channels_button->get_w() + 5;
			window->add_subwindow(channels_tumbler = new BC_ITumbler(channels_button, 1, MAXCHANNELS, x, y));
			y += channels_button->get_h() + 20;
		}

//printf("FormatTools::create_objects 6\n");
		aparams_thread = new FormatAThread(this);
	}

//printf("FormatTools::create_objects 7\n");
	x = init_x;
	if(do_video)
	{

//printf("FormatTools::create_objects 8\n");
		window->add_subwindow(new BC_Title(x, y, "Video:", LARGEFONT, RED));
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
		vparams_thread = new FormatVThread(this, lock_compressor);
	}

//printf("FormatTools::create_objects 11\n");

	x = init_x;
	if(strategy)
	{
		BC_WindowBase *tool = window->add_subwindow(new FormatMultiple(mwindow, x, y, strategy));
		y += tool->get_h() + 10;
	}

//printf("FormatTools::create_objects 12\n");

	init_y = y;
	return 0;
}

int FormatTools::set_audio_options()
{
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
 : BC_Button(x, y, mwindow->theme->wrench_data)
{
	this->format = format;
	set_tooltip("Configure audio compression");
}
FormatAParams::~FormatAParams() 
{
}
int FormatAParams::handle_event() 
{
	format->set_audio_options(); 
}

FormatVParams::FormatVParams(MWindow *mwindow, FormatTools *format, int x, int y)
 : BC_Button(x, y, mwindow->theme->wrench_data)
{ 
	this->format = format; 
	set_tooltip("Configure video compression");
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
	file->get_options(format->window, 
		format->plugindb, 
		format->asset, 
		1, 
		0,
		0);
}




FormatVThread::FormatVThread(FormatTools *format, 
	int lock_compressor)
 : Thread()
{
	this->lock_compressor = lock_compressor;
	this->format = format;
	file = new File;
}
FormatVThread::~FormatVThread() 
{
	delete file;
}
void FormatVThread::run()
{
	file->get_options(format->window, 
		format->plugindb, 
		format->asset, 
		0, 
		1, 
		lock_compressor);
}

FormatPathText::FormatPathText(int x, int y, FormatTools *format, Asset *asset)
 : BC_TextBox(x, y, 300, 1, asset->path) 
{
	this->format = format; 
	this->asset = asset; 
}
FormatPathText::~FormatPathText() 
{}
int FormatPathText::handle_event() 
{
	strcpy(asset->path, get_text());
}




FormatAudio::FormatAudio(int x, int y, FormatTools *format, int default_)
 : BC_CheckBox(x, 
 	y, 
	default_, 
	(char*)(format->recording ? "Record audio tracks" : "Render audio tracks"))
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
	(char*)(format->recording ? "Record video tracks" : "Render video tracks"))
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
	FormatTools *format, 
	Asset *asset)
 : FormatPopup(format->plugindb, 
 	x, 
	y,
	format->use_brender)
{ 
	this->format = format; 
	this->asset = asset; 
}
FormatFormat::~FormatFormat() 
{
}
int FormatFormat::handle_event()
{
	if(get_selection(0, 0) >= 0)
	{
		asset->format = File::strtoformat(format->plugindb, get_selection(0, 0)->get_text());
		format->format_text->update(get_selection(0, 0)->get_text());
//printf("FormatFormat::handle_event %d %s\n", asset->format, get_selection(0, 0)->get_text());
	}
	return 1;
}



FormatChannels::FormatChannels(int x, int y, Asset *asset)
 : BC_TextBox(x, y, 100, 1, asset->channels) 
{ 
 	this->asset = asset; 
}
FormatChannels::~FormatChannels() {}
int FormatChannels::handle_event() 
{
	asset->channels = atol(get_text());
	return 1;
}

FormatToTracks::FormatToTracks(int x, int y, int *output)
 : BC_CheckBox(x, y, *output, "Overwrite project with output")
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
	"Create new file at each label")
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


