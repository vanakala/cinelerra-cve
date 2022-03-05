// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcbutton.h"
#include "bclistbox.h"
#include "bclistboxitem.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bctitle.h"
#include "language.h"
#include "mainerror.h"
#include "mainsession.h"
#include "mutex.h"
#include "mwindow.h"

#include <string.h>
#include <stdarg.h>
#include <wchar.h>

MainError* MainError::main_error = 0;


MainErrorGUI::MainErrorGUI(MainError *thread, int x, int y)
 : BC_Window(MWindow::create_title(N_("Errors")),
	x,
	y,
	mainsession->ewindow_w,
	mainsession->ewindow_h,
	50,
	50,
	1,
	0,
	1,
	-1,
	"",
	1)
{
	this->thread = thread;
	set_icon(mwindow_global->get_window_icon());
	BC_Button *button;
	add_subwindow(button = new BC_OKButton(this));
	x = y = 10;
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
	mainsession->ewindow_w = w;
	mainsession->ewindow_h = h;
}


MainError::MainError()
 : BC_DialogThread()
{
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

	return new MainErrorGUI(this, x, y);
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
	if(get_gui())
	{
		MainErrorGUI *gui = (MainErrorGUI*)get_gui();
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
		start();
	}
	else
	{
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

wchar_t *MainError::StringBreaker(int font, const char *text, int boxwidth,
		BC_Window *win)
{
	int txlen = strlen(text);
	wchar_t *p, *q, *r;
	static wchar_t msgbuf[BCTEXTLEN];

	txlen = BC_Resources::encode(BC_Resources::encoding, BC_Resources::wide_encoding,
		(char*)text, (char*)msgbuf, BCTEXTLEN * sizeof(wchar_t), txlen) / sizeof(wchar_t);

	if(win->get_text_width(font, msgbuf) < boxwidth)
		return msgbuf;

	p = msgbuf;
	while(*p)
	{
		if(q = wcschr(p, '\n'))
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
			if(*q == ' ' || *q == '\t' || *q == '\n')
			{
				if(*r && win->get_text_width(font, p, q - p) > boxwidth)
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
		MainErrorBox ebox(bufr, message, confirm, x, y);
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

MainErrorBox::MainErrorBox(const char *title, const char *text, int confirm,
	int x, int y, int w, int h)
 : BC_Window(PROGRAM_NAME, x, y, w, h, w, h, 0)
{
	wchar_t *btext;

	set_icon(mwindow_global->get_window_icon());

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
}
