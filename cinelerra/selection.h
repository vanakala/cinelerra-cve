
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

#ifndef SELECTION_H
#define SELECTION_H

#include "selection.inc"
#include "bcbutton.h"
#include "bcmenuitem.h"
#include "bcpopupmenu.inc"
#include "bctextbox.h"
#include "bcwindow.inc"
#include "mwindow.inc"


class Selection : public BC_TextBox
{
public:
	Selection(int x, int y, BC_WindowBase *base, const char *texts[],
		int *value);

	int handle_event();
private:
	int *intvalue;
};

class SelectionButton : public BC_Button
{
public:
	SelectionButton(int x, int y, BC_PopupMenu *popupmenu, VFrame **images);

	int handle_event();
private:
	BC_PopupMenu *popupmenu;
};

class SelectionItem : public BC_MenuItem
{
public:
	SelectionItem(const char *text, BC_TextBox *output);

	int handle_event();
private:
	const char *text;
	BC_TextBox *output;
};

#endif
