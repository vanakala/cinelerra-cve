// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2004 Andraz Tori

#include "bcsignals.h"
#include "bctitle.h"
#include "cinelerra.h"
#include "manualgoto.h"
#include "edl.h"
#include "fonts.h"
#include "language.h"
#include "localsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "vwindow.h"
#include "vwindowgui.h"
#include "keys.h"
#include "cwindow.h"
#include "edlsession.h"
#include "units.h"

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
		icon_image = mwindow->get_window_icon();
	}
	else
	{
		position = vwindow_edl->local_session->get_selectionstart(1);
		icon_image = mwindow->vwindow->get_window_icon();
	}

	if(!running())
		start();
	else if(window)
		window->raise_window();
}

void ManualGoto::run()
{
	int result;
	int cx, cy;

	mwindow->get_abs_cursor_pos(&cx, &cy);
	window = new ManualGotoWindow(mwindow, this, cx, cy);
	result = window->run_window();

	if(result == 0) // ok button or return pressed
	{
		ptstime new_position = window->get_entered_position_sec();
		char modifier = window->signtitle->get_text()[0];

		if((masterwindow == (BC_WindowBase*)mwindow->cwindow->gui)||
			(masterwindow == (BC_WindowBase*)mwindow->gui->mbuttons))
		{
			// mwindow/cwindow update

			ptstime current_position = master_edl->local_session->get_selectionstart(1);
			switch(modifier)
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
			if(new_position < 0)
				new_position = 0;
			if(!PTSEQU(current_position, new_position))
			{
				master_edl->local_session->set_selection(new_position);
				mwindow->find_cursor();
				mwindow->update_gui(WUPD_SCROLLBARS |
					WUPD_CANVINCR | WUPD_TIMEBAR | WUPD_ZOOMBAR |
					WUPD_PATCHBAY | WUPD_CLOCK);
				mwindow->cwindow->update(WUPD_POSITION);
			}
		}
		else if(masterwindow == (BC_WindowBase*)mwindow->vwindow->gui)
		{
			// vwindow update
			VWindow *vwindow = mwindow->vwindow;
			ptstime current_position = vwindow_edl->local_session->get_selectionstart(1);

			switch(modifier)
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
			if(new_position > vwindow_edl->total_length())
				new_position = vwindow_edl->total_length();
			if(new_position < 0)
				new_position = 0;
			new_position = vwindow_edl->align_to_frame(new_position);

			if(!PTSEQU(current_position, new_position))
			{
				vwindow_edl->local_session->set_selection(new_position);
				vwindow->update_position(0, 1);
			}
		}
	}
	delete window;
	window = 0;
}


