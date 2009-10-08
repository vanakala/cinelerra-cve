
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

#include "labelnavigate.h"
#include "mbuttons.h"
#include "mwindow.h"
#include "theme.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

LabelNavigate::LabelNavigate(MWindow *mwindow, MButtons *gui, int x, int y)
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->x = x;
	this->y = y;
}

LabelNavigate::~LabelNavigate()
{
	delete prev_label;
	delete next_label;
}

void LabelNavigate::create_objects()
{
	gui->add_subwindow(prev_label = new PrevLabel(mwindow, 
		this, 
		x, 
		y));
	gui->add_subwindow(next_label = new NextLabel(mwindow, 
		this, 
		x + prev_label->get_w(), 
		y));
}


PrevLabel::PrevLabel(MWindow *mwindow, LabelNavigate *navigate, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("prevlabel"))
{ 
	this->mwindow = mwindow; 
	this->navigate = navigate;
	set_tooltip(_("Previous label"));
}

PrevLabel::~PrevLabel() {}

int PrevLabel::handle_event()
{
	mwindow->prev_label(shift_down());
	return 1;
}



NextLabel::NextLabel(MWindow *mwindow, LabelNavigate *navigate, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("nextlabel"))
{ 
	this->mwindow = mwindow; 
	this->navigate = navigate; 
	set_tooltip(_("Next label"));
}

NextLabel::~NextLabel() {}

int NextLabel::handle_event()
{
	mwindow->next_label(shift_down());
	return 1;
}


