// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "bcresources.h"
#include "bctitle.h"
#include "clip.h"
#include "keys.h"
#include "language.h"
#include "mainsession.h"
#include "mwindow.h"
#include "preferences.h"
#include "theme.h"
#include "tipwindow.h"
#include "vframe.h"


// Table of tips of the day
static const char *tips[] = 
{
	N_("When configuring slow effects, disable playback for the track.  After configuring it,\n"
	"re-enable playback to process a single frame."),

	N_("Ctrl + any transport command causes playback to only cover\n"
	"the region defined by the in/out points."),

	N_("Shift + clicking a patch causes all other patches except the\n"
	"selected one to toggle."),

	N_("Clicking on a patch and dragging across other tracks causes\n"
	"the other patches to match the first one."),

	N_("Shift + clicking on an effect boundary causes dragging to affect\n"
	"just the one effect."),

	N_("Load multiple files by clicking on one file and shift + clicking on\n"
	"another file.  Ctrl + clicking toggles individual files."),

	N_("Ctrl + left clicking on the time bar cycles forward a time format.\n"
	"Ctrl + middle clicking on the time bar cycles backward a time format."),

	N_("Use the +/- keys in the Compositor window to zoom in and out.\n"),

	N_("Pressing Alt while clicking in the cropping window causes translation of\n"
	"all 4 points.\n"),

	N_("Pressing Tab over a track toggles the Record status.\n"
	"Pressing Shift-Tab over a track toggles the Record status of all the other tracks.\n"),

	N_("Audio->Map 1:1 maps each recordable audio track to a different channel.\n"
	"Map 5.1:2 maps 6 recordable AC3 tracks to 2 channels.\n"),

	N_("Alt + left moves to the previous edit handle.\n"
	"Alt + right moves to the next edit handle.\n")
};

static int total_tips = sizeof(tips) / sizeof(char*);


TipWindow::TipWindow()
 : BC_DialogThread()
{
}

BC_Window* TipWindow::new_gui()
{
	int x, y;

	BC_Resources::get_abs_cursor(&x, &y);
	TipWindowGUI *gui = this->gui = new TipWindowGUI(this,
		x,
		y);
	return gui;
}

char* TipWindow::get_current_tip()
{
	CLAMP(mainsession->current_tip, 0, total_tips - 1);
	char *result = _(tips[mainsession->current_tip]);
	mainsession->current_tip++;
	if(mainsession->current_tip >= total_tips)
		mainsession->current_tip = 0;
	mwindow_global->save_defaults();
	return result;
}

void TipWindow::next_tip()
{
	gui->tip_text->update(get_current_tip());
}

void TipWindow::prev_tip()
{
	for(int i = 0; i < 2; i++)
	{
		mainsession->current_tip--;
		if(mainsession->current_tip < 0)
			mainsession->current_tip = total_tips - 1;
	}

	gui->tip_text->update(get_current_tip());
}


TipWindowGUI::TipWindowGUI(TipWindow *thread,
	int x,
	int y)
 : BC_Window(MWindow::create_title(N_("Tip of the day")),
	x,
	y,
	640,
	100,
	640,
	100,
	0,
	0,
	1)
{
	this->thread = thread;

	x = y = 10;
	add_subwindow(tip_text = new BC_Title(x, y, thread->get_current_tip()));
	y = get_h() - 30;

	set_icon(mwindow_global->get_window_icon());

	BC_CheckBox *checkbox;
	add_subwindow(checkbox = new TipDisable(this, x, y));

	BC_Button *button;
	y = get_h() - TipClose::calculate_h() - 10;
	x = get_w() - TipClose::calculate_w() - 10;
	add_subwindow(button = new TipClose(this, x, y));

	x -= TipNext::calculate_w() + 10;
	add_subwindow(button = new TipNext(this, x, y));

	x -= TipPrev::calculate_w() + 10;
	add_subwindow(button = new TipPrev(this, x, y));

	x += button->get_w() + 10;

	show_window();
	raise_window();
}

int TipWindowGUI::keypress_event()
{
	switch(get_keypress())
	{
		case RETURN:
		case ESC:
		case 'w':
			set_done(0);
			return 1;
	}
	return 0;
}


TipDisable::TipDisable(TipWindowGUI *gui, int x, int y)
 : BC_CheckBox(x, y, preferences_global->use_tipwindow,
	_("Show tip of the day."))
{
	this->gui = gui;
}

int TipDisable::handle_event()
{
	preferences_global->use_tipwindow = get_value();
	return 1;
}


TipNext::TipNext(TipWindowGUI *gui, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("next_tip"))
{
	this->gui = gui;
	set_tooltip(_("Next tip"));
}

int TipNext::handle_event()
{
	gui->thread->next_tip();
	return 1;
}

int TipNext::calculate_w()
{
	return theme_global->get_image_set("next_tip")[0]->get_w();
}


TipPrev::TipPrev(TipWindowGUI *gui, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("prev_tip"))
{
	this->gui = gui;
	set_tooltip(_("Previous tip"));
}

int TipPrev::handle_event()
{
	gui->thread->prev_tip();
	return 1;
}

int TipPrev::calculate_w()
{
	return theme_global->get_image_set("prev_tip")[0]->get_w();
}


TipClose::TipClose(TipWindowGUI *gui, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("close_tip"))
{
	this->gui = gui;
	set_tooltip(_("Close"));
}

int TipClose::handle_event()
{
	gui->set_done(0);
	return 1;
}

int TipClose::calculate_w()
{
	return theme_global->get_image_set("close_tip")[0]->get_w();
}

int TipClose::calculate_h()
{
	return theme_global->get_image_set("close_tip")[0]->get_h();
}
