
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

#include "bcsignals.h"
#include "language.h"
#include "mainerror.h"
#include "mainsession.h"
#include "mutex.h"
#include "mwindow.h"
#include "theme.h"

#include <string.h>
#include <ctype.h>
#include <stdarg.h>


MainError* MainError::main_error = 0;





MainErrorGUI::MainErrorGUI(MWindow *mwindow, MainError *thread, int x, int y)
 : BC_Window("Errors - " PROGRAM_NAME,
	x,
	y,
	mwindow->session->ewindow_w,
	mwindow->session->ewindow_h,
	50,
	50,
	1,
	0,
	1,
	-1,
	"",
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

MainErrorGUI::~MainErrorGUI()
{
}

void MainErrorGUI::create_objects()
{
	set_icon(mwindow->theme->get_image("mwindow_icon"));
	BC_Button *button;
	add_subwindow(button = new BC_OKButton(this));
	int x = 10, y = 10;
	add_subwindow(title = new BC_Title(x, y, _("The following errors occurred:")));
	y += title->get_h() + 5;
	add_subwindow(list = new BC_ListBox(x,
		y,
		get_w() - 20,
		button->get_y() - y - 5,
		&thread->errors));       // Each column has an ArrayList of BC_ListBoxItems.
	show_window();
}

void MainErrorGUI::resize_event(int w, int h)
{
	title->reposition_window(title->get_x(), title->get_y());
	int list_h = h - 
		list->get_y() - 
		(get_h() - list->get_y() - list->get_h());
	int list_w = w - 
		list->get_x() - 
		(get_w() - list->get_x() - list->get_w());
	list->reposition_window(list->get_x(),
		list->get_y(),
		list_w,
		list_h);
	mwindow->session->ewindow_w = w;
	mwindow->session->ewindow_h = h;
}






MainError::MainError(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	errors_lock = new Mutex("MainError::errors_lock");
	main_error = this;
}

MainError::~MainError()
{
	delete errors_lock;
}

BC_Window* MainError::new_gui()
{
	int x, y;

	BC_Resources::get_abs_cursor(&x, &y);

	MainErrorGUI *gui = new MainErrorGUI(mwindow, this, x, y);
	gui->create_objects();
	return gui;
}

void MainError::append_error(const char *string)
{
	const char *in_ptr = string;
	char string2[BCTEXTLEN];
	int first_line = 1;
	while(*in_ptr)
	{
		char *out_ptr = string2;
// Indent remaining lines
		if(!first_line)
		{
			*out_ptr++ = ' ';
			*out_ptr++ = ' ';
		}

		while(*in_ptr != '\n' &&
			*in_ptr)
		{
			*out_ptr++ = *in_ptr++;
		}
		*out_ptr++ = 0;

		errors.append(new BC_ListBoxItem(string2));

		if(*in_ptr == '\n')
		{
			in_ptr++;
			first_line = 0;
		}
	}
}

void MainError::show_error_local(const char *string)
{
// assume user won't get to closing the GUI here
	lock_window("MainError::show_error_local");
	if(get_gui())
	{
		MainErrorGUI *gui = (MainErrorGUI*)get_gui();
		gui->lock_window("MainError::show_error_local");
		append_error(string);
		gui->list->update(&errors,
			0,
			0,
			1,
			gui->list->get_xposition(),
			gui->list->get_yposition(),
			gui->list->get_highlighted_item(),  // Flat index of item cursor is over
			0,     // set all autoplace flags to 1
			1);
		gui->unlock_window();
		unlock_window();
		start();
	}
	else
	{
		unlock_window();
		errors.remove_all_objects();
		append_error(string);
		start();
	}
}


void MainError::show_error(const char *string)
{
	if(main_error)
		main_error->show_error_local(string);
	else
	{
		fprintf(stderr, "ERROR: %s", string);
		if(string[strlen(string) - 1] != '\n')
			fputc('\n', stderr);
	}
}

const char *MainError::StringBreaker(int font, const char *text, int boxwidth,
		BC_Window *win)
{
	int txlen = strlen(text);
	char *p, *q, *r;
	static char msgbuf[BCTEXTLEN];

	if(win->get_text_width(font, text) < boxwidth)
		return text;

	p = strncpy(msgbuf, text, sizeof(msgbuf)-1);

	while (*p)
	{
		if(q = strchr(p, '\n'))
		{
			if(win->get_text_width(font, p, q - p) < boxwidth)
			{
				p = ++q;
				continue;
			}
		}

		if(win->get_text_width(font, msgbuf) < boxwidth)
			return msgbuf;

		r = &msgbuf[txlen];
		q = p;
		while(*q)
		{
			if(isspace(*q))
			{
				if(win->get_text_width(font, p, q - p) > boxwidth)
				{
					*r = '\n';
					p = ++r;
					break;
				}
				else
					r = q;
			}
			if(*++q == 0)
			{
// There was a very long word if we reach here
				*r = '\n';
				return msgbuf;
			}
		}
	}
	return msgbuf;
}

int MainError::show_boxmsg(const char *title, const char *message, int confirm)
{
	char bufr[BCTEXTLEN];
	int res;
	int x, y;

	if(main_error)
	{
		BC_Resources::get_abs_cursor(&x, &y);
		bufr[0] = 0;
		if(title)
		{
			strcpy(bufr, title);
			strcat(bufr, " - ");
		}
		strcat(bufr, PROGRAM_NAME);
		MainErrorBox ebox(main_error->mwindow, x, y);
		ebox.create_objects(bufr, message, confirm);
		res = ebox.run_window();
	}
	else
	{
		if(title)
			fprintf(stderr, "%s: %s", title, message);
		else
			fprintf(stderr, "%s", message);
		if(confirm)
		{
			int ch;
			fputs(" [Yn]", stderr);
			ch = getchar();
			if(ch == 'y' || ch == 'Y' || ch == '\n')
				res = 0;
			else
				res = 1;
		}
		else
			fputc('\n', stderr);
	}
	if(confirm)
		return res;
	return 0;
}

void MainError::ErrorBoxMsg(const char *fmt, ...)
{
	char bufr[BCTEXTLEN];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(bufr, BCTEXTLEN, fmt, ap);
	va_end(ap);
	show_boxmsg(_("Error"), bufr);
}

void MainError::ErrorMsg(const char *fmt, ...)
{
	char bufr[BCTEXTLEN];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(bufr, BCTEXTLEN, fmt, ap);
	va_end(ap);
	show_error(bufr);
}

void MainError::MessageBox(const char *fmt, ...)
{
	char bufr[BCTEXTLEN];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(bufr, BCTEXTLEN, fmt, ap);
	va_end(ap);
	show_boxmsg(_("Message"), bufr);
}

int MainError::ConfirmBox(const char *fmt, ...)
{
	char bufr[BCTEXTLEN];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(bufr, BCTEXTLEN, fmt, ap);
	va_end(ap);
	return show_boxmsg(_("Confirmation"), bufr, 1);
}

int MainError::va_MessageBox(const char *hdr, const char *fmt, va_list ap)
{
	char bufr[BCTEXTLEN];

	vsnprintf(bufr, BCTEXTLEN, fmt, ap);
	return show_boxmsg(hdr, bufr);
}

MainErrorBox::MainErrorBox(MWindow *mwindow, int x, int y, int w, int h)
: BC_Window(PROGRAM_NAME, x, y, w, h, w, h, 0)
{
	set_icon(mwindow->theme->get_image("mwindow_icon"));
}

MainErrorBox::~MainErrorBox()
{
}

int MainErrorBox::create_objects(const char *title, const char *text, int confirm)
{
	int x, y;
	const char *btext;

	if(title)
		set_title(title);

	btext = MainError::StringBreaker(MEDIUMFONT, text, get_w() - 30, this);

	add_subwindow(new BC_Title(get_w() / 2,
		10,
		btext,
		MEDIUMFONT,
		get_resources()->default_text_color,
		1));

	y = get_h() - 50;
	if(confirm)
	{
		x = get_w() / 4;
		add_tool(new BC_OKButton(x - 30, y));
		x *= 3;
		add_tool(new BC_CancelButton(x - 30, y));
	}
	else
	{
		x = get_w() / 2 - 30;
		add_tool(new BC_OKButton(x, y));
	}
	return 0;
}
