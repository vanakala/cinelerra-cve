// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "bcresources.h"
#include "bclistboxitem.h"
#include "bcpopupmenu.h"
#include "bctextbox.h"
#include "bcsignals.h"
#include "language.h"
#include "paramlist.h"
#include "selection.h"
#include "subselection.h"
#include "vframe.h"

#define TEXTBOX_LIST_H 350

SubSelection::SubSelection(int x, int y, int w, BC_WindowBase *base, Param *param)
 : BC_TextBox(x, y, w, 1, param->prompt ? _(param->prompt) : param->name)
{
	Param *current;

	int x1 = x + w;
	int y1 = y + get_resources()->listbox_button[0]->get_h();

	base->add_subwindow(popupmenu = new BC_PopupMenu(x, y1, 0,
		"", POPUPMENU_USE_COORDS));
	base->add_subwindow(button = new SelectionButton(x1, y, popupmenu,
		get_resources()->listbox_button));

	if(param->helptext)
		button->set_tooltip(param->helptext);

	disable(1);

	if(param->subparams)
	{
		for(current = param->subparams->first; current; current = current->next)
			popupmenu->add_item(new SubSelectionItem(current, param, popupmenu));
	}
}

SubSelection::~SubSelection()
{
	if(!get_deleting())
	{
		delete popupmenu;
		delete button;
	}
}


SubSelectionItem::SubSelectionItem(Param *curitem, Param *parent, BC_PopupMenu *menu)
 : BC_MenuItem(curitem->name)
{
	this->parent = parent;
	this->curitem = curitem;
	this->menu = menu;

	switch(parent->type & PARAMTYPE_MASK)
	{
	case PARAMTYPE_LNG:
		if(parent->type & PARAMTYPE_BITS)
		{
			if(parent->longvalue & curitem->longvalue)
				set_checked(1);
		}
		else
		{
			if(parent->longvalue == curitem->longvalue)
				set_checked(1);
		}
		break;
	}
}

int SubSelectionItem::handle_event()
{
	int k;
	int newval = !get_checked();

	switch(parent->type & PARAMTYPE_MASK)
	{
	case PARAMTYPE_LNG:
		if(parent->type & PARAMTYPE_BITS)
		{
			if(newval)
				parent->longvalue |= curitem->longvalue;
			else
				parent->longvalue &= ~curitem->longvalue;
			set_checked(newval);
		}
		else
		{
			if(newval)
			{
				parent->longvalue = curitem->longvalue;

				k = menu->total_items();

				for(int i = 0; i < k; i++)
					menu->get_item(i)->set_checked(0);
				set_checked(1);
			}
		}
	}
	return 1;
}

SubSelectionPopup::SubSelectionPopup(int x, int y, int w, BC_WindowBase *base, Paramlist *paramlist)
 : BC_PopupTextBox(base, 0, 0, x, y, w, TEXTBOX_LIST_H)
{
	Param *current;
	codecs = new ArrayList<BC_ListBoxItem*>;

	list = paramlist;
	disable_text(1);

	for(current = paramlist->first; current; current = current->next)
	{
		codecs->append(new BC_ListBoxItem(current->name));
		if(paramlist->type & current->type & PARAMTYPE_INT &&
				paramlist->selectedint == current->intvalue)
			update(current->name);
		if(paramlist->type & current->type & PARAMTYPE_LNG &&
				paramlist->selectedlong == current->longvalue)
			update(current->name);
		if(paramlist->type & current->type & PARAMTYPE_DBL &&
				PTSEQU(paramlist->selectedfloat, current->floatvalue))
			update(current->name);
	}
	update_list(codecs);
}

SubSelectionPopup::~SubSelectionPopup()
{
	codecs->remove_all_objects();
	delete codecs;
}

int SubSelectionPopup::handle_event()
{
	int num = get_number();
	int i = 0;
	Param *current;

	for(current = list->first; current; current = current->next)
	{
		if(i == num)
		{
			if(list->type & PARAMTYPE_INT)
				list->set_selected(current->intvalue);
			if(list->type & PARAMTYPE_LNG)
				list->set_selected(current->longvalue);
			if(list->type & PARAMTYPE_DBL)
				list->set_selected(current->floatvalue);
			list->type |= PARAMTYPE_CHNG;
			break;
		}
		i++;
	}
	return 1;
}
