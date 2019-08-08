
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
