#include "cplayback.h"
#include "cwindow.h"
#include "defaults.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "levelwindow.h"
#include "mainundo.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"
#include "mainsession.h"
#include "theme.h"
#include "transportque.h"
#include "videowindow.h"
#include "vplayback.h"
#include "vwindow.h"

#include <string.h>

New::New(MWindow *mwindow)
 : BC_MenuItem("New...", "n", 'n')
{
	this->mwindow = mwindow;
	script = 0;
}

int New::create_objects()
{
	thread = new NewThread(mwindow, this);
	return 0;
}

int New::handle_event() 
{
	if(thread->running())
	{
		thread->nwindow->raise_window();
		return 1;
	}
	mwindow->edl->save_defaults(mwindow->defaults);
	create_new_edl();
	thread->start(); 

	return 1;
}

void New::create_new_edl()
{
	new_edl = new EDL;
	new_edl->create_objects();
	new_edl->load_defaults(mwindow->defaults);
}


int New::create_new_project()
{
//printf("New::create_new_project 1\n");
	mwindow->cwindow->playback_engine->que->send_command(STOP,
		CHANGE_NONE, 
		0,
		0);
	mwindow->vwindow->playback_engine->que->send_command(STOP,
		CHANGE_NONE, 
		0,
		0);
	mwindow->cwindow->playback_engine->interrupt_playback(0);
	mwindow->vwindow->playback_engine->interrupt_playback(0);

	mwindow->gui->lock_window();
//printf("New::create_new_project 1\n");
	mwindow->undo->update_undo_before("New", LOAD_ALL);

//printf("New::create_new_project 1\n");
	new_edl->session->boundaries();
	new_edl->create_default_tracks();

//printf("New::create_new_project 1\n");
	mwindow->set_filename("");
	mwindow->undo->update_undo_after();

//printf("New::create_new_project 1\n");
	mwindow->hide_plugins();
	delete mwindow->edl;
	mwindow->edl = new_edl;
	mwindow->save_defaults();

// Load file sequence
	mwindow->update_project(LOAD_REPLACE);
	mwindow->session->changes_made = 0;
	mwindow->gui->unlock_window();
//printf("New::create_new_project 2\n");
	return 0;
}

NewThread::NewThread(MWindow *mwindow, New *new_project)
 : Thread()
{
	this->mwindow = mwindow;
	this->new_project = new_project;
}

NewThread::~NewThread()
{
}


void NewThread::run()
{
	int result = 0;
	load_defaults();

	{
		nwindow = new NewWindow(mwindow, this);
		nwindow->create_objects();
		result = nwindow->run_window();
		delete nwindow;	
//printf("NewThread::run 1\n");
		new_project->new_edl->save_defaults(mwindow->defaults);
//printf("NewThread::run 2\n");
		mwindow->defaults->save();
//printf("NewThread::run 3\n");
	}

	if(result)
	{
// Aborted
		delete new_project->new_edl;
	}
	else
	{
//printf("NewThread::run 4\n");
		new_project->create_new_project();
//printf("NewThread::run 5\n");
	}
}

int NewThread::load_defaults()
{
	auto_aspect = mwindow->defaults->get("AUTOASPECT", 0);
	return 0;
}

int NewThread::save_defaults()
{
	mwindow->defaults->update("AUTOASPECT", auto_aspect);
	return 0;
}

int NewThread::update_aspect()
{
	if(auto_aspect)
	{
		char string[1024];
		mwindow->create_aspect_ratio(new_project->new_edl->session->aspect_w, 
			new_project->new_edl->session->aspect_h, 
			new_project->new_edl->session->output_w, 
			new_project->new_edl->session->output_h);
		sprintf(string, "%.02f", new_project->new_edl->session->aspect_w);
		nwindow->aspect_w_text->update(string);
		sprintf(string, "%.02f", new_project->new_edl->session->aspect_h);
		nwindow->aspect_h_text->update(string);
	}
	return 0;
}

#define WIDTH 600
#define HEIGHT 400

