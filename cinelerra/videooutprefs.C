#include "vdeviceprefs.h"
#include "videodevice.inc"
#include "defaults.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "videooutprefs.h"
#include "preferences.h"

#include <string.h>

VideoOutPrefs::VideoOutPrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
	head_text = 0;
	head_count_text = 0;
	host_text = 0;
	video_device = 0;
}

VideoOutPrefs::~VideoOutPrefs()
{
	delete_strategy();
	mwindow->defaults->update("PLAYBACK_HEAD", current_head);
	delete video_device;
}

int VideoOutPrefs::create_objects()
{
	int x = 5, y = 5;
	char string[1024];
	BC_PopupTextBox *popup;

//printf("VideoOutPrefs::create_objects 1\n");
	current_head = mwindow->defaults->get("PLAYBACK_HEAD", 0);
	strategies.append(new BC_ListBoxItem("Local Host"));
	strategies.append(new BC_ListBoxItem("Multihead"));
	strategies.append(new BC_ListBoxItem("Blond Symphony"));

//	add_subwindow(new BC_Title(x, y, "Playback", LARGEFONT, BLACK));
//	x += 200;
// 	add_subwindow(new BC_Title(x, y, "Strategy:"));
// 	popup = new PlaybackStrategy(this, x + 70, y);
// 	popup->create_objects();
// 	x = 10;
//	y += popup->get_h() + 30;

// Global playback options

// All strategies use these
	add_subwindow(new BC_Title(x, y, "Video", LARGEFONT, BLACK));
	y += 30;

	add_subwindow(new BC_Title(x, y, "Framerate achieved:"));
	add_subwindow(framerate_title = new BC_Title(x + 200, y, "--", RED));
	y += 30;

	add_subwindow(new VideoEveryFrame(pwindow, x, y));
	y += 35;
 	add_subwindow(new BC_Title(x, y, "Scaling equation:"));
	y += 20;
	add_subwindow(nearest_neighbor = new PlaybackNearest(pwindow, 
		this, 
		pwindow->thread->edl->session->interpolation_type == NEAREST_NEIGHBOR, 
		x, 
		y));
	y += 20;
	add_subwindow(bicubic = new PlaybackBicubic(pwindow, 
		this, 
		pwindow->thread->edl->session->interpolation_type == CUBIC_INTERPOLATE, 
		x, 
		y));

	y += 35;
	add_subwindow(new BC_Title(x, y, "Preload buffer for Quicktime:", MEDIUMFONT, BLACK));
	sprintf(string, "%d", pwindow->thread->edl->session->playback_preload);
	add_subwindow(new PlaybackPreload(x + 210, y, pwindow, this, string));
	y += 35;

// Strategic playback options created here
	set_strategy(pwindow->thread->edl->session->playback_strategy, y);
// 
// 	add_subwindow(new BC_Title(x, y, "Playback driver:", MEDIUMFONT, BLACK));
// 	add_subwindow(new AudioDriverMenu(x, y + 20, out_device, &(pwindow->thread->preferences->aconfig->audio_out_driver), 0, 1));
// 	y += 70;
// 
	return 0;
}

void VideoOutPrefs::update(int interpolation)
{
	pwindow->thread->edl->session->interpolation_type = interpolation;
	nearest_neighbor->update(interpolation == NEAREST_NEIGHBOR);
	bicubic->update(interpolation == CUBIC_INTERPOLATE);
}

char* VideoOutPrefs::strategy_to_string(int strategy)
{
	switch(strategy)
	{
		case PLAYBACK_LOCALHOST:
			return "Local Host";
			break;
		case PLAYBACK_MULTIHEAD:
			return "Multihead";
			break;
		case PLAYBACK_BLONDSYMPHONY:
			return "Blond Symphony";
			break;
	}
	return "Local Host";
}

// Delete strategy dependant objects
void VideoOutPrefs::delete_strategy()
{
	if(head_text) 
	{
//		delete head_text;
		delete head_title;
		head_text = 0;
	}
	if(host_text)
	{
		delete host_title;
//		delete host_text;
		host_text = 0;
	}
	if(head_count_text)
	{
		delete head_count_title;
//		delete head_count_text;
		head_count_text = 0;
	}
	if(video_device)
	{
		delete vdevice_title;
		delete video_device;
		video_device = 0;
	}
}

