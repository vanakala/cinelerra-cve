#include "adeviceprefs.h"
#include "adrivermenu.h"
#include "audioconfig.h"
#include "audiodevice.inc"
#include "audiooutprefs.h"
#include "defaults.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "preferences.h"

#include <string.h>

AudioOutPrefs::AudioOutPrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
	head_text = 0;
	head_count_text = 0;
	host_text = 0;
	video_device = 0;
}

AudioOutPrefs::~AudioOutPrefs()
{
	delete_strategy();
	mwindow->defaults->update("PLAYBACK_HEAD", current_head);
	delete audio_device;
	delete video_device;
}

int AudioOutPrefs::create_objects()
{
	int x = 5, y = 5;
	char string[1024];
	BC_PopupTextBox *popup;

//printf("AudioOutPrefs::create_objects 1\n");
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
	add_subwindow(new BC_Title(x, y, "Audio Out", LARGEFONT, BLACK));
	y += 30;
	add_subwindow(new BC_Title(x, y, "Samples to read from disk at a time:", MEDIUMFONT, BLACK));
	sprintf(string, "%d", pwindow->thread->edl->session->audio_read_length);
	add_subwindow(new PlaybackReadLength(x + 275, y, pwindow, this, string));
	y += 30;
	add_subwindow(new BC_Title(x, y, "Samples to send to console at a time:", MEDIUMFONT, BLACK));
	sprintf(string, "%d", pwindow->thread->edl->session->audio_module_fragment);
	add_subwindow(new PlaybackModuleFragment(x + 275, y, pwindow, this, string));
	y += 30;
	add_subwindow(new PlaybackDisableNoEdits(pwindow, pwindow->thread->edl->session->test_playback_edits, y));
	y += 30;
	add_subwindow(new PlaybackViewFollows(pwindow, pwindow->thread->edl->session->view_follows_playback, y));
	y += 30;
	add_subwindow(new PlaybackSoftwareTimer(pwindow, pwindow->thread->edl->session->playback_software_position, y));
	y += 30;
	add_subwindow(new PlaybackRealTime(pwindow, pwindow->thread->edl->session->real_time_playback, y));
	y += 40;
	add_subwindow(new BC_Title(x, y, "Audio Driver:"));
	audio_device = new ADevicePrefs(x + 100, 
		y, 
		pwindow, 
		this, 
		current_config()->aconfig, 
		0,
		MODEPLAY);
	audio_device->initialize();

// Strategic playback options created here
	set_strategy(pwindow->thread->edl->session->playback_strategy);
// 
// 	add_subwindow(new BC_Title(x, y, "Playback driver:", MEDIUMFONT, BLACK));
// 	add_subwindow(new AudioDriverMenu(x, y + 20, out_device, &(pwindow->thread->preferences->aconfig->audio_out_driver), 0, 1));
// 	y += 70;
// 
	return 0;
}

char* AudioOutPrefs::strategy_to_string(int strategy)
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
void AudioOutPrefs::delete_strategy()
{
	if(head_text) 
	{
		delete head_text;
		delete head_title;
		head_text = 0;
	}
	if(host_text)
	{
		delete host_title;
		delete host_text;
		host_text = 0;
	}
	if(head_count_text)
	{
		delete head_count_title;
		delete head_count_text;
		head_count_text = 0;
	}
	if(video_device)
	{
		delete vdevice_title;
		delete video_device;
		video_device = 0;
	}
}

