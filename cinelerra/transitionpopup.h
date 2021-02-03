// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TRANSITIONPOPUP_H
#define TRANSITIONPOPUP_H

#include "bcwindow.h"
#include "bcmenuitem.h"
#include "bctextbox.h"
#include "bcpopupmenu.h"
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
	TransitionLengthThread(TransitionPopup *popup);

	void run();
private:
	TransitionPopup *popup;
};


class TransitionLengthDialog : public BC_Window
{
public:
	TransitionLengthDialog(Plugin *transition,
		int absx, int absy);

	Plugin *transition;
private:
	TransitionLengthText *text;
};

class TransitionLengthText : public BC_TumbleTextBox
{
public:
	TransitionLengthText(TransitionLengthDialog *gui,
		int x,
		int y);

	int handle_event();
private:
	TransitionLengthDialog *gui;
};


class TransitionPopup : public BC_PopupMenu
{
public:
	TransitionPopup();
	~TransitionPopup();

	void update(Plugin *transition);

// Acquired through the update command as the plugin currently being operated on
	Plugin *transition;
	TransitionLengthThread *length_thread;
private:
// Needed for loading updates
	TransitionPopupOn *on;
	TransitionPopupShow *show;
	TransitionPopupDetach *detach;
	TransitionPopupLength *length;
	int has_gui;
};

class TransitionPopupDetach : public BC_MenuItem
{
public:
	TransitionPopupDetach(TransitionPopup *popup);

	int handle_event();
private:
	TransitionPopup *popup;
};

class TransitionPopupShow : public BC_MenuItem
{
public:
	TransitionPopupShow(TransitionPopup *popup);

	int handle_event();
private:
	TransitionPopup *popup;
};

class TransitionPopupOn : public BC_MenuItem
{
public:
	TransitionPopupOn(TransitionPopup *popup);

	int handle_event();
private:
	TransitionPopup *popup;
};

class TransitionPopupLength : public BC_MenuItem
{
public:
	TransitionPopupLength(TransitionPopup *popup);

	int handle_event();
private:
	TransitionPopup *popup;
};

#endif
