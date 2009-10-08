
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

#include "autoconf.h"
#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "gwindowgui.h"
#include "language.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "trackcanvas.h"





GWindowGUI::GWindowGUI(MWindow *mwindow,
	int w,
	int h)
 : BC_Window(PROGRAM_NAME N_(": Overlays"),
 	mwindow->session->gwindow_x, 
    mwindow->session->gwindow_y, 
    w, 
    h,
    w,
    h,
    0,
    0,
    1)
{
	this->mwindow = mwindow;
}

static char *other_text[NONAUTOTOGGLES_COUNT] =
{
	N_("Assets"),
	N_("Titles"),
	N_("Transitions"),
	N_("Plugin Autos")
};

static char *auto_text[] = 
{
	N_("Mute"),
	N_("Camera X"),
	N_("Camera Y"),
	N_("Camera Z"),
	N_("Projector X"),
	N_("Projector Y"),
	N_("Projector Z"),
	N_("Fade"),
	N_("Pan"),
	N_("Mode"),
	N_("Mask"),
	N_("Nudge")
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
	{1, AUTOMATION_CAMERA_X},
	{1, AUTOMATION_CAMERA_Y},
	{1, AUTOMATION_CAMERA_Z},
	{1, AUTOMATION_PROJECTOR_X},
	{1, AUTOMATION_PROJECTOR_Y},
	{1, AUTOMATION_PROJECTOR_Z},
};

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
			toggle_order[i].isauto ? auto_text[toggle_order[i].ref] : other_text[toggle_order[i].ref]);
		*w = MAX(current_w, *w);
		*h += current_h + 5;
	}

	*h += 10;
	*w += 20;
}



void GWindowGUI::create_objects()
{
	int x = 10, y = 10;


	for(int i = 0; i < NONAUTOTOGGLES_COUNT + AUTOMATION_TOTAL; i++)
	{
		add_tool(toggles[i] = new GWindowToggle(mwindow, 
			this, 
			x, 
			y, 
			toggle_order[i]));
		y += toggles[i]->get_h() + 5;
	}
}

void GWindowGUI::update_mwindow()
{
	unlock_window();
	mwindow->gui->mainmenu->update_toggles(1);
	lock_window("GWindowGUI::update_mwindow");
}

void GWindowGUI::update_toggles(int use_lock)
{
	if(use_lock) lock_window("GWindowGUI::update_toggles");

	for(int i = 0; i < NONAUTOTOGGLES_COUNT + AUTOMATION_TOTAL; i++)
	{
		toggles[i]->update();
	}

	if(use_lock) unlock_window();
}

int GWindowGUI::translation_event()
{
	mwindow->session->gwindow_x = get_x();
	mwindow->session->gwindow_y = get_y();
	return 0;
}

int GWindowGUI::close_event()
{
	hide_window();
	mwindow->session->show_gwindow = 0;
	unlock_window();

	mwindow->gui->lock_window("GWindowGUI::close_event");
	mwindow->gui->mainmenu->show_gwindow->set_checked(0);
	mwindow->gui->unlock_window();

	lock_window("GWindowGUI::close_event");
	mwindow->save_defaults();
	return 1;
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






GWindowToggle::GWindowToggle(MWindow *mwindow, 
	GWindowGUI *gui, 
	int x, 
	int y, 
	toggleinfo toggleinf)
 : BC_CheckBox(x, 
 	y, 
	*get_main_value(mwindow, toggleinf), 
        toggleinf.isauto ? auto_text[toggleinf.ref] : other_text[toggleinf.ref])
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->toggleinf = toggleinf;
}

int GWindowToggle::handle_event()
{
	*get_main_value(mwindow, toggleinf) = get_value();
	gui->update_mwindow();


// Update stuff in MWindow
	unlock_window();
	mwindow->gui->lock_window("GWindowToggle::handle_event");
	if(toggleinf.isauto)
	{
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
	}
	else
	{
		switch(toggleinf.ref)
		{
			case NONAUTOTOGGLES_ASSETS:
			case NONAUTOTOGGLES_TITLES:
				mwindow->gui->update(1,
					1,
					0,
					0,
					1, 
					0,
					0);
				break;

			case NONAUTOTOGGLES_TRANSITIONS:
			case NONAUTOTOGGLES_PLUGIN_AUTOS:
				mwindow->gui->canvas->draw_overlays();
				mwindow->gui->canvas->flash();
				break;
		}
	}

	mwindow->gui->unlock_window();
	lock_window("GWindowToggle::handle_event");

	return 1;
}

int* GWindowToggle::get_main_value(MWindow *mwindow, toggleinfo toggleinf)
{
	if(toggleinf.isauto)
	{
		return &mwindow->edl->session->auto_conf->autos[toggleinf.ref];
	}
	else
	{
		switch(toggleinf.ref)
		{
			case NONAUTOTOGGLES_ASSETS:
				return &mwindow->edl->session->show_assets;
				break;
			case NONAUTOTOGGLES_TITLES:
				return &mwindow->edl->session->show_titles;
				break;
			case NONAUTOTOGGLES_TRANSITIONS:
				return &mwindow->edl->session->auto_conf->transitions;
				break;
			case NONAUTOTOGGLES_PLUGIN_AUTOS:
				return &mwindow->edl->session->auto_conf->plugins;
				break;
		}
	}
}

void GWindowToggle::update()
{
	set_value(*get_main_value(mwindow, toggleinf));
}