NewWindow::NewWindow(MWindow *mwindow, NewThread *new_thread)
 : BC_Window(PROGRAM_NAME ": New Project", 
 		mwindow->gui->get_root_w() / 2 - WIDTH / 2,
		mwindow->gui->get_root_h() / 2 - HEIGHT / 2,
		WIDTH, 
		HEIGHT,
		-1,
		-1,
		0,
		0,
		1)
{
	this->mwindow = mwindow;
	this->new_thread = new_thread;
	this->new_edl = new_thread->new_project->new_edl;
}

NewWindow::~NewWindow()
{
	for(int i = 0; i < preset_items.total; i++)
		delete preset_items.values[i];
}

void NewWindow::create_presets(int &x, int &y)
{
	NewPresetItem *item;
	add_subwindow(new BC_Title(x, y, "Presets:"));
	int x1 = x;
	y += 20;

	item = new NewPresetItem(mwindow, this, "User Defined");
	preset_items.append(item);

	item = new NewPresetItem(mwindow, this, "1080P");
	item->edl->session->audio_channels = 2;
	item->edl->session->audio_tracks = 2;
	item->edl->session->sample_rate = 48000;
	item->edl->session->video_channels = 1;
	item->edl->session->video_tracks = 1;
	item->edl->session->frame_rate = (double)30000.0 / 1001;
// 	item->edl->session->track_w = 1920;
// 	item->edl->session->track_h = 1080;
	item->edl->session->output_w = 1920;
	item->edl->session->output_h = 1080;
	item->edl->session->aspect_w = 16;
	item->edl->session->aspect_h = 9;
	preset_items.append(item);

	item = new NewPresetItem(mwindow, this, "720P");
	item->edl->session->audio_channels = 2;
	item->edl->session->audio_tracks = 2;
	item->edl->session->sample_rate = 48000;
	item->edl->session->video_channels = 1;
	item->edl->session->video_tracks = 1;
	item->edl->session->frame_rate = (double)30000.0 / 1001;
// 	item->edl->session->track_w = 1280;
// 	item->edl->session->track_h = 720;
	item->edl->session->output_w = 1280;
	item->edl->session->output_h = 720;
	item->edl->session->aspect_w = 16;
	item->edl->session->aspect_h = 9;
	preset_items.append(item);

	item = new NewPresetItem(mwindow, this, "Full D-1 NTSC");
	item->edl->session->audio_channels = 2;
	item->edl->session->audio_tracks = 2;
	item->edl->session->sample_rate = 48000;
	item->edl->session->video_channels = 1;
	item->edl->session->video_tracks = 1;
	item->edl->session->frame_rate = (double)30000.0 / 1001;
// 	item->edl->session->track_w = 720;
// 	item->edl->session->track_h = 480;
	item->edl->session->output_w = 720;
	item->edl->session->output_h = 480;
	item->edl->session->aspect_w = 4;
	item->edl->session->aspect_h = 3;
	preset_items.append(item);

	item = new NewPresetItem(mwindow, this, "Half D-1 NTSC");
	item->edl->session->audio_channels = 2;
	item->edl->session->audio_tracks = 2;
	item->edl->session->sample_rate = 48000;
	item->edl->session->video_channels = 1;
	item->edl->session->video_tracks = 1;
	item->edl->session->frame_rate = (double)30000.0 / 1001;
// 	item->edl->session->track_w = 360;
// 	item->edl->session->track_h = 240;
	item->edl->session->output_w = 360;
	item->edl->session->output_h = 240;
	item->edl->session->aspect_w = 4;
	item->edl->session->aspect_h = 3;
	preset_items.append(item);

	item = new NewPresetItem(mwindow, this, "Realvideo");
	item->edl->session->audio_channels = 1;
	item->edl->session->audio_tracks = 1;
	item->edl->session->sample_rate = 22050;
	item->edl->session->video_channels = 1;
	item->edl->session->video_tracks = 1;
	item->edl->session->frame_rate = 15;
// 	item->edl->session->track_w = 240;
// 	item->edl->session->track_h = 180;
	item->edl->session->output_w = 240;
	item->edl->session->output_h = 180;
	item->edl->session->aspect_w = 4;
	item->edl->session->aspect_h = 3;
	preset_items.append(item);

	item = new NewPresetItem(mwindow, this, "CD Audio");
	item->edl->session->audio_channels = 2;
	item->edl->session->audio_tracks = 2;
	item->edl->session->sample_rate = 44100;
	item->edl->session->video_channels = 1;
	item->edl->session->video_tracks = 0;
	item->edl->session->frame_rate = (double)30000.0 / 1001;
// 	item->edl->session->track_w = 720;
// 	item->edl->session->track_h = 480;
	item->edl->session->output_w = 720;
	item->edl->session->output_h = 480;
	item->edl->session->aspect_w = 4;
	item->edl->session->aspect_h = 3;
	preset_items.append(item);

	add_subwindow(presets_text = new NewPresetsText(mwindow, this, x, y));
	x += presets_text->get_w();
	add_subwindow(presets = new NewPresetsPulldown(mwindow, this, x, y));
	x = x1;
}

