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

Selection::Selection(int x, int y, BC_WindowBase *base, const char *texts[], int *value)
 : BC_TextBox(x, y, 90, 1, "")
{
	BC_PopupMenu *popupmenu;
	int x1 = x + get_w();
	int y1 = y + get_resources()->listbox_button[0]->get_h();
	intvalue = value;

	base->add_subwindow(popupmenu = new BC_PopupMenu(x, y1, 0, "", POPUPMENU_USE_COORDS));
	base->add_subwindow(new SelectionButton(x1, y, popupmenu,
		get_resources()->listbox_button));

	for(int i = 0; texts[i]; i++)
		popupmenu->add_item(new SelectionItem(texts[i], this));
}

int Selection::handle_event()
{
	*intvalue = atoi(get_text());
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


SelectionItem::SelectionItem(const char *text, BC_TextBox *output)
 : BC_MenuItem(text)
{
	this->output = output;
	this->text = text;
}

int SelectionItem::handle_event()
{
	output->update(text);
	output->handle_event();
	return 1;
}
