// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "adeviceprefs.h"
#include "audiodevice.inc"
#include "bcbar.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "colormodels.inc"
#include "clip.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "playbackprefs.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "theme.h"
#include "vdeviceprefs.h"
#include "videodevice.inc"


PlaybackPrefs::PlaybackPrefs(PreferencesWindow *pwindow)
 : PreferencesDialog(pwindow)
{
	video_device = 0;
}

PlaybackPrefs::~PlaybackPrefs()
{
	delete audio_device;
	delete video_device;
}

void PlaybackPrefs::show()
{
	int x, y, y1, x2, y2, wt, wh;
	char string[BCTEXTLEN];
	BC_PopupTextBox *popup;
	BC_WindowBase *window;
	BC_Resources *resources = BC_WindowBase::get_resources();
	BC_WindowBase *win;
	int maxw;
	BC_Title *title1;

	playback_config = pwindow->thread->this_edlsession->playback_config;

	x = theme_global->preferencesoptions_x;
	y = theme_global->preferencesoptions_y;

// Audio
	add_subwindow(new BC_Title(x, 
		y, 
		_("Audio Out"), 
		LARGEFONT));

	y += get_text_height(LARGEFONT) + 5;
	x2 = x;
	add_subwindow(title1 = new BC_Title(x2, y, _("Audio offset (sec):")));
	x2 += title1->get_w() + 5;
	PlaybackAudioOffset *audio_offset = new PlaybackAudioOffset(pwindow,
		this,
		x2,
		y);
	y += audio_offset->get_h() + 5;

	win = add_subwindow(new PlaybackViewFollows(pwindow, pwindow->thread->this_edlsession->view_follows_playback, y));
	y += win->get_h();
	win = add_subwindow(new PlaybackSoftwareTimer(pwindow, pwindow->thread->this_edlsession->playback_software_position, y));

	y += win->get_h() + 10;
	win = add_subwindow(new BC_Title(x, y, _("Audio Driver:")));
	y += win->get_h();
	audio_device = new ADevicePrefs(x + 55,
		y, 
		pwindow, 
		this, 
		playback_config->aconfig);
	audio_device->initialize();

// Video
	y += audio_device->get_h();

	add_subwindow(new BC_Bar(5, y, get_w() - 10));
	y += 5;

	add_subwindow(new BC_Title(x, y, _("Video Out"), LARGEFONT));
	y += 30;

	win = add_subwindow(window = new VideoEveryFrame(pwindow, this, x, y));
	y1 = y + 2;
	y += window->get_h() + 20;

	y2 = y1;
	x2 = x + 370;
	win = add_subwindow(new BC_Title(x2, y2, _("Framerate achieved:")));
	wt = win->get_w();
	wh = win->get_h() + 5;
	y2 += wh;
	win = add_subwindow(new BC_Title(x2, y2, _("Frames played:")));
	if(win->get_w() > wt)
		wt = win->get_w();
	y2 += wh;
	win = add_subwindow(new BC_Title(x2, y2, _("Late frames:")));
	if(win->get_w() > wt)
		wt = win->get_w();
	y2 += wh;
	win = add_subwindow(new BC_Title(x2, y2, _("Average delay:")));
	y2 = y1;
	x2 += wt + 10;
	add_subwindow(framerate_title = new BC_Title(x2, y2, "--", MEDIUMFONT, RED));
	y2 += wh;
	add_subwindow(playedframes_title = new BC_Title(x2, y2, "-", MEDIUMFONT, RED));
	y2 += wh;
	add_subwindow(lateframes_title = new BC_Title(x2, y2, "-", MEDIUMFONT, RED));
	y2 += wh;
	add_subwindow(avgdelay_title = new BC_Title(x2, y2, "-", MEDIUMFONT, RED));
	draw_framerate();

	add_subwindow(new BC_Title(x, y, _("Scaling equation:")));
	y += 20;
	add_subwindow(lanczos_lanczos = new PlaybackLanczosLanczos(pwindow,
		this,
		BC_Resources::interpolation_method == LANCZOS_LANCZOS,
		10, 
		y));
	y += 20;
	add_subwindow(cubic_cubic = new PlaybackBicubicBicubic(pwindow,
		this, 
		BC_Resources::interpolation_method == CUBIC_CUBIC,
		10, 
		y));
	y += 20;
	add_subwindow(cubic_linear = new PlaybackBicubicBilinear(pwindow, 
		this, 
		BC_Resources::interpolation_method == CUBIC_LINEAR,
		10, 
		y));
	y += 20;
	add_subwindow(linear_linear = new PlaybackBilinearBilinear(pwindow, 
		this, 
		BC_Resources::interpolation_method == LINEAR_LINEAR,
		10, 
		y));
	y += 20;
	add_subwindow(nearest_neighbor = new PlaybackNearest(pwindow, 
		this, 
		BC_Resources::interpolation_method == NEAREST_NEIGHBOR,
		10, 
		y));

	y += 35;

	x2 = x;
	x += 370;
	win = add_subwindow(new BC_Title(x, y, _("Timecode offset:"), MEDIUMFONT));
	x += win->get_w();
	sprintf(string, "%d", pwindow->thread->this_edlsession->timecode_offset[3]);
	win = add_subwindow(new TimecodeOffset(x, y, pwindow, this, string, 3));
	win = add_subwindow(new BC_Title(win->get_x() + win->get_w(), y, ":", MEDIUMFONT));
	sprintf(string, "%d", pwindow->thread->this_edlsession->timecode_offset[2]);
	win = add_subwindow(new TimecodeOffset(win->get_x() + win->get_w(), y, pwindow, this, string, 2));
	win = add_subwindow(new BC_Title(win->get_x() + win->get_w(), y, ":", MEDIUMFONT));
	sprintf(string, "%d", pwindow->thread->this_edlsession->timecode_offset[1]);
	win = add_subwindow(new TimecodeOffset(win->get_x() + win->get_w(), y, pwindow, this, string, 1));
	win = add_subwindow(new BC_Title(win->get_x() + win->get_w(), y, ":", MEDIUMFONT));
	sprintf(string, "%d", pwindow->thread->this_edlsession->timecode_offset[0]);
	add_subwindow(new TimecodeOffset(win->get_x() + win->get_w(), y, pwindow, this, string, 0));
	x = x2;
	y += 10;
	add_subwindow(vdevice_title = new BC_Title(x, y, _("Video Driver:")));
	y += vdevice_title->get_h();
	video_device = new VDevicePrefs(55,
		y, 
		pwindow, 
		this, 
		playback_config->vconfig);
	video_device->initialize();
}


