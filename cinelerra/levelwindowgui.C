// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "mainsession.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "theme.h"

LevelWindowGUI::LevelWindowGUI()
 : BC_Window(MWindow::create_title(N_("Levels")),
	mainsession->lwindow_x,
	mainsession->lwindow_y,
	mainsession->lwindow_w,
	mainsession->lwindow_h,
	10,
	10,
	1,
	0, 
	1)
{
	set_icon(mwindow_global->get_window_icon());
	theme_global->draw_lwindow_bg(this);
	panel = new MeterPanel(this,
		5,
		5,
		get_h() - 10,
		edlsession->audio_channels,
		1);
}

LevelWindowGUI::~LevelWindowGUI()
{
	delete panel;
}

void LevelWindowGUI::resize_event(int w, int h)
{
	mainsession->lwindow_x = get_x();
	mainsession->lwindow_y = get_y();
	mainsession->lwindow_w = w;
	mainsession->lwindow_h = h;

	theme_global->draw_lwindow_bg(this);

	panel->reposition_window(panel->x,
		panel->y,
		h - 10);

	BC_WindowBase::resize_event(w, h);
}

void LevelWindowGUI::translation_event()
{
	mainsession->lwindow_x = get_x();
	mainsession->lwindow_y = get_y();
}

void LevelWindowGUI::close_event()
{
	hide_window();
	mwindow_global->mark_lwindow_hidden();
}

int LevelWindowGUI::keypress_event()
{
	if(get_keypress() == 'w' || get_keypress() == 'W')
	{
		close_event();
		return 1;
	}
	return 0;
}
