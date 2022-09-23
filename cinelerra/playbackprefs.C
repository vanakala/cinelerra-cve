// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "adeviceprefs.h"
#include "audiodevice.inc"
#include "bcbar.h"
#include "bchash.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "colormodels.inc"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "guielements.h"
#include "language.h"
#include "playbackprefs.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "theme.h"
#include "vdeviceprefs.h"
#include "videodevice.inc"

const struct selection_int PlaybackPrefs::scalings[] =
{
	{ N_("Lanczos enlarge and reduce"), LANCZOS_LANCZOS },
	{ N_("Bicubic enlarge and reduce"), CUBIC_CUBIC },
	{ N_("Bicubic enlarge and bilinear reduce"), CUBIC_LINEAR },
	{ N_("Bilinear enlarge and bilinear reduce"), LINEAR_LINEAR },
	{ N_("Nearest neighbor enlarge and reduce"), NEAREST_NEIGHBOR },
	{ 0, 0 }
};

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
	BC_PopupTextBox *popup;
	BC_Resources *resources = BC_WindowBase::get_resources();
	BC_WindowBase *win;
	int maxw;
	BC_Title *title1;
	RadialSelection *rsel;

	playback_config = pwindow->thread->this_edlsession->playback_config;

	x = theme_global->preferencesoptions_x;
	y = theme_global->preferencesoptions_y;

// Audio
	add_subwindow(new BC_Title(x, y, _("Audio Out"), LARGEFONT));

	y += get_text_height(LARGEFONT) + 5;
	x2 = x;
	add_subwindow(title1 = new BC_Title(x2, y, _("Audio offset (sec):")));
	x2 += title1->get_w() + 5;
	DblValueTumbleTextBox *ttbox = new DblValueTumbleTextBox(x2, y, this,
		&playback_config->aconfig->audio_offset,
		-10.0, 10.0, 0.01, GUIELEM_VAL_W, 2);
	y += ttbox->get_h() + 5;
	win = add_subwindow(new CheckBox(x, y, _("View follows playback"),
		&pwindow->thread->this_edlsession->view_follows_playback));
	y += win->get_h() + 3;
	win = add_subwindow(new CheckBox(x, y, _("Use software for positioning information"),
		&pwindow->thread->this_edlsession->playback_software_position));

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

	win = add_subwindow(new CheckBox(x, y, _("Play every frame"),
		&pwindow->thread->this_edlsession->video_every_frame));
	y1 = y + 2;
	y += win->get_h() + 20;

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

	add_subwindow(rsel = new RadialSelection(x, y, scalings,
		&BC_Resources::interpolation_method));
	rsel->show();

	y += rsel->get_h();

	x2 = x;
	x += 370;
	win = add_subwindow(new BC_Title(x, y, _("Timecode offset:"), MEDIUMFONT));
	x += win->get_w();

	win = add_subwindow(new ValueTextBox(x, y,
		&pwindow->thread->this_edlsession->timecode_offset[3], 30));
	win = add_subwindow(new BC_Title(win->get_x() + win->get_w(), y, ":", MEDIUMFONT));
	win = add_subwindow(new ValueTextBox(win->get_x() + win->get_w(), y,
		&pwindow->thread->this_edlsession->timecode_offset[2], 30));
	win = add_subwindow(new BC_Title(win->get_x() + win->get_w(), y, ":", MEDIUMFONT));
	win = add_subwindow(new ValueTextBox(win->get_x() + win->get_w(), y,
		&pwindow->thread->this_edlsession->timecode_offset[1], 30));
	win = add_subwindow(new BC_Title(win->get_x() + win->get_w(), y, ":", MEDIUMFONT));
	win = add_subwindow(new ValueTextBox(win->get_x() + win->get_w(), y,
		&pwindow->thread->this_edlsession->timecode_offset[0], 30));
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