void PlaybackPrefs::update(int interpolation)
{
	BC_Resources::interpolation_method = interpolation;
	nearest_neighbor->update(interpolation == NEAREST_NEIGHBOR);
	lanczos_lanczos->update(interpolation == LANCZOS_LANCZOS);
	cubic_cubic->update(interpolation == CUBIC_CUBIC);
	cubic_linear->update(interpolation == CUBIC_LINEAR);
	linear_linear->update(interpolation == LINEAR_LINEAR);
}

void PlaybackPrefs::draw_framerate()
{
	framerate_title->update(pwindow->thread->this_edlsession->actual_frame_rate, 2);
}

void PlaybackPrefs::draw_playstatistics()
{
	playedframes_title->update(pwindow->thread->this_edlsession->frame_count);
	lateframes_title->update(pwindow->thread->this_edlsession->frames_late);
	avgdelay_title->update(pwindow->thread->this_edlsession->avg_delay);
}

PlaybackAudioOffset::PlaybackAudioOffset(PreferencesWindow *pwindow, 
	PlaybackPrefs *playback, 
	int x, 
	int y)
 : BC_TumbleTextBox(playback,
	playback->playback_config->aconfig->audio_offset,
	-10.0,
	10.0,
	x,
	y,
	100)
{
	this->pwindow = pwindow;
	this->playback = playback;
	set_precision(2);
	set_increment(0.1);
}