int NewWindow::create_objects()
{
	int x = 10, y = 10, x1, y1;
	BC_TextBox *textbox;

	mwindow->theme->draw_new_bg(this);

	add_subwindow(new BC_Title(x, y, "Parameters for the new project:"));
	y += 20;
	create_presets(x, y);
	y += 40;
	y1 = y;
	add_subwindow(new BC_Title(x, y, "Audio", LARGEFONT));
	y += 30;

	x1 = x;
	add_subwindow(new BC_Title(x1, y, "Tracks:"));
	x1 += 100;
	add_subwindow(atracks = new NewATracks(this, "", x1, y));
	x1 += atracks->get_w();
	add_subwindow(new NewATracksTumbler(this, x1, y));
	y += atracks->get_h() + 5;

	x1 = x;
	add_subwindow(new BC_Title(x1, y, "Channels:"));
	x1 += 100;
	add_subwindow(achannels = new NewAChannels(this, "", x1, y));
	x1 += achannels->get_w();
	add_subwindow(new NewAChannelsTumbler(this, x1, y));
	y += achannels->get_h() + 5;

	x1 = x;
	add_subwindow(new BC_Title(x1, y, "Samplerate:"));
	x1 += 100;
	add_subwindow(sample_rate = new NewSampleRate(this, "", x1, y));
	x1 += sample_rate->get_w();
	add_subwindow(new SampleRatePulldown(mwindow, sample_rate, x1, y));
	
	x += 250;
	y = y1;
	add_subwindow(new BC_Title(x, y, "Video", LARGEFONT));
	y += 30;
	x1 = x;
	add_subwindow(new BC_Title(x1, y, "Tracks:"));
	x1 += 100;
	add_subwindow(vtracks = new NewVTracks(this, "", x1, y));
	x1 += vtracks->get_w();
	add_subwindow(new NewVTracksTumbler(this, x1, y));
	y += vtracks->get_h() + 5;

// 	x1 = x;
// 	add_subwindow(new BC_Title(x1, y, "Channels:"));
// 	x1 += 100;
// 	add_subwindow(vchannels = new NewVChannels(this, "", x1, y));
// 	x1 += vchannels->get_w();
// 	add_subwindow(new NewVChannelsTumbler(this, x1, y));
// 	y += vchannels->get_h() + 5;
	x1 = x;
	add_subwindow(new BC_Title(x1, y, "Framerate:"));
	x1 += 100;
	add_subwindow(frame_rate = new NewFrameRate(this, "", x1, y));
	x1 += frame_rate->get_w();
	add_subwindow(new FrameRatePulldown(mwindow, frame_rate, x1, y));
	y += frame_rate->get_h() + 5;

//	x1 = x;
//	add_subwindow(new BC_Title(x1, y, "Canvas size:"));
// 	x1 += 100;
// 	add_subwindow(canvas_w_text = new NewTrackW(this, x1, y));
// 	x1 += canvas_w_text->get_w() + 2;
// 	add_subwindow(new BC_Title(x1, y, "x"));
// 	x1 += 10;
// 	add_subwindow(canvas_h_text = new NewTrackH(this, x1, y));
// 	x1 += canvas_h_text->get_w();
// 	add_subwindow(new FrameSizePulldown(mwindow, 
// 		canvas_w_text, 
// 		canvas_h_text, 
// 		x1, 
// 		y));
//	x1 += 100;
//	add_subwindow(new NewCloneToggle(mwindow, this, x1, y));
//	y += canvas_h_text->get_h() + 5;

	x1 = x;
	add_subwindow(new BC_Title(x1, y, "Canvas size:"));
	x1 += 100;
	add_subwindow(output_w_text = new NewOutputW(this, x1, y));
	x1 += output_w_text->get_w() + 2;
	add_subwindow(new BC_Title(x1, y, "x"));
	x1 += 10;
	add_subwindow(output_h_text = new NewOutputH(this, x1, y));
	x1 += output_h_text->get_w();
	add_subwindow(new FrameSizePulldown(mwindow, 
		output_w_text, 
		output_h_text, 
		x1, 
		y));
	y += output_h_text->get_h() + 5;

	x1 = x;
	add_subwindow(new BC_Title(x1, y, "Aspect ratio:"));
	x1 += 100;
	add_subwindow(aspect_w_text = new NewAspectW(this, "", x1, y));
	x1 += aspect_w_text->get_w() + 2;
	add_subwindow(new BC_Title(x1, y, ":"));
	x1 += 10;
	add_subwindow(aspect_h_text = new NewAspectH(this, "", x1, y));
	x1 += aspect_h_text->get_w();
	add_subwindow(new AspectPulldown(mwindow, 
		aspect_w_text, 
		aspect_h_text, 
		x1, 
		y));

	x1 = aspect_w_text->get_x();
	y += aspect_w_text->get_h() + 5;
	add_subwindow(new NewAspectAuto(this, x1, y));
	y += 40;
	add_subwindow(new BC_Title(x, y, "Color model:"));
	x += 100;
	add_subwindow(textbox = new BC_TextBox(x, y, 200, 1, ""));
	x += textbox->get_w();
	add_subwindow(new ColormodelPulldown(mwindow, 
		textbox, 
		&new_edl->session->color_model,
		x, 
		y));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	flash();
	update();
	show_window();
	return 0;
}