ManualGotoWindow::ManualGotoWindow(MWindow *mwindow, ManualGoto *thread, int absx, int absy)
 : BC_Window(MWindow::create_title(N_("Goto position")),
	absx - 250 / 2,
	absy - 80 / 2,
	350,
	120,
	350,
	120,
	0,
	0,
	1)
{
#define TOP_MARGIN 8
#define INTERVAL 8
	int xpos, y, margin, signw;
	int boxrel, hdrw;
	char *timehdrs[NUM_TIMEPARTS];
	int boxw[NUM_TIMEPARTS];
	int boxpos[NUM_TIMEPARTS];
	int hdrpos[NUM_TIMEPARTS];

	this->mwindow = mwindow;
	this->thread = thread;
	numboxes = 0;

	for(int i = 0; i < NUM_TIMEPARTS; i++)
		timehdrs[i] = 0;

	timeformat = edlsession->time_format;

	timehdrs[0] = _("hour");
	timehdrs[1] = _("min");
	timehdrs[2] = _("sec");

	switch(timeformat)
	{
	case TIME_HMS:
	case TIME_HMS2:
	case TIME_HMS3:
		timehdrs[3] = _("msec");
		break;
	case TIME_HMSF:
		timehdrs[3] = _("frames");
		break;
	case TIME_SECONDS:  // sec msec
		timehdrs[0] = _("sec");
		timehdrs[1] = _("msec");
		break;
	case TIME_SAMPLES:
		timehdrs[0] = _("Audio samples");
		break;
	case TIME_FRAMES:
		timehdrs[0] = _("Frames");
		break;
	case TIME_FEET_FRAMES:
		timehdrs[0] = _("Feet");
		timehdrs[1] = _("frames");
		break;
	}
	xpos = 0;
	edlsession->ptstotext(timestring, (ptstime)0);
	numboxes = split_timestr(timestring);
	for(int i = 0; i < numboxes; i++)
	{
		boxw[i] = get_text_width(MEDIUMFONT, timeparts[i]) + 12;
		hdrw = get_text_width(SMALLFONT, timehdrs[i]);
		boxrel = (hdrw - boxw[i]) / 2;

		if(boxrel > 0)
		{
			hdrpos[i] = xpos;
			boxpos[i] = xpos + boxrel;
			xpos += hdrw;
		}
		else
		{
			hdrpos[i] = xpos - boxrel;
			boxpos[i] = xpos;
			xpos += boxw[i];
		}
		xpos += INTERVAL;
	}
	signw = get_text_width(LARGEFONT, "=");
	margin = (get_w() - xpos - signw) / 2;
	// Increase the sixe of the only box
	if(numboxes == 1)
		boxw[0] += 4;
	set_icon(thread->icon_image);

	y = 0;
	for(int i = 0; i < numboxes; i++)
	{
		BC_Title *title;
		add_subwindow(title = new BC_Title(hdrpos[i] + margin, TOP_MARGIN, timehdrs[i]));
		if(!y)
		{
			y = TOP_MARGIN + title->get_h();
			add_subwindow(signtitle = new BC_Title(margin - signw - INTERVAL, y, "=", LARGEFONT));
		}
		add_subwindow(boxes[i] = new ManualGotoNumber(this, boxpos[i] + margin, y,
			boxw[i], strlen(timeparts[i])));
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

	for(i = 0; i < numboxes; i++)
	{
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
	for(n = 0; n < 4; n++)
	{
		if(*p == 0)
			break;
		timeparts[n] = p;
		while(isdigit(*p))
			p++;
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
	if (numstr == 0)
		numstr = get_text();
	update(numstr);
	select_whole_text(-1);
}

int ManualGotoNumber::keypress_event()
{
	int in_textlen = strlen(get_text());
	int key = get_keypress();

	if(key == '+')
	{
		window->signtitle->update("+");
		return 1;
	}
	if(key == '-')
	{
		window->signtitle->update("-");
		return 1;
	}
	if(key == '=')
	{
		window->signtitle->update("=");
		return 1;
	}

	int ok_key = 0;

	if((key >= '0' && key <='9') ||
			(key == ESC) ||  (key == RETURN) ||
			(key == TAB) ||  (key == LEFTTAB) ||
			(key == '.') ||
			(key == LEFT) || (key == RIGHT) ||
			(key == UP) ||   (key == DOWN) ||
			(key == PGUP) || (key == PGDN) ||
			(key == END) ||  (key == HOME) ||
			(key == BACKSPACE) || (key == DELETE) ||
			(ctrl_down() && (key == 'v' || key == 'V' ||
				key == 'c' || key == 'C' || key == 'x' || key == 'X')))
		ok_key = 1;

	if (in_textlen >= chars && key >= '0' && key <= '9' && !select_whole_text(0))
		ok_key = 0;

	if(!ok_key)
		return 1;

// treat dot as tab - for use from numerical keypad
	if(key == '.')
	{
		cycle_textboxes(1);
		return 1; 
	};

	int result = BC_TextBox::keypress_event();
	int out_textlen = strlen(get_text());

// automatic cycle when we enter two numbers
	if(key != TAB && out_textlen == chars && get_ibeam_letter() == chars)
		cycle_textboxes(1);
	return result;
}