int PlaybackAudioOffset::handle_event()
{
	playback->playback_config->aconfig->audio_offset = atof(get_text());
	return 1;
}

PlaybackViewFollows::PlaybackViewFollows(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, _("View follows playback"))
{ 
	this->pwindow = pwindow; 
}

int PlaybackViewFollows::handle_event() 
{ 
	pwindow->thread->this_edlsession->view_follows_playback = get_value();
	return 1;
}


PlaybackSoftwareTimer::PlaybackSoftwareTimer(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, _("Use software for positioning information"))
{ 
	this->pwindow = pwindow; 
}

int PlaybackSoftwareTimer::handle_event() 
{ 
	pwindow->thread->this_edlsession->playback_software_position = get_value();
	return 1;
}


PlaybackNearest::PlaybackNearest(PreferencesWindow *pwindow, PlaybackPrefs *prefs, int value, int x, int y)
 : BC_Radial(x, y, value, _("Nearest neighbor enlarge and reduce"))
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}

int PlaybackNearest::handle_event()
{
	prefs->update(NEAREST_NEIGHBOR);
	return 1;
}


PlaybackLanczosLanczos::PlaybackLanczosLanczos(PreferencesWindow *pwindow, PlaybackPrefs *prefs, int value, int x, int y)
 : BC_Radial(x, y, value, _("Lanczos enlarge and reduce"))
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}

int PlaybackLanczosLanczos::handle_event()
{
	prefs->update(LANCZOS_LANCZOS);
	return 1;
}


PlaybackBicubicBicubic::PlaybackBicubicBicubic(PreferencesWindow *pwindow, PlaybackPrefs *prefs, int value, int x, int y)
 : BC_Radial(x, y, value, _("Bicubic enlarge and reduce"))
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}
int PlaybackBicubicBicubic::handle_event()
{
	prefs->update(CUBIC_CUBIC);
	return 1;
}


PlaybackBicubicBilinear::PlaybackBicubicBilinear(PreferencesWindow *pwindow, PlaybackPrefs *prefs, int value, int x, int y)
 : BC_Radial(x, y, value, _("Bicubic enlarge and bilinear reduce"))
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}

int PlaybackBicubicBilinear::handle_event()
{
	prefs->update(CUBIC_LINEAR);
	return 1;
}


PlaybackBilinearBilinear::PlaybackBilinearBilinear(PreferencesWindow *pwindow, 
	PlaybackPrefs *prefs, 
	int value, 
	int x, 
	int y)
 : BC_Radial(x, y, value, _("Bilinear enlarge and bilinear reduce"))
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}

int PlaybackBilinearBilinear::handle_event()
{
	prefs->update(LINEAR_LINEAR);
	return 1;
}

VideoEveryFrame::VideoEveryFrame(PreferencesWindow *pwindow, 
	PlaybackPrefs *playback_prefs,
	int x, 
	int y)
 : BC_CheckBox(x, y, pwindow->thread->this_edlsession->video_every_frame, _("Play every frame"))
{
	this->pwindow = pwindow;
	this->playback_prefs = playback_prefs;
}

int VideoEveryFrame::handle_event()
{
	pwindow->thread->this_edlsession->video_every_frame = get_value();
	return 1;
}

TimecodeOffset::TimecodeOffset(int x, int y, PreferencesWindow *pwindow, 
      PlaybackPrefs *playback, char *text, int unit)
 : BC_TextBox(x, y, 30, 1, text)
{
	this->pwindow = pwindow;
	this->playback = playback;
	this->unit = unit;
}

int TimecodeOffset::handle_event()
{
	pwindow->thread->this_edlsession->timecode_offset[unit] = atol(get_text());
	return 1;
}
