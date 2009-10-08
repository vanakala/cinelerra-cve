
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

#include "clip.h"
#include "cplayback.h"
#include "cwindow.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "interlacemodes.h"
#include "language.h"
#include "levelwindow.h"
#include "mainundo.h"
#include "mainmenu.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"
#include "newpresets.h"
#include "mainsession.h"
#include "patchbay.h"
#include "preferences.h"
#include "theme.h"
#include "transportque.h"
#include "videowindow.h"
#include "vplayback.h"
#include "vwindow.h"


#include <string.h>


#define WIDTH 600
#define HEIGHT 400


New::New(MWindow *mwindow)
 : BC_MenuItem(_("New..."), "n", 'n')
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
		thread->window_lock->lock("New::handle_event");
		if(thread->nwindow)
		{
			thread->nwindow->lock_window("New::handle_event");
			thread->nwindow->raise_window();
			thread->nwindow->unlock_window();
		}
		thread->window_lock->unlock();
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
	mwindow->reset_caches();

	memcpy(new_edl->session->achannel_positions,
		&mwindow->preferences->channel_positions[
			MAXCHANNELS * (new_edl->session->audio_channels - 1)],
		sizeof(int) * MAXCHANNELS);
	new_edl->session->boundaries();
	new_edl->create_default_tracks();

	mwindow->set_filename("");
	mwindow->undo->update_undo(_("New"), LOAD_ALL);

	mwindow->hide_plugins();
	delete mwindow->edl;
	mwindow->edl = new_edl;
	mwindow->save_defaults();

// Load file sequence
	mwindow->update_project(LOAD_REPLACE);
	mwindow->session->changes_made = 0;
	mwindow->gui->unlock_window();
	return 0;
}

NewThread::NewThread(MWindow *mwindow, New *new_project)
 : Thread()
{
	this->mwindow = mwindow;
	this->new_project = new_project;
	window_lock = new Mutex("NewThread::window_lock");
}

NewThread::~NewThread()
{
	delete window_lock;
}


