
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
#include "editpopup.inc"
#include "plugindialog.inc"
#include "resizetrackthread.inc"
#include "track.inc"


class EditPopupResize;
class EditPopupMatchSize;
class EditPopupTitleText;
class EditPopupTitleWindow;
class EditPopupTitleButton;
class EditPopupTitleButtonRes;
class EditPopupPasteKeyFrame;

class EditPopup : public BC_PopupMenu
{
public:
	EditPopup();
	~EditPopup();

	void update(Track *track);

// Acquired through the update command as the edit currently being operated on
	Track *track;
private:
	EditPopupResize *resize_option;
	EditPopupMatchSize *matchsize_option;
	EditPopupPasteKeyFrame *pastekeyframe;
	int have_video;
	int have_keyframe;
};

class EditPopupMatchSize : public BC_MenuItem
{
public:
	EditPopupMatchSize(EditPopup *popup);

	int handle_event();
private:
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
	EditPopupDeleteTrack(EditPopup *popup);

	int handle_event();
private:
	EditPopup *popup;
};

class EditPopupAddTrack : public BC_MenuItem
{
public:
	EditPopupAddTrack(EditPopup *popup);

	int handle_event();
private:
	EditPopup *popup;
};


class EditAttachEffect : public BC_MenuItem
{
public:
	EditAttachEffect(EditPopup *popup);
	~EditAttachEffect();

	int handle_event();
private:
	EditPopup *popup;
	PluginDialogThread *dialog_thread;
};


class EditMoveTrackUp : public BC_MenuItem
{
public:
	EditMoveTrackUp(EditPopup *popup);

	int handle_event();
private:
	EditPopup *popup;
};

class EditMoveTrackDown : public BC_MenuItem
{
public:
	EditMoveTrackDown(EditPopup *popup);

	int handle_event();
private:
	EditPopup *popup;
};

class EditPopupPasteKeyFrame : public BC_MenuItem
{
public:
	EditPopupPasteKeyFrame(EditPopup *popup);

	int handle_event();

private:
	EditPopup *popup;
};
#endif
