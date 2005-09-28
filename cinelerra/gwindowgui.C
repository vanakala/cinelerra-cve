#include "autoconf.h"
#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "gwindowgui.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "trackcanvas.h"





GWindowGUI::GWindowGUI(MWindow *mwindow,
	int w,
	int h)
 : BC_Window(PROGRAM_NAME ": Overlays",
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

static char *other_text[OTHER_TOGGLES] =
{
	"Assets",
	"Titles",
	"Transitions",
	"Plugin Autos"
};

static char *auto_text[] = 
{
	"Mute",
	"Camera X",
	"Camera Y",
	"Camera Z",
	"Projector X",
	"Projector Y",
	"Projector Z",
	"Fade",
	"Pan",
	"Mode",
	"Mask",
	"Nudge"
};

void GWindowGUI::calculate_extents(BC_WindowBase *gui, int *w, int *h)
{
	int temp1, temp2, temp3, temp4, temp5, temp6, temp7;
	int current_w, current_h;
	*w = 10;
	*h = 10;
	for(int i = 0; i < OTHER_TOGGLES; i++)
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
			other_text[i]);
		*w = MAX(current_w, *w);
		*h += current_h + 5;
	}

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
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
			auto_text[i]);
		*w = MAX(current_w, *w);
		*h += current_h + 5;
	}
	*h += 10;
	*w += 20;
}



void GWindowGUI::create_objects()
{
	int x = 10, y = 10;


	for(int i = 0; i < OTHER_TOGGLES; i++)
	{
		add_tool(other[i] = new GWindowToggle(mwindow, 
			this, 
			x, 
			y, 
			-1,
			i, 
			other_text[i]));
		y += other[i]->get_h() + 5;
	}

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		add_tool(auto_toggle[i] = new GWindowToggle(mwindow, 
			this, 
			x, 
			y, 
			i,
			-1, 
			auto_text[i]));
		y += auto_toggle[i]->get_h() + 5;
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

	for(int i = 0; i < OTHER_TOGGLES; i++)
	{
		other[i]->update();
	}

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		auto_toggle[i]->update();
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
	int subscript, 
	int other,
	char *text)
 : BC_CheckBox(x, 
 	y, 
	*get_main_value(mwindow, subscript, other), 
	text)
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->subscript = subscript;
	this->other = other;
}

int GWindowToggle::handle_event()
{
	*get_main_value(mwindow, subscript, other) = get_value();
	gui->update_mwindow();


// Update stuff in MWindow
	unlock_window();
	mwindow->gui->lock_window("GWindowToggle::handle_event");
	if(subscript >= 0)
	{
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
	}
	else
	{
		switch(other)
		{
			case ASSETS:
			case TITLES:
				mwindow->gui->update(1,
					1,
					0,
					0,
					1, 
					0,
					0);
				break;

			case TRANSITIONS:
			case PLUGIN_AUTOS:
				mwindow->gui->canvas->draw_overlays();
				mwindow->gui->canvas->flash();
				break;
		}
	}

	mwindow->gui->unlock_window();
	lock_window("GWindowToggle::handle_event");

	return 1;
}

int* GWindowToggle::get_main_value(MWindow *mwindow, int subscript, int other)
{
	if(subscript >= 0)
	{
		return &mwindow->edl->session->auto_conf->autos[subscript];
	}
	else
	{
		switch(other)
		{
			case ASSETS:
				return &mwindow->edl->session->show_assets;
				break;
			case TITLES:
				return &mwindow->edl->session->show_titles;
				break;
			case TRANSITIONS:
				return &mwindow->edl->session->auto_conf->transitions;
				break;
			case PLUGIN_AUTOS:
				return &mwindow->edl->session->auto_conf->plugins;
				break;
		}
	}
}

void GWindowToggle::update()
{
	set_value(*get_main_value(mwindow, subscript, other));
}



