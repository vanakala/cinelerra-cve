
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

#ifndef TRANSITIONPOPUP_H
#define TRANSITIONPOPUP_H

#include "bcwindow.h"
#include "bcmenuitem.h"
#include "bctextbox.h"
#include "bcpopupmenu.h"
#include "mwindowgui.inc"
#include "plugin.inc"
#include "thread.h"
#include "transitionpopup.inc"

class TransitionPopupOn;
class TransitionPopupShow;
class TransitionPopupDetach;
class TransitionPopupLength;
class TransitionLengthText;

class TransitionLengthThread : public Thread
{
public:
	TransitionLengthThread(MWindow *mwindow, TransitionPopup *popup);

	void run();

	MWindow *mwindow;
	TransitionPopup *popup;
};


class TransitionLengthDialog : public BC_Window
{
public:
	TransitionLengthDialog(MWindow *mwindow, Plugin *transition,
		int absx, int absy);

	MWindow *mwindow;
	Plugin *transition;
	TransitionLengthText *text;
};

class TransitionLengthText : public BC_TumbleTextBox
{
public:
	TransitionLengthText(MWindow *mwindow, 
		TransitionLengthDialog *gui,
		int x,
		int y);
	int handle_event();
	MWindow *mwindow;
	TransitionLengthDialog *gui;
};


class TransitionPopup : public BC_PopupMenu
{
public:
	TransitionPopup(MWindow *mwindow, MWindowGUI *gui);
	~TransitionPopup();

	void update(Plugin *transition);

// Acquired through the update command as the plugin currently being operated on
	Plugin *transition;

// Set when the user clicks a transition.
	MWindow *mwindow;
	MWindowGUI *gui;

// Needed for loading updates
	TransitionPopupOn *on;
	TransitionPopupShow *show;
	TransitionPopupDetach *detach;
	TransitionPopupLength *length;
	TransitionLengthThread *length_thread;
};

class TransitionPopupDetach : public BC_MenuItem
{
public:
	TransitionPopupDetach(MWindow *mwindow, TransitionPopup *popup);

	int handle_event();
	MWindow *mwindow;
	TransitionPopup *popup;
};

class TransitionPopupShow : public BC_MenuItem
{
public:
	TransitionPopupShow(MWindow *mwindow, TransitionPopup *popup);

	int handle_event();
	MWindow *mwindow;
	TransitionPopup *popup;
};

class TransitionPopupOn : public BC_MenuItem
{
public:
	TransitionPopupOn(MWindow *mwindow, TransitionPopup *popup);

	int handle_event();
	MWindow *mwindow;
	TransitionPopup *popup;
};

class TransitionPopupLength : public BC_MenuItem
{
public:
	TransitionPopupLength(MWindow *mwindow, TransitionPopup *popup);

	int handle_event();
	MWindow *mwindow;
	TransitionPopup *popup;
};

#endif
