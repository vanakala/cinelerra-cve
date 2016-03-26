
/*
 * CINELERRA
 * Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#ifndef SUBSELECTION_H
#define SUBSELECTION_H

#include "bctextbox.h"
#include "bcmenuitem.h"
#include "paramlist.inc"

class SubSelection : public BC_TextBox
{
public:
	SubSelection(int x, int y, int h, BC_WindowBase *base, Param *param);
	~SubSelection();

private:
	BC_Button *button;
	BC_PopupMenu *popupmenu;
};

class SubSelectionItem : public BC_MenuItem
{
public:
	SubSelectionItem(Param *curitem, Param *parent, BC_PopupMenu *menu);

	int handle_event();

private:
	BC_PopupMenu *menu;
	Param *parent;
	Param *curitem;
};

class SubSelectionPopup : public BC_PopupTextBox
{
public:
	SubSelectionPopup(int x, int y, int w, BC_WindowBase *base, Paramlist *paramlist);
	~SubSelectionPopup();

	virtual int handle_event();

protected:
	Paramlist *list;
	ArrayList<BC_ListBoxItem*> *codecs;
};

#endif
