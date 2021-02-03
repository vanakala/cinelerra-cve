// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PLUGINPOPUP_H
#define PLUGINPOPUP_H

class PluginPopupChange;
class PluginPopupDetach;
class PluginPopupOn;
class PluginPopupShow;
class PluginPopupPasteKeyFrame;

#include "bcmenuitem.h"
#include "bcpopupmenu.h"
#include "mwindow.inc"
#include "plugin.inc"
#include "plugindialog.inc"


class PluginPopup : public BC_PopupMenu
{
public:
	PluginPopup();
	~PluginPopup();

	void update(Plugin *plugin);

// Acquired through the update command as the plugin currently being operated on
	Plugin *plugin;
private:
	PluginPopupChange *change;
	PluginPopupDetach *detach;
	PluginPopupShow *show;
	PluginPopupOn *on;
	PluginPopupPasteKeyFrame *pastekeyframe;
	int have_show;
	int have_keyframe;
};


class PluginPopupChange : public BC_MenuItem
{
public:
	PluginPopupChange(PluginPopup *popup);
	~PluginPopupChange();

	int handle_event();
private:
	PluginPopup *popup;
	PluginDialogThread *dialog_thread;
};


class PluginPopupDetach : public BC_MenuItem
{
public:
	PluginPopupDetach(PluginPopup *popup);

	int handle_event();
private:
	PluginPopup *popup;
};


class PluginPopupShow : public BC_MenuItem
{
public:
	PluginPopupShow(PluginPopup *popup);

	int handle_event();
private:
	PluginPopup *popup;
};


class PluginPopupOn : public BC_MenuItem
{
public:
	PluginPopupOn(PluginPopup *popup);

	int handle_event();
private:
	PluginPopup *popup;
};


class PluginPopupUp : public BC_MenuItem
{
public:
	PluginPopupUp(PluginPopup *popup);

	int handle_event();
private:
	PluginPopup *popup;
};


class PluginPopupDown : public BC_MenuItem
{
public:
	PluginPopupDown(PluginPopup *popup);

	int handle_event();
private:
	PluginPopup *popup;
};

class PluginPopupPasteKeyFrame : public BC_MenuItem
{
public:
	PluginPopupPasteKeyFrame(PluginPopup *popup);

	int handle_event();
private:
	PluginPopup *popup;
};

class PluginPopupClearKeyFrames : public BC_MenuItem
{
public:
	PluginPopupClearKeyFrames(PluginPopup *popup);
	int handle_event();
private:
	PluginPopup *popup;
};
#endif