int NewWindow::update()
{
	char string[BCTEXTLEN];
	atracks->update((long)new_edl->session->audio_tracks);
	achannels->update((long)new_edl->session->audio_channels);
	sample_rate->update((long)new_edl->session->sample_rate);
	vtracks->update((long)new_edl->session->video_tracks);
//	vchannels->update((long)new_edl->session->video_channels);
	frame_rate->update((float)new_edl->session->frame_rate);
// 	canvas_w_text->update((long)new_edl->session->track_w);
// 	canvas_h_text->update((long)new_edl->session->track_h);
	output_w_text->update((long)new_edl->session->output_w);
	output_h_text->update((long)new_edl->session->output_h);
	aspect_w_text->update((float)new_edl->session->aspect_w);
	aspect_h_text->update((float)new_edl->session->aspect_h);
	return 0;
}

NewPresetsText::NewPresetsText(MWindow *mwindow, NewWindow *window, int x, int y)
 : BC_TextBox(x, y, 200, 1, window->preset_items.values[0]->get_text())
{
	this->mwindow = mwindow;
	this->window = window;
}

int NewPresetsText::handle_event()
{
	return 1;
}





NewPresetsPulldown::NewPresetsPulldown(MWindow *mwindow, NewWindow *window, int x, int y)
 : BC_ListBox(x, 
		y, 
		200, 
		200,
		LISTBOX_TEXT,                   // Display text list or icons
		(ArrayList<BC_ListBoxItem*>*)&window->preset_items, // Each column has an ArrayList of BC_ListBoxItems.
		0,             // Titles for columns.  Set to 0 for no titles
		0,                // width of each column
		1,                      // Total columns.
		0,                    // Pixel of top of window.
		1)
{
	this->mwindow = mwindow;
	this->window = window;
}
int NewPresetsPulldown::handle_event()
{
	NewPresetItem *preset = ((NewPresetItem*)get_selection(0, 0));
	window->new_edl->copy_all(preset->edl);
	window->update();
	window->presets_text->update(preset->get_text());
	return 1;
}

NewPresetItem::NewPresetItem(MWindow *mwindow, NewWindow *window, char *text)
 : BC_ListBoxItem(text)
{
	this->mwindow = mwindow;
	this->window = window;
	edl = new EDL;
	edl->create_objects();
	edl->copy_all(window->new_edl);
}

