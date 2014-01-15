/*
 * CINELERRA
 * Copyright (C) 2014 Einar Rünkaru <einarry at smail dot ee>
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

#include "selection.h"
#include "bcpopupmenu.h"
#include "bcsignals.h"
#include "bctextbox.h"
#include "bcbutton.h"
#include "bcmenuitem.h"
#include "bcwindow.h"
#include "bcwindowbase.h"
#include "bcresources.h"
#include "mwindow.h"
#include "theme.h"

#include <stdlib.h>

const struct selection_int SampleRateSelection::sample_rates[] =
{
	{ "8000", 8000 },
	{ "16000", 16000 },
	{ "22050", 22050 },
	{ "32000", 32000 },
	{ "44100", 44100 },
	{ "48000", 48000 },
	{ "96000", 96000 },
	{ "192000", 192000 },
	{ 0, 0 }
};

const struct selection_double FrameRateSelection::frame_rates[] =
{
	{ "1", 1 },
	{ "5", 5 },
	{ "10", 10 },
	{ "12", 12 },
	{ "15", 15 },
	{ "23.97", 24 / 1.001 },
	{ "24", 24 },
	{ "25", 25 },
	{ "29.97", 30 / 1.001 },
	{ "30", 30 },
	{ "50", 50 },
	{ "59.94", 60 / 1.001 },
	{ "60", 60 },
	{ 0, 0 }
};

SampleRateSelection::SampleRateSelection(int x, int y, BC_WindowBase *base, int *value)
 : Selection(x, y , base, sample_rates, value)
{
}

FrameRateSelection::FrameRateSelection(int x, int y, BC_WindowBase *base, double *value)
 : Selection(x, y , base, frame_rates, value)
{
}

Selection::Selection(int x, int y, BC_WindowBase *base,
	const struct selection_int items[], int *value)
 : BC_TextBox(x, y, 90, 1, "")
{
	BC_PopupMenu *popupmenu;

	popupmenu = init_objects(x, y, base);
	intvalue = value;

	for(int i = 0; items[i].text; i++)
		popupmenu->add_item(new SelectionItem(&items[i], this));
}

Selection::Selection(int x, int y, BC_WindowBase *base,
	const struct selection_double items[], double *value)
 : BC_TextBox(x, y, 90, 1, "")
{
	BC_PopupMenu *popupmenu;

	popupmenu = init_objects(x, y, base);

	doublevalue = value;
	set_precision(2);

	for(int i = 0; items[i].text; i++)
		popupmenu->add_item(new SelectionItem(&items[i], this));
}

BC_PopupMenu *Selection::init_objects(int x, int y, BC_WindowBase *base)
{
	BC_PopupMenu *popupmenu;
	int x1 = x + get_w();
	int y1 = y + get_resources()->listbox_button[0]->get_h();

	base->add_subwindow(popupmenu = new BC_PopupMenu(x, y1, 0, "", POPUPMENU_USE_COORDS));
	base->add_subwindow(new SelectionButton(x1, y, popupmenu,
		get_resources()->listbox_button));

	intvalue = 0;
	doublevalue = 0;
	current_double = 0;
	return popupmenu;
}

int Selection::handle_event()
{
	if(intvalue)
		*intvalue = atoi(get_text());
	if(doublevalue)
	{
		if(current_double)
		{
			*doublevalue = current_double->value;
			current_double = 0;
		}
		else
			*doublevalue = atof(get_text());
	}
	return 1;
}


SelectionButton::SelectionButton(int x, int y, BC_PopupMenu *popupmenu, VFrame **images)
 : BC_Button(x, y, images)
{
	this->popupmenu = popupmenu;
}

int SelectionButton::handle_event()
{
	popupmenu->activate_menu();
	// simulate release - cursor is outside the popup and
	// popup does not get the first release event
	popupmenu->button_release_event();
	return 1;
}


SelectionItem::SelectionItem(const struct selection_int *item, Selection *output)
 : BC_MenuItem(item->text)
{
	this->output = output;
	intitem = item;
	doubleitem = 0;
}

SelectionItem::SelectionItem(const struct selection_double *item, Selection *output)
 : BC_MenuItem(item->text)
{
	this->output = output;
	intitem = 0;
	doubleitem = item;
}

int SelectionItem::handle_event()
{
	if(intitem)
		output->update(intitem->text);
	if(doubleitem)
	{
		output->update(doubleitem->text);
		output->current_double = doubleitem;
	}
	output->handle_event();
	return 1;
}
