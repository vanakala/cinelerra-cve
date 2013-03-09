
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

#ifndef KEYFRAMEPOPUP_H
#define KEYFRAMEPOPUP_H

#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "plugin.inc"
#include "keyframe.inc"
#include "automation.h" 


class KeyframePopupDelete;
class KeyframePopupCopy;


class KeyframePopup : public BC_PopupMenu
{
public:
	KeyframePopup(MWindow *mwindow, MWindowGUI *gui);

	void create_objects();
	void update(Plugin *plugin, KeyFrame *keyframe);
	void update(Automation *automation, Autos *autos, Auto *auto_keyframe);

	MWindow *mwindow;
	MWindowGUI *gui;

// Acquired through the update command as the plugin currently being operated on
	Plugin *keyframe_plugin;
	Autos *keyframe_autos;
	Automation *keyframe_automation;
	Auto *keyframe_auto;

	KeyframePopupDelete *key_delete;
	KeyframePopupCopy *key_copy;
};

class KeyframePopupDelete : public BC_MenuItem
{
public:
	KeyframePopupDelete(MWindow *mwindow, KeyframePopup *popup);

	int handle_event();

	MWindow *mwindow;
	KeyframePopup *popup;
};

class KeyframePopupCopy : public BC_MenuItem
{
public:
	KeyframePopupCopy(MWindow *mwindow, KeyframePopup *popup);

	int handle_event();

	MWindow *mwindow;
	KeyframePopup *popup;
};

#endif