NewPresetItem::~NewPresetItem()
{
	delete edl;
}





NewATracks::NewATracks(NewWindow *nwindow, char *text, int x, int y)
 : BC_TextBox(x, y, 90, 1, text)
{
	this->nwindow = nwindow;
}

int NewATracks::handle_event()
{
	nwindow->new_edl->session->audio_tracks = atol(get_text());
	return 1;
}

NewATracksTumbler::NewATracksTumbler(NewWindow *nwindow, int x, int y)
 : BC_Tumbler(x, y)
{
	this->nwindow = nwindow;
}
int NewATracksTumbler::handle_up_event()
{
	nwindow->new_edl->session->audio_tracks++;
	nwindow->new_edl->boundaries();
	nwindow->update();
	return 1;
}
int NewATracksTumbler::handle_down_event()
{
	nwindow->new_edl->session->audio_tracks--;
	nwindow->new_edl->boundaries();
	nwindow->update();
	return 1;
}

NewAChannels::NewAChannels(NewWindow *nwindow, char *text, int x, int y)
 : BC_TextBox(x, y, 90, 1, text)
{
	this->nwindow = nwindow;
} 

int NewAChannels::handle_event()
{
	nwindow->new_edl->session->audio_channels = atol(get_text());
	return 1;
}

NewAChannelsTumbler::NewAChannelsTumbler(NewWindow *nwindow, int x, int y)
 : BC_Tumbler(x, y)
{
	this->nwindow = nwindow;
}
int NewAChannelsTumbler::handle_up_event()
{
	nwindow->new_edl->session->audio_channels++;
	nwindow->new_edl->boundaries();
	nwindow->update();
	return 1;
}
int NewAChannelsTumbler::handle_down_event()
{
	nwindow->new_edl->session->audio_channels--;
	nwindow->new_edl->boundaries();
	nwindow->update();
	return 1;
}


NewSampleRate::NewSampleRate(NewWindow *nwindow, char *text, int x, int y)
 : BC_TextBox(x, y, 90, 1, text)
{
	this->nwindow = nwindow;
}

int NewSampleRate::handle_event()
{
	nwindow->new_edl->session->sample_rate = atol(get_text());
	return 1;
}

SampleRatePulldown::SampleRatePulldown(MWindow *mwindow, BC_TextBox *output, int x, int y)
 : BC_ListBox(x,
 	y,
	100,
	200,
	LISTBOX_TEXT,
	&mwindow->theme->sample_rates,
	0,
	0,
	1,
	0,
	1)
{
	this->mwindow = mwindow;
	this->output = output;
}
int SampleRatePulldown::handle_event()
{
	char *text = get_selection(0, 0)->get_text();
	output->update(text);
	output->handle_event();
	return 1;
}















NewVTracks::NewVTracks(NewWindow *nwindow, char *text, int x, int y)
 : BC_TextBox(x, y, 90, 1, text)
{
	this->nwindow = nwindow;
}

int NewVTracks::handle_event()
{
	nwindow->new_edl->session->video_tracks = atol(get_text());
	return 1;
}

NewVTracksTumbler::NewVTracksTumbler(NewWindow *nwindow, int x, int y)
 : BC_Tumbler(x, y)
{
	this->nwindow = nwindow;
}
int NewVTracksTumbler::handle_up_event()
{
printf("NewVTracks::handle_event 1 %f\n", nwindow->new_edl->session->frame_rate);
	nwindow->new_edl->session->video_tracks++;
printf("NewVTracks::handle_event 2 %d %f\n", nwindow->new_edl->session->video_tracks, nwindow->new_edl->session->frame_rate);
	nwindow->new_edl->boundaries();
printf("NewVTracks::handle_event 3 %d %f\n", nwindow->new_edl->session->video_tracks, nwindow->new_edl->session->frame_rate);
	nwindow->update();
printf("NewVTracks::handle_event 4 %d %f\n", nwindow->new_edl->session->video_tracks, nwindow->new_edl->session->frame_rate);
	return 1;
}
int NewVTracksTumbler::handle_down_event()
{
	nwindow->new_edl->session->video_tracks--;
	nwindow->new_edl->boundaries();
	nwindow->update();
	return 1;
}