void NewThread::run()
{
	int result = 0;
	load_defaults();

	int x = mwindow->gui->get_root_w(0, 1) / 2 - WIDTH / 2;
	int y = mwindow->gui->get_root_h(1) / 2 - HEIGHT / 2;

	window_lock->lock("NewThread::run 1\n");
	nwindow = new NewWindow(mwindow, this, x, y);
	nwindow->create_objects();
	window_lock->unlock();

	result = nwindow->run_window();

	window_lock->lock("NewThread::run 2\n");
	delete nwindow;	
	nwindow = 0;
	window_lock->unlock();

	new_project->new_edl->save_defaults(mwindow->defaults);
	mwindow->defaults->save();

	if(result)
	{
// Aborted
		delete new_project->new_edl;
	}
	else
	{
		new_project->create_new_project();
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
		char string[BCTEXTLEN];
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




#if 0
N_("Cinelerra: New Project");
#endif

NewWindow::NewWindow(MWindow *mwindow, NewThread *new_thread, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": New Project"), 
 		x,
		y,
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
	format_presets = 0;
}

NewWindow::~NewWindow()
{
	if(format_presets) delete format_presets;
}

int NewWindow::create_objects()
{
	int x = 10, y = 10, x1, y1;
	BC_TextBox *textbox;

	mwindow->theme->draw_new_bg(this);

	add_subwindow(new BC_Title(x, y, _("Parameters for the new project:")));
	y += 20;

	format_presets = new NewPresets(mwindow,
		this, 
		x, 
		y);
	format_presets->create_objects();
	x = format_presets->x;
	y = format_presets->y;



	y += 40;
	y1 = y;
	add_subwindow(new BC_Title(x, y, _("Audio"), LARGEFONT));
	y += 30;

	x1 = x;
	add_subwindow(new BC_Title(x1, y, _("Tracks:")));
	x1 += 100;
	add_subwindow(atracks = new NewATracks(this, "", x1, y));
	x1 += atracks->get_w();
	add_subwindow(new NewATracksTumbler(this, x1, y));
	y += atracks->get_h() + 5;

	x1 = x;
	add_subwindow(new BC_Title(x1, y, _("Channels:")));
	x1 += 100;
	add_subwindow(achannels = new NewAChannels(this, "", x1, y));
	x1 += achannels->get_w();
	add_subwindow(new NewAChannelsTumbler(this, x1, y));
	y += achannels->get_h() + 5;

	x1 = x;
	add_subwindow(new BC_Title(x1, y, _("Samplerate:")));
	x1 += 100;
	add_subwindow(sample_rate = new NewSampleRate(this, "", x1, y));
	x1 += sample_rate->get_w();
	add_subwindow(new SampleRatePulldown(mwindow, sample_rate, x1, y));
	
	x += 250;
	y = y1;
	add_subwindow(new BC_Title(x, y, _("Video"), LARGEFONT));
	y += 30;
	x1 = x;
	add_subwindow(new BC_Title(x1, y, _("Tracks:")));
	x1 += 100;
	add_subwindow(vtracks = new NewVTracks(this, "", x1, y));
	x1 += vtracks->get_w();
	add_subwindow(new NewVTracksTumbler(this, x1, y));
	y += vtracks->get_h() + 5;

// 	x1 = x;
// 	add_subwindow(new BC_Title(x1, y, _("Channels:")));
// 	x1 += 100;
// 	add_subwindow(vchannels = new NewVChannels(this, "", x1, y));
// 	x1 += vchannels->get_w();
// 	add_subwindow(new NewVChannelsTumbler(this, x1, y));
// 	y += vchannels->get_h() + 5;
	x1 = x;
	add_subwindow(new BC_Title(x1, y, _("Framerate:")));
	x1 += 100;
	add_subwindow(frame_rate = new NewFrameRate(this, "", x1, y));
	x1 += frame_rate->get_w();
	add_subwindow(new FrameRatePulldown(mwindow, frame_rate, x1, y));
	y += frame_rate->get_h() + 5;

//	x1 = x;
//	add_subwindow(new BC_Title(x1, y, _("Canvas size:")));
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
	add_subwindow(new BC_Title(x1, y, _("Canvas size:")));
	x1 += 100;
	add_subwindow(output_w_text = new NewOutputW(this, x1, y));
	x1 += output_w_text->get_w() + 2;
	add_subwindow(new BC_Title(x1, y, "x"));
	x1 += 10;
	add_subwindow(output_h_text = new NewOutputH(this, x1, y));
	x1 += output_h_text->get_w();
	FrameSizePulldown *pulldown;
	add_subwindow(pulldown = new FrameSizePulldown(mwindow, 
		output_w_text, 
		output_h_text, 
		x1, 
		y));
	x1 += pulldown->get_w() + 5;
	add_subwindow(new NewSwapExtents(mwindow, this, x1, y));
	y += output_h_text->get_h() + 5;

	x1 = x;
	add_subwindow(new BC_Title(x1, y, _("Aspect ratio:")));
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
	add_subwindow(new BC_Title(x, y, _("Color model:")));
	add_subwindow(textbox = new BC_TextBox(x + 100, y, 200, 1, ""));
	add_subwindow(new ColormodelPulldown(mwindow, 
		textbox, 
		&new_edl->session->color_model,
		x + 100 + textbox->get_w(),
		y));
	y += textbox->get_h() + 5;

	// --------------------
	add_subwindow(new BC_Title(x, y, _("Interlace mode:")));
	add_subwindow(textbox = new BC_TextBox(x + 100, y, 140, 1, ""));
	add_subwindow(interlace_pulldown = new InterlacemodePulldown(mwindow, 
		textbox, 
		&new_edl->session->interlace_mode,
		(ArrayList<BC_ListBoxItem*>*)&mwindow->interlace_project_modes,
		x + 100 + textbox->get_w(), 
		y)); 
	y += textbox->get_h() + 5;

	add_subwindow(new BC_OKButton(this, 
		mwindow->theme->get_image_set("new_ok_images")));
	add_subwindow(new BC_CancelButton(this, 
		mwindow->theme->get_image_set("new_cancel_images")));
	flash();
	update();
	show_window();
	return 0;
}

int NewWindow::update()
{
	char string[BCTEXTLEN];
	atracks->update((int64_t)new_edl->session->audio_tracks);
	achannels->update((int64_t)new_edl->session->audio_channels);
	sample_rate->update((int64_t)new_edl->session->sample_rate);
	vtracks->update((int64_t)new_edl->session->video_tracks);
	frame_rate->update((float)new_edl->session->frame_rate);
	output_w_text->update((int64_t)new_edl->session->output_w);
	output_h_text->update((int64_t)new_edl->session->output_h);
	aspect_w_text->update((float)new_edl->session->aspect_w);
	aspect_h_text->update((float)new_edl->session->aspect_h);
	interlace_pulldown->update(new_edl->session->interlace_mode);
	return 0;
}







NewPresets::NewPresets(MWindow *mwindow, NewWindow *gui, int x, int y)
 : FormatPresets(mwindow, gui, 0, x, y)
{
}

NewPresets::~NewPresets()
{
}

int NewPresets::handle_event()
{
	new_gui->update();
	return 1;
}

EDL* NewPresets::get_edl()
{
	return new_gui->new_edl;
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
	nwindow->new_edl->session->video_tracks++;
	nwindow->new_edl->boundaries();
	nwindow->update();
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
	int64_t w, h;
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
	nwindow->new_edl->session->output_w = MAX(1,atol(get_text()));
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
	nwindow->new_edl->session->output_h = MAX(1, atol(get_text()));
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

InterlacemodeItem::InterlacemodeItem(char *text, int value)
 : BC_ListBoxItem(text)
{
	this->value = value;
}

InterlacemodePulldown::InterlacemodePulldown(MWindow *mwindow, 
		BC_TextBox *output_text,
		int *output_value,
		ArrayList<BC_ListBoxItem*> *data,
		int x, 
		int y)
 : BC_ListBox(x,
 	y,
	200,
	150,
	LISTBOX_TEXT,
	data,
	0,
	0,
	1,
	0,
	1)
{
	char string[BCTEXTLEN];
	this->mwindow = mwindow;
	this->output_text = output_text;
	this->output_value = output_value;
	output_text->update(interlacemode_to_text());
}

int InterlacemodePulldown::handle_event()
{
	output_text->update(get_selection(0, 0)->get_text());
	*output_value = ((InterlacemodeItem*)get_selection(0, 0))->value;
	return 1;
}

char* InterlacemodePulldown::interlacemode_to_text()
{
	ilacemode_to_text(this->string,*output_value);
	return (this->string);
}

int InterlacemodePulldown::update(int interlace_mode)
{
	*output_value = interlace_mode;
	output_text->update(interlacemode_to_text());
	return 1;
}


NewAspectAuto::NewAspectAuto(NewWindow *nwindow, int x, int y)
 : BC_CheckBox(x, y, nwindow->new_thread->auto_aspect, _("Auto aspect ratio"))
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








NewSwapExtents::NewSwapExtents(MWindow *mwindow, NewWindow *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("swap_extents"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip("Swap dimensions");
}

int NewSwapExtents::handle_event()
{
	int w = gui->new_edl->session->output_w;
	int h = gui->new_edl->session->output_h;
	gui->new_edl->session->output_w = h;
	gui->new_edl->session->output_h = w;
	gui->output_w_text->update((int64_t)h);
	gui->output_h_text->update((int64_t)w);
	gui->new_thread->update_aspect();
	return 1;
}




