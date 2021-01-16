// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>

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
