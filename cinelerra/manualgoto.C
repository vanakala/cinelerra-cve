
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

ManualGoto::ManualGoto(MWindow *mwindow, BC_WindowBase *masterwindow)
 : Thread()
{
	this->mwindow = mwindow;
	this->masterwindow = masterwindow;
	gotowindow = 0;
	done = 0;
}

ManualGoto::~ManualGoto()
{
	done = 1;
	gotowindow->set_done(1);
	Thread::join();
	if (gotowindow)
		delete gotowindow;
}

void ManualGoto::open_window()
{

	double position;
	if ((masterwindow == (BC_WindowBase *)mwindow->cwindow->gui)||
	   (masterwindow == (BC_WindowBase *)mwindow->gui->mbuttons))
	{
	
		position = mwindow->edl->local_session->get_selectionstart(1);
		position += mwindow->edl->session->get_frame_offset() / 
						 mwindow->edl->session->frame_rate;;
	}
	else
		if (mwindow->vwindow->get_edl())
			position = mwindow->vwindow->get_edl()->local_session->get_selectionstart(1);
		else
			return;
	if (!gotowindow)
	{
		gotowindow = new ManualGotoWindow(mwindow, this);
		gotowindow->create_objects();
		gotowindow->reset_data(position);
		Thread::start();
	} else
		gotowindow->reset_data(position);
}

void ManualGoto::run()
{
	int result = 1;
	while (!done) 
	{
		result = gotowindow->run_window();
		gotowindow->lock_window();
		gotowindow->hide_window();		
		gotowindow->unlock_window();

		if (!done && result == 0) // ok button or return pressed
		{
			double new_position = gotowindow->get_entered_position_sec();
			char modifier = gotowindow->signtitle->get_text()[0];
			if ((masterwindow == (BC_WindowBase *)mwindow->cwindow->gui)||
			   (masterwindow == (BC_WindowBase *)mwindow->gui->mbuttons))
			
			{
				// mwindow/cwindow update
				
				double current_position = mwindow->edl->local_session->get_selectionstart(1);
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
				new_position = mwindow->edl->align_to_frame(new_position, 1);
				new_position -= mwindow->edl->session->get_frame_offset() / mwindow->edl->session->frame_rate;;
				if (new_position < 0) 
					new_position = 0;
				if (current_position != new_position)
				{
					mwindow->edl->local_session->set_selectionstart(new_position);
					mwindow->edl->local_session->set_selectionend(new_position);
					mwindow->gui->lock_window();
					mwindow->find_cursor();
					mwindow->gui->update(1, 1, 1, 1, 1, 1, 0);	
					mwindow->gui->unlock_window();
					mwindow->cwindow->update(1, 0, 0, 0, 0);			
				}
			} else
			if ((masterwindow == (BC_WindowBase *)mwindow->vwindow->gui) &&
			    mwindow->vwindow->get_edl())
			{
				// vwindow update
				VWindow *vwindow = mwindow->vwindow;
				double current_position = vwindow->get_edl()->local_session->get_selectionstart(1);
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
				new_position = vwindow->get_edl()->align_to_frame(new_position, 1);
				if (current_position != new_position)
				{
					vwindow->get_edl()->local_session->set_selectionstart(new_position);
					vwindow->get_edl()->local_session->set_selectionend(new_position);
					vwindow->gui->lock_window();
					vwindow->update_position(CHANGE_NONE, 0, 1);			
					vwindow->gui->unlock_window();
				}
			}
		}		
	}
}



ManualGotoWindow::ManualGotoWindow(MWindow *mwindow, ManualGoto *thread)
 : BC_Window(PROGRAM_NAME ": Goto position", 
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
	this->mwindow = mwindow;
	this->thread = thread;
}

ManualGotoWindow::~ManualGotoWindow()
{
}

void ManualGotoWindow::reset_data(double position)
{
	lock_window();
	reposition_window(
			mwindow->gui->get_abs_cursor_x(1) - 250 / 2,
			mwindow->gui->get_abs_cursor_y(1) - 80 / 2);
	set_entered_position_sec(position);
	signtitle->update("=");
	activate();
	show_window();
	unlock_window();
}

double ManualGotoWindow::get_entered_position_sec()
{
	int64_t hh = atoi(boxhours->get_text());
	int64_t mm = atoi(boxminutes->get_text());
	int64_t ss = atoi(boxseconds->get_text());
	int64_t ms = atoi(boxmsec->get_text());
	
	double seconds = ((((hh * 60) + mm) * 60) + ss) + (1.0 * ms) / 1000;
	return seconds;
}

void ManualGotoWindow::set_entered_position_sec(double position)
{
// Lifted from units.C
	int hour, minute, second, thousandths;

	position = fabs(position);
	hour = (int)(position / 3600);
	minute = (int)(position / 60 - hour * 60);
	second = (int)position - (int64_t)hour * 3600 - (int64_t)minute * 60;
	thousandths = (int)(position * 1000) % 1000;
  		
	boxhours->reshape_update(hour);
	boxminutes->reshape_update(minute);
	boxseconds->reshape_update(second);
	boxmsec->reshape_update(thousandths);
}

int ManualGotoWindow::activate()
{	
	boxhours->reshape_update(-1);
	boxminutes->reshape_update(-1);
	boxseconds->reshape_update(-1);
	boxmsec->reshape_update(-1);
	int result = BC_Window::activate();
	boxminutes->deactivate();
	boxseconds->deactivate();
	boxmsec->deactivate();
	boxhours->activate();
	return result;	
}	
void ManualGotoWindow::create_objects()
{
	int x = 76, y = 5;
	int x1 = x;
	int boxwidth = 26;

	BC_Title *title;
	add_subwindow(title = new BC_Title(x1 - 2, y, _("hour  min     sec     msec"), SMALLFONT));
	y += title->get_h() + 3;

	add_subwindow(signtitle = new BC_Title(x1 - 17, y, "=", LARGEFONT));
	add_subwindow(boxhours = new ManualGotoNumber(this, x1, y, 16, 0, 9, 1));
	x1 += boxhours->get_w() + 4;
	add_subwindow(boxminutes = new ManualGotoNumber(this, x1, y, 26, 0, 59, 2));
	x1 += boxminutes->get_w() + 4;
	add_subwindow(boxseconds = new ManualGotoNumber(this, x1, y, 26, 0, 59, 2));
	x1 += boxseconds->get_w() + 4;
	add_subwindow(boxmsec = new ManualGotoNumber(this, x1, y, 34, 0, 999, 3));
	y += boxhours->get_h() + 10;

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
}





ManualGotoNumber::ManualGotoNumber(ManualGotoWindow *window, int x, int y, int w, int min_num, int max_num, int chars)
 : BC_TextBox(x, y, w, 1, "0")
{
	this->window = window;
	this->min_num = min_num;
	this->max_num = max_num;
	this->chars = chars;
}

int ManualGotoNumber::handle_event()
{
	return 1;
}

int ManualGotoNumber::deactivate()
{
	reshape_update(-1);
	return BC_TextBox::deactivate();
}

int ManualGotoNumber::activate()
{
	int retval = BC_TextBox::activate();
	select_whole_text(1);
	return retval;
}


void ManualGotoNumber::reshape_update(int64_t number)
{
	char format_text[10];
	char text[BCTEXTLEN];
	if (number < 0)
		number = atoll(get_text());
	if (number > max_num) number = max_num;
	if (number < min_num) number = min_num;
	sprintf(format_text, "%%0%dlli", chars);
	sprintf(text, format_text, number);
	update(text);
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