NewVChannels::NewVChannels(NewWindow *nwindow, char *text, int x, int y)
 : BC_TextBox(x, y, 90, 1, text)
{
	this->nwindow = nwindow;
}

int NewVChannels::handle_event()
{
	nwindow->new_edl->session->video_channels = atol(get_text());
	return 1;
}

NewVChannelsTumbler::NewVChannelsTumbler(NewWindow *nwindow, int x, int y)
 : BC_Tumbler(x, y)
{
	this->nwindow = nwindow;
}
int NewVChannelsTumbler::handle_up_event()
{
	nwindow->new_edl->session->video_channels++;
	nwindow->new_edl->boundaries();
	nwindow->update();
	return 1;
}
int NewVChannelsTumbler::handle_down_event()
{
	nwindow->new_edl->session->video_channels--;
	nwindow->new_edl->boundaries();
	nwindow->update();
	return 1;
}

NewFrameRate::NewFrameRate(NewWindow *nwindow, char *text, int x, int y)
 : BC_TextBox(x, y, 90, 1, text)
{
	this->nwindow = nwindow;
}

int NewFrameRate::handle_event()
{
	nwindow->new_edl->session->frame_rate = Units::atoframerate(get_text());
	return 1;
}

FrameRatePulldown::FrameRatePulldown(MWindow *mwindow, 
	BC_TextBox *output, 
	int x, 
	int y)
 : BC_ListBox(x,
 	y,
	100,
	200,
	LISTBOX_TEXT,
	&mwindow->theme->frame_rates,
	0,
	0,
	1,
	0,
	1)
{
	this->mwindow = mwindow;
	this->output = output;
}
int FrameRatePulldown::handle_event()
{
	char *text = get_selection(0, 0)->get_text();
	output->update(text);
	output->handle_event();
	return 1;
}

// NewTrackW::NewTrackW(NewWindow *nwindow, int x, int y)
//  : BC_TextBox(x, y, 70, 1, nwindow->new_edl->session->track_w)
// {
// 	this->nwindow = nwindow;
// }
// int NewTrackW::handle_event()
// {
// 	nwindow->new_edl->session->track_w = atol(get_text());
// 	return 1;
// }
// 
// NewTrackH::NewTrackH(NewWindow *nwindow, int x, int y)
//  : BC_TextBox(x, y, 70, 1, nwindow->new_edl->session->track_h)
// {
// 	this->nwindow = nwindow;
// }
// 
// int NewTrackH::handle_event()
// {
// 	nwindow->new_edl->session->track_h = atol(get_text());
// 	return 1;
// }

FrameSizePulldown::FrameSizePulldown(MWindow *mwindow, 
		BC_TextBox *output_w, 
		BC_TextBox *output_h, 
		int x, 
		int y)
 : BC_ListBox(x,
 	y,
	100,
	200,
	LISTBOX_TEXT,
	&mwindow->theme->frame_sizes,
	0,
	0,
	1,
	0,
	1)
{
	this->mwindow = mwindow;
	this->output_w = output_w;
	this->output_h = output_h;
}
int FrameSizePulldown::handle_event()
{
	char *text = get_selection(0, 0)->get_text();
	char string[BCTEXTLEN];
	long w, h;
	char *ptr;
	
	strcpy(string, text);
	ptr = strrchr(string, 'x');
	if(ptr)
	{
		ptr++;
		h = atol(ptr);
		
		*--ptr = 0;
		w = atol(string);
		output_w->update(w);
		output_h->update(h);
		output_w->handle_event();
		output_h->handle_event();
	}
	return 1;
}

NewOutputW::NewOutputW(NewWindow *nwindow, int x, int y)
 : BC_TextBox(x, y, 70, 1, nwindow->new_edl->session->output_w)
{
	this->nwindow = nwindow;
}
int NewOutputW::handle_event()
{
	nwindow->new_edl->session->output_w = atol(get_text());
	nwindow->new_thread->update_aspect();
	return 1;
}

NewOutputH::NewOutputH(NewWindow *nwindow, int x, int y)
 : BC_TextBox(x, y, 70, 1, nwindow->new_edl->session->output_h)
{
	this->nwindow = nwindow;
}
int NewOutputH::handle_event()
{
	nwindow->new_edl->session->output_h = atol(get_text());
	nwindow->new_thread->update_aspect();
	return 1;
}

