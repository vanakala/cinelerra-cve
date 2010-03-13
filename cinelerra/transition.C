
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

#include "edit.h"
#include "edits.h"
#include "errorbox.h"
#include "filexml.h"
#include "keyframes.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patch.h"
#include "plugindialog.h"
#include "pluginset.h"
#include "mainsession.h"
#include "track.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transition.h"
#include "transitionpopup.h"
#include <string.h>


TransitionMenuItem::TransitionMenuItem(MWindow *mwindow, int audio, int video)
 : BC_MenuItem("Paste Transition")
{
	this->audio = audio;
	this->video = video;
}

TransitionMenuItem::~TransitionMenuItem()
{
}

int TransitionMenuItem::handle_event()
{
}


PasteTransition::PasteTransition(MWindow *mwindow, int audio, int video)
 : Thread()
{
	this->mwindow = mwindow;
	this->audio = audio;
	this->video = video;
}

PasteTransition::~PasteTransition()
{
}

void PasteTransition::run()
{
}













Transition::Transition(EDL *edl, Edit *edit, const char *title, long unit_length)
 : Plugin(edl, (PluginSet*)edit->edits, title)
{
	this->edit = edit;
	this->length = unit_length;
//printf("Transition::Transition %p %p %p %p\n", this, keyframes, keyframes->first, keyframes->last);
}

Transition::~Transition()
{
}

KeyFrame* Transition::get_keyframe()
{
	return (KeyFrame*)keyframes->default_auto;
}

Transition& Transition::operator=(Transition &that)
{
//printf("Transition::operator= 1\n");
	copy_from(&that);
	return *this;
}

Plugin& Transition::operator=(Plugin &that)
{
	copy_from((Transition*)&that);
	return *this;
}

Edit& Transition::operator=(Edit &that)
{
	copy_from((Transition*)&that);
	return *this;
}


int Transition::operator==(Transition &that)
{
	return identical(&that);
}

int Transition::operator==(Plugin &that)
{
	return identical((Transition*)&that);
}

int Transition::operator==(Edit &that)
{
	return identical((Transition*)&that);
}


void Transition::copy_from(Transition *that)
{
	Plugin::copy_from(that);
}

int Transition::identical(Transition *that)
{
	return this->length == that->length && Plugin::identical(that);
}


int Transition::reset_parameters()
{
	return 0;
}

void Transition::save_xml(FileXML *file)
{
	file->append_newline();
	file->tag.set_title("TRANSITION");
	file->tag.set_property("TITLE", title);
	file->tag.set_property("LENGTH", length);
	file->append_tag();
	if(on)
	{
		file->tag.set_title("ON");
		file->append_tag();
		file->tag.set_title("/ON");
		file->append_tag();
	}
	keyframes->copy(0, 0, file, 1, 0);
	file->tag.set_title("/TRANSITION");
	file->append_tag();
	file->append_newline();
}

void Transition::load_xml(FileXML *file)
{
	int result = 0;
	file->tag.get_property("TITLE", title);
	length = file->tag.get_property("LENGTH", length);
	on = 0;
	
	do{
		result = file->read_tag();
		if(!result)
		{
			if(file->tag.title_is("/TRANSITION"))
			{
				result = 1;
			}
			else
			if(file->tag.title_is("ON"))
			{
				on = 1;
			}
			else
			if(file->tag.title_is("KEYFRAME"))
			{
				keyframes->default_auto->load(file);;
			}
		}
	}while(!result);
}



int Transition::popup_transition(int x, int y)
{
// 	if(mwindow->session->tracks_vertical)
// 		mwindow->gui->transition_popup->activate_menu(this, PROGRAM_NAME ": Transition", y, x);
// 	else
// 		mwindow->gui->transition_popup->activate_menu(this, PROGRAM_NAME ": Transition", x, y);
}

int Transition::update_derived()
{
// Redraw transition titles
}

int Transition::update_display()
{
// Don't draw anything during loads.
	return 0;
}

const char* Transition::default_title()
{
	return "Transition";
}

void Transition::dump()
{
	printf("       title: %s length: %d\n", title, length);
}


