// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "autoconf.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "cinelerra.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "gwindowgui.h"
#include "language.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindow.h"
#include "theme.h"

static const char *other_text[NONAUTOTOGGLES_COUNT] =
{
	N_("Assets"),
	N_("Titles"),
	N_("Transitions"),
	N_("Plugin Autos")
};

static toggleinfo toggle_order[] = 
{
	{0, NONAUTOTOGGLES_ASSETS},
	{0, NONAUTOTOGGLES_TITLES},
	{0, NONAUTOTOGGLES_TRANSITIONS},
	{1, AUTOMATION_FADE},
	{1, AUTOMATION_MUTE},
	{1, AUTOMATION_MODE},
	{1, AUTOMATION_PAN},
	{0, NONAUTOTOGGLES_PLUGIN_AUTOS},
	{1, AUTOMATION_MASK},
	{1, AUTOMATION_CROP },
	{1, AUTOMATION_CAMERA_X},
	{1, AUTOMATION_CAMERA_Y},
	{1, AUTOMATION_CAMERA_Z},
	{1, AUTOMATION_PROJECTOR_X},
	{1, AUTOMATION_PROJECTOR_Y},
	{1, AUTOMATION_PROJECTOR_Z},
};

GWindowGUI::GWindowGUI(int w, int h)
 : BC_Window(MWindow::create_title(N_("Overlays")),
	mainsession->gwindow_x,
	mainsession->gwindow_y,
	w,
	h,
	w,
	h,
	0,
	0,
	1)
{
	int x = 10, y = 10;

	set_icon(mwindow_global->get_window_icon());
	for(int i = 0; i < NONAUTOTOGGLES_COUNT + AUTOMATION_TOTAL; i++)
	{
		add_tool(toggles[i] = new GWindowToggle(this,
			x,
			y,
			toggle_order[i]));
		y += toggles[i]->get_h() + 5;
	}
}

void GWindowGUI::calculate_extents(BC_WindowBase *gui, int *w, int *h)
{
	int temp1, temp2, temp3, temp4, temp5, temp6, temp7;
	int current_w, current_h;

	*w = 10;
	*h = 10;
	for(int i = 0; i < NONAUTOTOGGLES_COUNT + AUTOMATION_TOTAL; i++)
	{
		BC_Toggle::calculate_extents(gui, 
			BC_WindowBase::get_resources()->checkbox_images,
			0,
			&temp1,
			&current_w,
			&current_h,
			&temp2,
			&temp3,
			&temp4,
			&temp5, 
			&temp6,
			&temp7, 
			toggle_order[i].isauto ? _(Automation::automation_tbl[toggle_order[i].ref].name) :
				_(other_text[toggle_order[i].ref]));
		*w = MAX(current_w, *w);
		*h += current_h + 5;
	}

	*h += 10;
	*w += 20;
}

void GWindowGUI::update_toggles()
{
	for(int i = 0; i < NONAUTOTOGGLES_COUNT + AUTOMATION_TOTAL; i++)
	{
		toggles[i]->update();
	}
}

void GWindowGUI::translation_event()
{
	mainsession->gwindow_x = get_x();
	mainsession->gwindow_y = get_y();
}

void GWindowGUI::close_event()
{
	hide_window();
	mwindow_global->mark_gwindow_hidden();
}

int GWindowGUI::keypress_event()
{
	switch(get_keypress())
	{
		case 'w':
		case 'W':
			if(ctrl_down())
			{
				close_event();
				return 1;
			}
			break;
	}
	return 0;
}


GWindowToggle::GWindowToggle(GWindowGUI *gui,
	int x, 
	int y, 
	toggleinfo toggleinf)
 : BC_CheckBox(x, 
	y,
	*get_main_value(toggleinf),
	toggleinf.isauto ? _(Automation::automation_tbl[toggleinf.ref].name) :
		_(other_text[toggleinf.ref]))
{
	this->gui = gui;
	this->toggleinf = toggleinf;
}

int GWindowToggle::handle_event()
{
	*get_main_value(toggleinf) = get_value();
	mwindow_global->update_gui(WUPD_TOGGLES);

// Update stuff in MWindow
	if(toggleinf.isauto)
		mwindow_global->draw_canvas_overlays();
	else
	{
		switch(toggleinf.ref)
		{
			case NONAUTOTOGGLES_ASSETS:
			case NONAUTOTOGGLES_TITLES:
				mwindow_global->update_gui(WUPD_SCROLLBARS |
					WUPD_CANVINCR | WUPD_PATCHBAY);
				break;

			case NONAUTOTOGGLES_TRANSITIONS:
			case NONAUTOTOGGLES_PLUGIN_AUTOS:
				mwindow_global->draw_canvas_overlays();
				break;
		}
	}
	return 1;
}

int* GWindowToggle::get_main_value(toggleinfo toggleinf)
{
	if(toggleinf.isauto)
	{
		return &edlsession->auto_conf->auto_visible[toggleinf.ref];
	}
	else
	{
		switch(toggleinf.ref)
		{
		case NONAUTOTOGGLES_ASSETS:
			return &edlsession->show_assets;

		case NONAUTOTOGGLES_TITLES:
			return &edlsession->show_titles;

		case NONAUTOTOGGLES_TRANSITIONS:
			return &edlsession->auto_conf->transitions_visible;

		case NONAUTOTOGGLES_PLUGIN_AUTOS:
			return &edlsession->keyframes_visible;
		}
	}
}

void GWindowToggle::update()
{
	set_value(*get_main_value(toggleinf));
}
