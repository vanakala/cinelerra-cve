#ifndef EDITPOPUP_H
#define EDITPOPUP_H

#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "edit.inc"
#include "plugindialog.inc"
#include "resizetrackthread.inc"


class EditPopupResize;
class EditPopupMatchSize;

class EditPopup : public BC_PopupMenu
{
public:
	EditPopup(MWindow *mwindow, MWindowGUI *gui);
	~EditPopup();

	void create_objects();
	int update(Track *track, Edit *edit);

	MWindow *mwindow;
	MWindowGUI *gui;
// Acquired through the update command as the edit currently being operated on
//	Edit *edit;
	Track *track;
	EditPopupResize *resize_option;
	EditPopupMatchSize *matchsize_option;
};

class EditPopupMatchSize : public BC_MenuItem
{
public:
	EditPopupMatchSize(MWindow *mwindow, EditPopup *popup);
	~EditPopupMatchSize();
	int handle_event();
	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupResize : public BC_MenuItem
{
public:
	EditPopupResize(MWindow *mwindow, EditPopup *popup);
	~EditPopupResize();
	int handle_event();
	MWindow *mwindow;
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
	~EditMoveTrackUp();

	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

class EditMoveTrackDown : public BC_MenuItem
{
public:
	EditMoveTrackDown(MWindow *mwindow, EditPopup *popup);
	~EditMoveTrackDown();

	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

#endif