int AudioOutPrefs::set_strategy(int strategy)
{
	int x = 350, x1 = 450, y = 10;
	delete_strategy();

	pwindow->thread->edl->session->playback_strategy = strategy;
	switch(strategy)
	{
		case PLAYBACK_LOCALHOST:
			break;
		case PLAYBACK_MULTIHEAD:
			add_subwindow(head_title = new BC_Title(x, y, "Head:"));
			head_text = new PlaybackHead(this, x1, y);
			head_text->create_objects();
			y += 25;
			add_subwindow(head_count_title = new BC_Title(x, y, "Total Heads:"));
			head_count_text = new PlaybackHeadCount(this, x1, y);
			head_count_text->create_objects();
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
		case PLAYBACK_BLONDSYMPHONY:
			add_subwindow(head_title = new BC_Title(x, y, "Head:"));
			head_text = new PlaybackHead(this, x1, y);
			head_text->create_objects();
			y += 25;
			add_subwindow(head_count_title = new BC_Title(x, y, "Total Heads:"));
			head_count_text = new PlaybackHeadCount(this, x1, y);
			head_count_text->create_objects();
			y += 25;
			add_subwindow(host_title = new BC_Title(x, y, "Hostname:"));
			add_subwindow(host_text = new PlaybackHost(this, x1, y));
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

ArrayList<PlaybackConfig*>* AudioOutPrefs::current_config_list()
{
	return &pwindow->thread->edl->session->playback_config[pwindow->thread->edl->session->playback_strategy];
}

PlaybackConfig* AudioOutPrefs::current_config()
{
	return current_config_list()->values[current_head];
}



int AudioOutPrefs::get_buffer_bytes()
{
//	return pwindow->thread->edl->aconfig->oss_out_bits / 8 * pwindow->thread->preferences->aconfig->oss_out_channels * pwindow->thread->preferences->playback_buffer;
}






PlaybackStrategy::PlaybackStrategy(AudioOutPrefs *prefs, 
	int x, 
	int y)
 : BC_PopupTextBox(prefs, 
		&prefs->strategies,
		prefs->strategy_to_string(prefs->pwindow->thread->edl->session->playback_strategy),
		x, 
		y, 
		200,
		100)
{
	this->prefs = prefs;
}

int PlaybackStrategy::handle_event()
{
	prefs->set_strategy(get_number());
	return 1;
}





PlaybackHead::PlaybackHead(AudioOutPrefs *prefs, 
	int x, 
	int y)
 : BC_TumbleTextBox(prefs, 
		prefs->current_head,
		0, 
		prefs->current_config_list()->total - 1, 
		x,
		y,
		100)
{
	this->prefs = prefs;
}

int PlaybackHead::handle_event()
{
	return 1;
}

PlaybackHeadCount::PlaybackHeadCount(AudioOutPrefs *prefs, 
	int x, 
	int y)
 : BC_TumbleTextBox(prefs, 
		prefs->current_config_list()->total, 
		1, 
		MAX_CHANNELS,
		x,
		y,
		100)
{
	this->prefs = prefs;
}

int PlaybackHeadCount::handle_event()
{
	return 1;
}



PlaybackHost::PlaybackHost(AudioOutPrefs *prefs, int x, int y)
 : BC_TextBox(x, y, 100, 1, prefs->current_config()->hostname)
{ 
	this->prefs = prefs; 
}

int PlaybackHost::handle_event() 
{ 
	strcpy(prefs->current_config()->hostname, get_text()); 
	return 1;
}




PlaybackReadLength::PlaybackReadLength(int x, int y, PreferencesWindow *pwindow, AudioOutPrefs *playback, char *text)
 : BC_TextBox(x, y, 100, 1, text)
{ 
	this->pwindow = pwindow; 
	this->playback = playback; 
}

int PlaybackReadLength::handle_event() 
{ 
	pwindow->thread->edl->session->audio_read_length = atol(get_text()); 
	return 1;
}




PlaybackModuleFragment::PlaybackModuleFragment(int x, int y, PreferencesWindow *pwindow, AudioOutPrefs *playback, char *text)
 : BC_TextBox(x, y, 100, 1, text)
{ 
	this->pwindow = pwindow; 
}

int PlaybackModuleFragment::handle_event() 
{ 
	pwindow->thread->edl->session->audio_module_fragment = atol(get_text()); 
	return 1;
}




PlaybackBufferSize::PlaybackBufferSize(int x, int y, PreferencesWindow *pwindow, AudioOutPrefs *playback, char *text)
 : BC_TextBox(x, y, 100, 1, text)
{ 
	this->pwindow = pwindow; 
	this->playback = playback; 
}

int PlaybackBufferSize::handle_event() 
{ 
	pwindow->thread->edl->session->playback_buffer = atol(get_text()); 
	return 1;
}

PlaybackBufferBytes::PlaybackBufferBytes(int x, int y, PreferencesWindow *pwindow, AudioOutPrefs *playback, char *text)
 : BC_Title(x, y, text)
{
	this->pwindow = pwindow; 
	this->playback = playback;
}
int PlaybackBufferBytes::update_bytes()
{
	sprintf(string, "%d", playback->get_buffer_bytes());
	update(string);
	return 1;
}

PlaybackDisableNoEdits::PlaybackDisableNoEdits(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, "Disable tracks when no edits.")
{ 
	this->pwindow = pwindow; 
}

int PlaybackDisableNoEdits::handle_event() 
{ 
	pwindow->thread->edl->session->test_playback_edits = get_value(); 
	return 1;
}




PlaybackViewFollows::PlaybackViewFollows(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, "View follows playback")
{ 
	this->pwindow = pwindow; 
}

int PlaybackViewFollows::handle_event() 
{ 
	pwindow->thread->edl->session->view_follows_playback = get_value(); 
	return 1;
}




PlaybackSoftwareTimer::PlaybackSoftwareTimer(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, "Use software for positioning information")
{ 
	this->pwindow = pwindow; 
}

int PlaybackSoftwareTimer::handle_event() 
{ 
	pwindow->thread->edl->session->playback_software_position = get_value(); 
	return 1;
}




PlaybackRealTime::PlaybackRealTime(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, "Audio playback in real time priority (root only)")
{ 
	this->pwindow = pwindow; 
}

int PlaybackRealTime::handle_event() 
{ 
	pwindow->thread->edl->session->real_time_playback = get_value(); 
	return 1;
}