int VideoOutPrefs::set_strategy(int strategy, int y)
{
	int x = 5, x1 = 450;
	delete_strategy();

	pwindow->thread->edl->session->playback_strategy = strategy;
	switch(strategy)
	{
		case PLAYBACK_LOCALHOST:
			add_subwindow(vdevice_title = new BC_Title(x, y, "Video Driver:"));
			video_device = new VDevicePrefs(x + 100, 
				y, 
				pwindow, 
				this, 
				current_config()->vconfig, 
				0,
				MODEPLAY);
			video_device->initialize();
			break;
		case PLAYBACK_MULTIHEAD:
			add_subwindow(head_title = new BC_Title(x, y, "Head:"));
//			head_text = new PlaybackHead(this, x1, y);
//			head_text->create_objects();
			y += 25;
//			add_subwindow(head_count_title = new BC_Title(x, y, "Total Heads:"));
//			head_count_text = new PlaybackHeadCount(this, x1, y);
//			head_count_text->create_objects();
			x = 10;
			y = 390;
			break;
		case PLAYBACK_BLONDSYMPHONY:
			add_subwindow(head_title = new BC_Title(x, y, "Head:"));
//			head_text = new PlaybackHead(this, x1, y);
//			head_text->create_objects();
			y += 25;
//			add_subwindow(head_count_title = new BC_Title(x, y, "Total Heads:"));
//			head_count_text = new PlaybackHeadCount(this, x1, y);
//			head_count_text->create_objects();
			y += 25;
//			add_subwindow(host_title = new BC_Title(x, y, "Hostname:"));
//			add_subwindow(host_text = new PlaybackHost(this, x1, y));
			x = 10;
			y = 390;
			add_subwindow(vdevice_title = new BC_Title(x, y, "Video Driver:"));
			video_device = new VDevicePrefs(x + 100, 
				y, 
				pwindow, 
				this, 
				current_config()->vconfig, 
				0,
				MODEPLAY);
			video_device->initialize();
			break;
	}
	
	return 1;
}

ArrayList<PlaybackConfig*>* VideoOutPrefs::current_config_list()
{
	return &pwindow->thread->edl->session->playback_config[pwindow->thread->edl->session->playback_strategy];
}

PlaybackConfig* VideoOutPrefs::current_config()
{
	return current_config_list()->values[current_head];
}



int VideoOutPrefs::get_buffer_bytes()
{
//	return pwindow->thread->edl->session->aconfig->oss_out_bits / 8 * pwindow->thread->preferences->aconfig->oss_out_channels * pwindow->thread->preferences->playback_buffer;
}


PlaybackNearest::PlaybackNearest(PreferencesWindow *pwindow, VideoOutPrefs *prefs, int value, int x, int y)
 : BC_Radial(x, y, value, "Nearest neighbor")
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}
int PlaybackNearest::handle_event()
{
	prefs->update(NEAREST_NEIGHBOR);
	return 1;
}





PlaybackBicubic::PlaybackBicubic(PreferencesWindow *pwindow, VideoOutPrefs *prefs, int value, int x, int y)
 : BC_Radial(x, y, value, "Bicubic")
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}
int PlaybackBicubic::handle_event()
{
	prefs->update(CUBIC_INTERPOLATE);
	return 1;
}



PlaybackPreload::PlaybackPreload(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	VideoOutPrefs *playback, 
	char *text)
 : BC_TextBox(x, y, 100, 1, text)
{ 
	this->pwindow = pwindow; 
	this->playback = playback; 
}

int PlaybackPreload::handle_event() 
{ 
	pwindow->thread->edl->session->playback_preload = atol(get_text()); 
	return 1;
}



VideoEveryFrame::VideoEveryFrame(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->edl->session->video_every_frame, "Play every frame")
{
	this->pwindow = pwindow;
}

VideoEveryFrame::~VideoEveryFrame()
{
}

int VideoEveryFrame::handle_event()
{
	pwindow->thread->edl->session->video_every_frame = get_value();
	return 1;
}
