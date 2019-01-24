
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
#include "filexml.h"
#include "keyframe.h"
#include "keyframes.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugindialog.h"
#include "pluginset.h"
#include "mainsession.h"
#include "track.h"
#include "trackcanvas.h"
#include "transition.h"
#include "transitionpopup.h"
#include <string.h>

Transition::Transition(EDL *edl, Edit *edit, const char *title, 
	ptstime length)
 : Plugin(edl, (PluginSet*)edit->edits, title)
{
	this->edit = edit;
	this->length_time = length;
	KeyFrame *new_auto = (KeyFrame *)keyframes->insert_auto(0);
}

KeyFrame* Transition::get_keyframe()
{
	return (KeyFrame*)keyframes->first;
}

Transition& Transition::operator=(Transition &that)
{
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
	return PTSEQU(this->length_time, that->length_time) && Plugin::identical(that);
}

void Transition::save_xml(FileXML *file)
{
	file->append_newline();
	file->tag.set_title("TRANSITION");
	file->tag.set_property("TITLE", title);
	file->tag.set_property("LENGTH_TIME", length_time);
	file->append_tag();
	if(on)
	{
		file->tag.set_title("ON");
		file->append_tag();
		file->tag.set_title("/ON");
		file->append_tag();
	}
	keyframes->save_xml(file);
	file->tag.set_title("/TRANSITION");
	file->append_tag();
	file->append_newline();
}

void Transition::load_xml(FileXML *file)
{
	posnum length;
	int result = 0;

	file->tag.get_property("TITLE", title);
	length = file->tag.get_property("LENGTH", length);
	if(length)
		length_time = track->from_units(length);
	length_time = file->tag.get_property("LENGTH_TIME", length_time);
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
				keyframes->first->load(file);
			}
		}
	}while(!result);
}

ptstime Transition::length(void)
{
	return length_time;
}

void Transition::dump(int indent)
{
	printf("%*sTransition %p: '%s' length: %.3f\n", indent, "", this,
		title, length_time);
	if(keyframes)
		keyframes->dump(indent + 2);
}
