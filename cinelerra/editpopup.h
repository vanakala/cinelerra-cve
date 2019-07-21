
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

#ifndef EDITPOPUP_H
#define EDITPOPUP_H

#include "bcmenuitem.h"
#include "bcpopupmenu.h"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "edit.inc"
#include "plugindialog.inc"
#include "resizetrackthread.inc"


class EditPopupResize;
class EditPopupMatchSize;
class EditPopupTitleText;
class EditPopupTitleWindow;
class EditPopupTitleButton;
class EditPopupTitleButtonRes;

class EditPopup : public BC_PopupMenu
{
public:
	EditPopup(MWindow *mwindow, MWindowGUI *gui);

	void update(Track *track, Edit *edit);

	MWindow *mwindow;
	MWindowGUI *gui;
// Acquired through the update command as the edit currently being operated on
	Edit *edit;
	Track *track;
	EditPopupResize *resize_option;
	EditPopupMatchSize *matchsize_option;
};

class EditPopupMatchSize : public BC_MenuItem
{
public:
	EditPopupMatchSize(MWindow *mwindow, EditPopup *popup);

	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupResize : public BC_MenuItem
{
public:
	EditPopupResize(EditPopup *popup);

	int handle_event();
private:
	EditPopup *popup;
	ResizeTrackThread *dialog_thread;
};

class EditPopupDeleteTrack : public BC_MenuItem
{
public:
	EditPopupDeleteTrack(MWindow *mwindow, EditPopup *popup);

	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupAddTrack : public BC_MenuItem
{
public:
	EditPopupAddTrack(MWindow *mwindow, EditPopup *popup);

	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};


class EditAttachEffect : public BC_MenuItem
{
public:
	EditAttachEffect(MWindow *mwindow, EditPopup *popup);
	~EditAttachEffect();

	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
	PluginDialogThread *dialog_thread;
};


class EditMoveTrackUp : public BC_MenuItem
{
public:
	EditMoveTrackUp(MWindow *mwindow, EditPopup *popup);

	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

class EditMoveTrackDown : public BC_MenuItem
{
public:
	EditMoveTrackDown(MWindow *mwindow, EditPopup *popup);

	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

#endif