NewAspectW::NewAspectW(NewWindow *nwindow, char *text, int x, int y)
 : BC_TextBox(x, y, 70, 1, text)
{
	this->nwindow = nwindow;
}

int NewAspectW::handle_event()
{
	nwindow->new_edl->session->aspect_w = atof(get_text());
	return 1;
}

NewAspectH::NewAspectH(NewWindow *nwindow, char *text, int x, int y)
 : BC_TextBox(x, y, 70, 1, text)
{
	this->nwindow = nwindow;
}

int NewAspectH::handle_event()
{
	nwindow->new_edl->session->aspect_h = atof(get_text());
	return 1;
}

AspectPulldown::AspectPulldown(MWindow *mwindow, 
		BC_TextBox *output_w, 
		BC_TextBox *output_h, 
		int x, 
		int y)
 : BC_ListBox(x,
 	y,
	100,
	200,
	LISTBOX_TEXT,
	&mwindow->theme->aspect_ratios,
	0,
	0,
	1,
	0,
	1)
{
	this->mwindow = mwindow;
	this->output_w = output_w;
	this->output_h = output_h;
}
int AspectPulldown::handle_event()
{
	char *text = get_selection(0, 0)->get_text();
	char string[BCTEXTLEN];
	float w, h;
	char *ptr;
	
	strcpy(string, text);
	ptr = strrchr(string, ':');
	if(ptr)
	{
		ptr++;
		h = atof(ptr);
		
		*--ptr = 0;
		w = atof(string);
		output_w->update(w);
		output_h->update(h);
		output_w->handle_event();
		output_h->handle_event();
	}
	return 1;
}

ColormodelItem::ColormodelItem(char *text, int value)
 : BC_ListBoxItem(text)
{
	this->value = value;
}

ColormodelPulldown::ColormodelPulldown(MWindow *mwindow, 
		BC_TextBox *output_text, 
		int *output_value,
		int x, 
		int y)
 : BC_ListBox(x,
 	y,
	200,
	150,
	LISTBOX_TEXT,
	(ArrayList<BC_ListBoxItem*>*)&mwindow->colormodels,
	0,
	0,
	1,
	0,
	1)
{
	this->mwindow = mwindow;
	this->output_text = output_text;
	this->output_value = output_value;
	output_text->update(colormodel_to_text());
}

int ColormodelPulldown::handle_event()
{
	output_text->update(get_selection(0, 0)->get_text());
	*output_value = ((ColormodelItem*)get_selection(0, 0))->value;
	return 1;
}

char* ColormodelPulldown::colormodel_to_text()
{
	for(int i = 0; i < mwindow->colormodels.total; i++)
		if(mwindow->colormodels.values[i]->value == *output_value) 
			return mwindow->colormodels.values[i]->get_text();
	return "Unknown";
}










NewAspectAuto::NewAspectAuto(NewWindow *nwindow, int x, int y)
 : BC_CheckBox(x, y, nwindow->new_thread->auto_aspect, "Auto aspect ratio")
{
	this->nwindow = nwindow;
}
NewAspectAuto::~NewAspectAuto()
{
}
int NewAspectAuto::handle_event()
{
	nwindow->new_thread->auto_aspect = get_value();
	nwindow->new_thread->update_aspect();
	return 1;
}


#if 0

NewCloneToggle::NewCloneToggle(MWindow *mwindow, NewWindow *nwindow, int x, int y)
 : BC_Toggle(x, 
 	y, 
	mwindow->theme->chain_data, 
	nwindow->new_thread->auto_sizes,
	"",
	0, 
	0, 
	0)
{
	this->mwindow = mwindow;
	this->nwindow = nwindow;
}

int NewCloneToggle::handle_event()
{
	nwindow->canvas_w_text->update((long)nwindow->new_edl->session->track_w);
	nwindow->canvas_h_text->update((long)nwindow->new_edl->session->track_h);
	nwindow->new_edl->session->output_w = nwindow->new_edl->session->track_w;
	nwindow->new_edl->session->output_h = nwindow->new_edl->session->track_h;
	nwindow->new_thread->update_aspect();
	return 1;
}

#endif
