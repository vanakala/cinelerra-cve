
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

#include "clip.h"
#include "loadmode.h"
#include "mwindow.h"
#include "theme.h"
#include "language.h"


LoadModeItem::LoadModeItem(char *text, int value)
 : BC_ListBoxItem(text)
{
	this->value = value;
}


LoadMode::LoadMode(MWindow *mwindow,
	BC_WindowBase *window, 
	int x, 
	int y, 
	int *output, 
	int use_nothing)
{
	this->mwindow = mwindow;
	this->window = window;
	this->x = x;
	this->y = y;
	this->output = output;
	this->use_nothing = use_nothing;
	if(use_nothing) load_modes.append(new LoadModeItem(_("Insert nothing"), LOADMODE_NOTHING));
	load_modes.append(new LoadModeItem(_("Replace current project"), LOADMODE_REPLACE));
	load_modes.append(new LoadModeItem(_("Replace current project and concatenate tracks"), LOADMODE_REPLACE_CONCATENATE));
	load_modes.append(new LoadModeItem(_("Append in new tracks"), LOADMODE_NEW_TRACKS));
	load_modes.append(new LoadModeItem(_("Concatenate to existing tracks"), LOADMODE_CONCATENATE));
	load_modes.append(new LoadModeItem(_("Paste at insertion point"), LOADMODE_PASTE));
	load_modes.append(new LoadModeItem(_("Create new resources only"), LOADMODE_RESOURCESONLY));

	window->add_subwindow(title = new BC_Title(x, y, _("Insertion strategy:")));
	y += 20;
	window->add_subwindow(textbox = new BC_TextBox(x,
		y,
		mwindow->theme->loadmode_w,
		1,
		mode_to_text()));
	x += textbox->get_w();
	window->add_subwindow(listbox = new LoadModeListBox(window, this, x, y));
}

LoadMode::~LoadMode()
{
	delete title;
	delete textbox;
	delete listbox;
	for(int i = 0; i < load_modes.total; i++)
		delete load_modes.values[i];
}

int LoadMode::calculate_h(BC_WindowBase *gui)
{
	return BC_TextBox::calculate_h(gui, MEDIUMFONT, 1, 1);
}

const char* LoadMode::mode_to_text()
{
	for(int i = 0; i < load_modes.total; i++)
	{
		if(load_modes.values[i]->value == *output) 
			return load_modes.values[i]->get_text();
	}
	return _("Unknown");
}

int LoadMode::get_h()
{
	int result = 0;
	result = MAX(result, title->get_h());
	result = MAX(result, textbox->get_h());
	return result;
}

int LoadMode::get_x()
{
	return x;
}

int LoadMode::get_y()
{
	return y;
}

void LoadMode::reposition_window(int x, int y)
{
	this->x = x;
	this->y = y;
	title->reposition_window(x, y);
	y += 20;
	textbox->reposition_window(x, y);
	x += textbox->get_w();
	listbox->reposition_window(x, 
		y, 
		mwindow->theme->loadmode_w);
}


LoadModeListBox::LoadModeListBox(BC_WindowBase *window, 
	LoadMode *loadmode, 
	int x, 
	int y)
 : BC_ListBox(x,
	y,
	loadmode->mwindow->theme->loadmode_w,
	150,
	(ArrayList<BC_ListBoxItem *>*)&loadmode->load_modes,
	LISTBOX_POPUP)
{
	this->window = window;
	this->loadmode = loadmode;
}

int LoadModeListBox::handle_event()
{
	if(get_selection(0, 0) >= 0)
	{
		loadmode->textbox->update(get_selection(0, 0)->get_text());
		*(loadmode->output) = ((LoadModeItem*)get_selection(0, 0))->value;
	}
	return 1;
}
