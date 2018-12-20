
/*
 * CINELERRA
 * Copyright (C) 2004 Andraz Tori
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

#include "awindow.h"
#include "awindowgui.h"
#include "bctitle.h"
#include "cinelerra.h"
#include "manualgoto.h"
#include "edl.h"
#include "fonts.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "vwindow.h"
#include "vwindowgui.h"
#include "keys.h"
#include "maincursor.h"
#include "cwindow.h"
#include "mwindowgui.h"
#include "edlsession.h"
#include "tracks.h"
#include "units.h"
#include "theme.h"

#include <ctype.h>

ManualGoto::ManualGoto(MWindow *mwindow, BC_WindowBase *masterwindow)
 : Thread()
{
	this->mwindow = mwindow;
	this->masterwindow = masterwindow;
	window = 0;
}

void ManualGoto::open_window()
{
	if ((masterwindow == (BC_WindowBase *)mwindow->cwindow->gui)||
		(masterwindow == (BC_WindowBase *)mwindow->gui->mbuttons))
	{
		position = master_edl->local_session->get_selectionstart(1) +
			edlsession->get_frame_offset();
		icon_image = mwindow->theme->get_image("mwindow_icon");
	}
	else
	if (mwindow->vwindow->get_edl())
	{
		position = mwindow->vwindow->get_edl()->local_session->get_selectionstart(1);
		icon_image = mwindow->theme->get_image("vwindow_icon");
	}
	else
		return;

	if(!running())
		start();
	else if(window)
		window->raise_window();
}

void ManualGoto::run()
{
	int result;

	window = new ManualGotoWindow(mwindow, this);
	result = window->run_window();

	if (result == 0) // ok button or return pressed
	{
		ptstime new_position = window->get_entered_position_sec();
		char modifier = window->signtitle->get_text()[0];
		if ((masterwindow == (BC_WindowBase *)mwindow->cwindow->gui)||
			(masterwindow == (BC_WindowBase *)mwindow->gui->mbuttons))
		{
			// mwindow/cwindow update

			ptstime current_position = master_edl->local_session->get_selectionstart(1);
			switch (modifier)
			{
			case '+':
				new_position += current_position;
				break;
			case '-':
				new_position = current_position - new_position;
				break;
			default:
				break;
			}
			new_position = master_edl->align_to_frame(new_position) -
				edlsession->get_frame_offset();
			if (new_position < 0) 
				new_position = 0;
			if (!PTSEQU(current_position, new_position))
			{
				master_edl->local_session->set_selection(new_position);
				mwindow->find_cursor();
				mwindow->gui->update(WUPD_SCROLLBARS |
				WUPD_CANVINCR | WUPD_TIMEBAR | WUPD_ZOOMBAR |
				WUPD_PATCHBAY | WUPD_CLOCK);
				mwindow->cwindow->update(WUPD_POSITION);
			}
		} else
		if ((masterwindow == (BC_WindowBase *)mwindow->vwindow->gui) &&
			mwindow->vwindow->get_edl())
		{
			// vwindow update
			VWindow *vwindow = mwindow->vwindow;
			ptstime current_position = vwindow->get_edl()->local_session->get_selectionstart(1);
			switch (modifier)
			{
			case '+':
				new_position += current_position;
				break;
			case '-':
				new_position = current_position - new_position;
				break;
			default:
				break;
			}
			if (new_position > vwindow->get_edl()->tracks->total_length())
				new_position = vwindow->get_edl()->tracks->total_length();
			if (new_position < 0)
				new_position = 0;
			new_position = vwindow->get_edl()->align_to_frame(new_position);
			if (!PTSEQU(current_position, new_position))
			{
				vwindow->get_edl()->local_session->set_selection(new_position);
				vwindow->update_position(CHANGE_NONE, 0, 1);
			}
		}
	}
	delete window;
	window = 0;
}


ManualGotoWindow::ManualGotoWindow(MWindow *mwindow, ManualGoto *thread)
 : BC_Window(MWindow::create_title(N_("Goto position")),
	mwindow->gui->get_abs_cursor_x(1) - 250 / 2,
	mwindow->gui->get_abs_cursor_y(1) - 80 / 2,
	250, 
	80,
	250,
	80,
	0,
	0,
	1)
{
	int x = 76, y = 5;
	int x1 = x;
	const char *htxt;
	int i, j, l, max;

	this->mwindow = mwindow;
	this->thread = thread;
	numboxes = 0;

	timeformat = edlsession->time_format;

	switch(timeformat)
	{
	case TIME_HMS:
	case TIME_HMS2:
	case TIME_HMS3:
		htxt = _("hour  min   sec     msec");
		break;
	case TIME_HMSF:
		htxt = _("hour  min   sec  frames");
		break;
	case TIME_SECONDS:
		htxt = _("  sec         msec");
		break;
	case TIME_SAMPLES:
	case TIME_SAMPLES_HEX:
		htxt = _("   Audio samples");
		timeformat = TIME_SAMPLES;
		break;
	case TIME_FRAMES:
		htxt = _("   Frames");
		break;
	case TIME_FEET_FRAMES:
		htxt = _("   Feet frames");
		break;
	}
	edlsession->ptstotext(timestring, (ptstime)0);
	numboxes = split_timestr(timestring);

	set_icon(thread->icon_image);

	BC_Title *title;
	add_subwindow(title = new BC_Title(x1 - 2, y, htxt, SMALLFONT));
	y += title->get_h() + 3;
	add_subwindow(signtitle = new BC_Title(x1 - 17, y, "=", LARGEFONT));
	for(i = 0; i < numboxes; i++)
	{
		l = strlen(timeparts[i]);
		add_subwindow(boxes[i] = new ManualGotoNumber(this, 
			x1, y, 9 * l + 6, l));
		x1 += boxes[i]->get_w() + 4;
	}

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));

	set_entered_position_sec(thread->position);
	activate();
	show_window();
}

ptstime ManualGotoWindow::get_entered_position_sec()
{
	int i;
	char *p = timestring;

	for(i = 0; i < numboxes; i++){
		strcpy(p, boxes[i]->get_text());
		while(*p) p++;
		*p++ = '.';
	}
	*--p = 0;
	return Units::text_to_seconds(timestring,
		edlsession->sample_rate,
		timeformat,
		edlsession->frame_rate,
		edlsession->frames_per_foot);
}

int ManualGotoWindow::split_timestr(char *timestr)
{
	char *p;
	int i, n;
	p = timestr;

	timeparts[0] = 0;
	for (n = 0; n < 4; n++)
	{
		if(*p == 0)
			break;
		timeparts[n] = p;
		while(isdigit(*p)) p++;
		if(*p)
			*p++ = 0;
	}
	timeparts[n] = 0;
	return n;
}

void ManualGotoWindow::set_entered_position_sec(ptstime position)
{
	int i;

	position = fabs(position);
	Units::totext(timestring,
		position,
		timeformat,
		edlsession->sample_rate,
		edlsession->frame_rate,
		edlsession->frames_per_foot);
	split_timestr(timestring);

	for(i = 0; i < numboxes; i++)
		boxes[i]->reshape_update(timeparts[i]);
}

void ManualGotoWindow::activate()
{
	int i;

	for(i = 0; i < numboxes; i++)
		boxes[i]->reshape_update(0);
	BC_Window::activate();
	for(i = 1; i < numboxes; i++)
		boxes[i]->deactivate();
	boxes[0]->activate();
}


ManualGotoNumber::ManualGotoNumber(ManualGotoWindow *window, int x, int y, int w, int chars)
 : BC_TextBox(x, y, w, 1, "0")
{
	this->window = window;
	this->chars = chars;
}

int ManualGotoNumber::handle_event()
{
	return 1;
}

void ManualGotoNumber::deactivate()
{
	reshape_update(0);
	BC_TextBox::deactivate();
}

void ManualGotoNumber::activate()
{
	BC_TextBox::activate();
	select_whole_text(1);
}

void ManualGotoNumber::reshape_update(char *numstr)
{
	char format_text[10];
	char text[BCTEXTLEN];
	if (numstr == 0)
		numstr = get_text();
	update(numstr);
	select_whole_text(-1);
}

int ManualGotoNumber::keypress_event()
{
	int in_textlen = strlen(get_text());
	int key = get_keypress();

	if (key == '+') 
	{
		window->signtitle->update("+");
		return 1;
	}
	if (key == '-')
	{
		window->signtitle->update("-");
		return 1;
	}
	if (key == '=') 
	{
		window->signtitle->update("=");
		return 1;
	}
	
	int ok_key = 0;
	if ((key >= '0' && key <='9') ||
		(key == ESC) ||  (key == RETURN) ||
		(key == TAB) ||  (key == LEFTTAB) ||
		(key == '.') ||
		(key == LEFT) || (key == RIGHT) ||
		(key == UP) ||   (key == DOWN) ||
		(key == PGUP) || (key == PGDN) ||
		(key == END) ||  (key == HOME) ||
		(key == BACKSPACE) || (key == DELETE) ||
		(ctrl_down() && (key == 'v' || key == 'V' || key == 'c' || key == 'C' || key == 'x' || key == 'X')))
		ok_key = 1;

	if (in_textlen >= chars && key >= '0' && key <= '9' && !select_whole_text(0))
		ok_key = 0;

	if (!ok_key) return 1;

// treat dot as tab - for use from numerical keypad
	if (key == '.') 
	{ 
		cycle_textboxes(1); 
		return 1; 
	};

	int result = BC_TextBox::keypress_event();
	int out_textlen = strlen(get_text());

// automatic cycle when we enter two numbers
	if (key != TAB && out_textlen == chars && get_ibeam_letter() == chars) 
		cycle_textboxes(1);
	return result;
}
