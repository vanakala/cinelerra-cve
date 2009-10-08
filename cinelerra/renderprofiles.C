
/*
 * CINELERRA
 * Copyright (C) 2007 Andraz Tori
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
#include "renderprofiles.h"
#include "mwindow.h"
#include "theme.h"
#include "bchash.h"
#include "string.h"
#include "render.h"
#include "asset.h"
#include "errorbox.h"
#include "mwindowgui.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#define LISTWIDTH 200

RenderProfileItem::RenderProfileItem(char *text, int value)
 : BC_ListBoxItem(text)
{
	this->value = value;
}


RenderProfile::RenderProfile(MWindow *mwindow,
	RenderWindow *rwindow, 
	int x, 
	int y, 
	int use_nothing)
{
	this->mwindow = mwindow;
	this->rwindow = rwindow;
	this->x = x;
	this->y = y;
	this->use_nothing = use_nothing;
	for (int i = 1; i < MAX_PROFILES; i++)
	{
		char string_name[100];
		char name[100] = "";
		sprintf(string_name, "RENDER_%i_PROFILE_NAME", i);
		mwindow->defaults->get(string_name, name);
		if (strlen(name) != 0)
			profiles.append(new RenderProfileItem(name, i));
					
	}
}

RenderProfile::~RenderProfile()
{
//	delete title;
//	delete textbox;
//	delete listbox;
	for(int i = 0; i < profiles.total; i++)
		delete profiles.values[i];
}



int RenderProfile::calculate_h(BC_WindowBase *gui)
{
	return BC_TextBox::calculate_h(gui, MEDIUMFONT, 1, 1);
}

int RenderProfile::create_objects()
{
	int x = this->x, y = this->y;
	char *default_text = "";
	rwindow->add_subwindow(new BC_Title(x, 
		y, 
			_("RenderProfile:")));


	int old_y = y;
	rwindow->add_subwindow(title = new BC_Title(x, y, _("Render profile:")));
	y += 25;
	rwindow->add_subwindow(textbox = new BC_TextBox(x, 
		y, 
		LISTWIDTH, 
		1, 
		default_text));
	x += textbox->get_w();
	rwindow->add_subwindow(listbox = new RenderProfileListBox(rwindow, this, x, y));

	y = old_y;
	x += listbox->get_w() + 10;
	rwindow->add_subwindow(saveprofile = new SaveRenderProfileButton(this, 
		x, 
		y));
	y += 25;
	rwindow->add_subwindow(deleteprofile = new DeleteRenderProfileButton(this, 
		x, 
		y));



	return 0;
}

int RenderProfile::get_h()
{
	int result = 0;
	result = MAX(result, title->get_h());
	result = MAX(result, textbox->get_h());
	return result;
}

int RenderProfile::get_x()
{
	return x;
}

int RenderProfile::get_y()
{
	return y;
}

int RenderProfile::reposition_window(int x, int y)
{
	this->x = x;
	this->y = y;
	title->reposition_window(x, y);
	y += 20;
	textbox->reposition_window(x, y);
	x += textbox->get_w();
	listbox->reposition_window(x, 
		y, 
		LISTWIDTH);
	return 0;
}


RenderProfileListBox::RenderProfileListBox(BC_WindowBase *window, 
	RenderProfile *renderprofile, 
	int x, 
	int y)
 : BC_ListBox(x,
 	y,
	LISTWIDTH,
	150,
	LISTBOX_TEXT,
	(ArrayList<BC_ListBoxItem *>*)&renderprofile->profiles,
	0,
	0,
	1,
	0,
	1)
{
	this->window = window;
	this->renderprofile = renderprofile;
}

RenderProfileListBox::~RenderProfileListBox()
{
}

int RenderProfileListBox::handle_event()
{
	if(get_selection(0, 0) >= 0)
	{
		renderprofile->textbox->update(get_selection(0, 0)->get_text());
		renderprofile->rwindow->load_profile(((RenderProfileItem*)get_selection(0, 0))->value);
	}
	return 1;
}

int RenderProfile::get_profile_slot_by_name(char * profile_name)
{
	for (int i = 1; i < MAX_PROFILES; i++)
	{
		char string_name[100];
		char name[100] = "";
		sprintf(string_name, "RENDER_%i_PROFILE_NAME", i);
		
		mwindow->defaults->get(string_name, name);
		if (strcmp(name, profile_name) == 0)
			return i;
	}
// No free profile slots!
	return -1;
}

int RenderProfile::get_new_profile_slot()
{
	for (int i = 1; i < MAX_PROFILES; i++)
	{
		char string_name[100];
		char name[100] = "";
		sprintf(string_name, "RENDER_%i_PROFILE_NAME", i);
		mwindow->defaults->get(string_name, name);
		if (strlen(name) == 0)
			return i;
	}
	return -1;
}


int RenderProfile::save_to_slot(int profile_slot, char *profile_name)
{
	char string_name[100];
	sprintf(string_name, "RENDER_%i_PROFILE_NAME", profile_slot);
	mwindow->defaults->update(string_name, profile_name);

	sprintf(string_name, "RENDER_%i_STRATEGY", profile_slot);
	mwindow->defaults->update(string_name, rwindow->render->strategy);
	sprintf(string_name, "RENDER_%i_LOADMODE", profile_slot);
	mwindow->defaults->update(string_name, rwindow->render->load_mode);
	sprintf(string_name, "RENDER_%i_RANGE_TYPE", profile_slot);
	mwindow->defaults->update(string_name, rwindow->render->range_type);

	sprintf(string_name, "RENDER_%i_", profile_slot);
	rwindow->asset->save_defaults(mwindow->defaults, 
		string_name,
		1,
		1,
		1,
		1,
		1);

	mwindow->save_defaults();
	return 0;
}



SaveRenderProfileButton::SaveRenderProfileButton(RenderProfile *profile, int x, int y)
 : BC_GenericButton(x, y, _("Save profile"))
{
	this->profile = profile;
}
int SaveRenderProfileButton::handle_event()
{
	
	char *profile_name = profile->textbox->get_text();
	if (strlen(profile_name) == 0)     // Don't save when name not defined
		return 1;
	int slot = profile->get_profile_slot_by_name(profile_name);
	if (slot < 0)
	{
		slot = profile->get_new_profile_slot();
		if (slot < 0)
		{
			ErrorBox error_box(PROGRAM_NAME ": Error",
					   profile->mwindow->gui->get_abs_cursor_x(1),
					   profile->mwindow->gui->get_abs_cursor_y(1));
			error_box.create_objects("Maximum number of render profiles reached");
			error_box.raise_window();
			error_box.run_window();
			return 1;
		}
		
		profile->profiles.append(new RenderProfileItem(profile_name, slot));
		profile->listbox->update((ArrayList<BC_ListBoxItem *>*)&(profile->profiles), 0, 0, 1);
	
	}
	
	if (slot >= 0)
	{
		profile->save_to_slot(slot, profile_name);
	}
	return 1;
}


DeleteRenderProfileButton::DeleteRenderProfileButton(RenderProfile *profile, int x, int y)
 : BC_GenericButton(x, y, _("Delete profile"))
{
	this->profile = profile;
}
int DeleteRenderProfileButton::handle_event()
{
	char *profile_name = profile->textbox->get_text();
	int slot = profile->get_profile_slot_by_name(profile_name);
	if (slot >= 0)
	{
		for(int i = 0; i < profile->profiles.total; i++)
		{
			if(profile->profiles.values[i]->value == slot)
			{
				profile->profiles.remove_object_number(i);
				profile->save_to_slot(slot, "");

				break;
			}
		}
		profile->listbox->update((ArrayList<BC_ListBoxItem *>*)&(profile->profiles), 0, 0, 1);
		profile->textbox->update("");

	}


	return 1;
}





