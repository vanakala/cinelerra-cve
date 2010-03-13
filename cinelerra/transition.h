
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

#ifndef TRANSITION_H
#define TRANSITION_H

class PasteTransition;

#include "edit.inc"
#include "filexml.inc"
#include "mwindow.inc"
#include "messages.inc"
#include "plugin.h"
#include "sharedlocation.h"

class TransitionMenuItem : public BC_MenuItem
{
public:
	TransitionMenuItem(MWindow *mwindow, int audio, int video);
	~TransitionMenuItem();
	int handle_event();
	int audio;
	int video;
//	PasteTransition *thread;
};

class PasteTransition : public Thread
{
public:
	PasteTransition(MWindow *mwindow, int audio, int video);
	~PasteTransition();

	void run();

	MWindow *mwindow;
	int audio, video;
};


class Transition : public Plugin
{
public:
	Transition(EDL *edl, Edit *edit, const char *title, long unit_length);

	Edit *edit;



	void save_xml(FileXML *file);
	void load_xml(FileXML *file);




	Transition(Transition *that, Edit *edit);
	~Transition();

	KeyFrame* get_keyframe();
	int reset_parameters();
	int update_derived();
	Transition& operator=(Transition &that);
	Plugin& operator=(Plugin &that);
	Edit& operator=(Edit &that);
	int operator==(Transition &that);
	int operator==(Plugin &that);
	int operator==(Edit &that);
	int identical(Transition *that);

// Only the show value from the attachment point is used.
	int set_show_derived(int value) {};

	int popup_transition(int x, int y);
// Update the widgets after loading
	int update_display();
// Update edit after attaching
	int update_edit(int is_loading);
	const char* default_title();
	void dump();

private:
// Only used by operator= and copy constructor
	void copy_from(Transition *that);
};

#endif
