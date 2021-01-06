// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "asset.h"
#include "bcsubwindow.h"
#include "bcsignals.h"
#include "bctitle.h"
#include "bcresources.h"
#include "language.h"
#include "mutex.h"
#include "paramlist.h"
#include "paramlistwindow.h"
#include "subselection.h"
#include "theme.h"
#include "vframe.h"

#include <unistd.h>

ParamlistSubWindow::ParamlistSubWindow(int x, int y, int max_h, Paramlist *params)
 : BC_SubWindow(x,
	y,
	200,
	200)
{
	this->params = params;
	bottom_margin = max_h;
	win_x = x;
	win_y = y;
}

void ParamlistSubWindow::draw_list()
{
	Param *current, *subparam;
	BC_WindowBase *win = 0;
	int w1 = 0, h1 = 0, tw;
	int name_width;

	top = left = PARAMLIST_WIN_MARGIN;
	base_w = 0;
	base_y = top;
	bot_max = 0;
	new_column = 0;
	name_width = 0;

	for(current = params->first; current; current = current->next)
	{
		if((tw = get_text_width(MEDIUMFONT, current->name)) > name_width)
			name_width = tw;
	}

	name_width += 8;

	for(current = params->first; current; current = current->next)
	{
		calc_pos(h1, w1);
		h1 = w1 = 0;

		switch(current->type & PARAMTYPE_MASK)
		{
		case PARAMTYPE_LNG:
			if(current->subparams)
			{
				add_subwindow(win = new SubSelection(left,
					top, name_width, this, current));
				w1 = win->get_w();
			}
			else
			if(current->type & PARAMTYPE_BOOL)
			{
				add_subwindow(new BC_Title(left, top, current->name));
				add_subwindow(win = new ParamChkBox(left + name_width,
				top, current, &current->longvalue));
				w1 = name_width + win->get_w();
			}
			else
			{
				add_subwindow(new BC_Title(left, top, current->name));
				add_subwindow(win = new Parami64Txtbx(left + name_width,
					top, current, &current->longvalue));
				w1 = name_width + win->get_w();
			}
			h1 = win->get_h();
			break;
		case PARAMTYPE_STR:
			add_subwindow(new BC_Title(left, top, current->name));
			add_subwindow(win = new ParamStrTxtbx(left + name_width,
				top, current, current->stringvalue));
			w1 = name_width + win->get_w();
			h1 = win->get_h();
			break;
		case PARAMTYPE_DBL:
			add_subwindow(new BC_Title(left, top, current->name));
			add_subwindow(win = new ParamDblTxtbx(left + name_width,
				top, current, &current->floatvalue));
			w1 = name_width + win->get_w();
			h1 = win->get_h();
			break;
		default:
			break;
		}
	}
	calc_pos(h1, w1);
	add_subwindow(win = new ParamResetButton(left, top, this));
	w1 = base_w - win->get_w() - 10;
	if(w1 < 0)
		w1 = 0;
	win->reposition_window(left + w1, top + 5);
	calc_pos(win->get_h() + 5, w1);

	int w = left + base_w + PARAMLIST_WIN_MARGIN;
	if(new_column && left > base_w)
		w -= base_w + PARAMLIST_WIN_MARGIN;
	reposition_window(win_x, win_y, w, bot_max);
}

void ParamlistSubWindow::calc_pos(int h, int w)
{
	top += h + 5;

	if(w > base_w)
		base_w = w;

	if(bot_max < top)
		bot_max = top;

	if(top > bottom_margin)
	{
		top = base_y;
		left += base_w + PARAMLIST_WIN_MARGIN;
		new_column = 1;
	}
	else
		new_column = 0;
}

BC_WindowBase *ParamlistSubWindow::set_scrollbar(int x, int y, int w)
{
	return new ParamWindowScroll(this, x, y, w, get_w());
}

int ParamlistSubWindow::max_name_size(Paramlist *list, BC_WindowBase *win, int mxlen)
{
	Param *current;
	int tw;

	if(list)
	{
		for(current = list->first; current; current = current->next)
		{
			if((tw = win->get_text_width(MEDIUMFONT, current->name)) > mxlen)
				mxlen = tw;
		}
	}
	return mxlen;
}

int ParamlistSubWindow::handle_reset()
{
	params->reset_defaults();
	return 1;
}

Parami64Txtbx::Parami64Txtbx(int x, int y, Param *param, int64_t *val)
 : BC_TextBox(x, y, 100, 1, *val)
{
	this->param = param;
	if(param->helptext)
		set_tooltip(param->helptext);
	valptr = val;
}

int Parami64Txtbx::handle_event()
{
	*valptr = atol(get_text());
	return 1;
}


ParamStrTxtbx::ParamStrTxtbx(int x, int y, Param *param, const char *str)
 : BC_TextBox(x, y, 100, 1, str)
{
	this->param = param;
	if(param->helptext)
		set_tooltip(param->helptext);
}

ParamStrTxtbx::~ParamStrTxtbx()
{
	param->set_string(get_text());
}

ParamDblTxtbx::ParamDblTxtbx(int x, int y, Param *param, double *val)
 : BC_TextBox(x, y, 100, 1, (float)*val)
{
	this->param = param;
	if(param->helptext)
		set_tooltip(param->helptext);
	valptr = val;
}

int ParamDblTxtbx::handle_event()
{
	*valptr = atof(get_text());
	return 1;
}

ParamChkBox::ParamChkBox(int x, int y, Param *param, int64_t *val)
 : BC_CheckBox(x, y, (int)(*val & 0xff))
{
	this->param = param;
	if(param->helptext)
		set_tooltip(param->helptext);
	valptr = val;
}

int ParamChkBox::handle_event()
{
	int val = get_value();

	if(val)
		*valptr = (int64_t)1;
	else
		*valptr = 0;
	return 1;
}

ParamWindowScroll::ParamWindowScroll(ParamlistSubWindow *listwin,
	int x, int y, int pixels, int length)
 : BC_ScrollBar(x, y, SCROLL_HORIZ, pixels, length, 0,
    round((double)pixels / length * pixels))
{
	this->paramwin = listwin;
	param_x = paramwin->get_x();
	param_y = paramwin->get_y();
	zoom = (double)pixels / length;
}

int ParamWindowScroll::handle_event()
{
	paramwin->reposition_window(param_x - (get_value() * zoom), param_y);
	return 1;
}


ParamlistWindow::ParamlistWindow(Paramlist *params, const char *winname,
    VFrame *window_icon)
 : BC_Window(winname,
	200,
	100,
	200,
	200)
{
	ParamlistSubWindow *listwin;

	if(window_icon)
		set_icon(window_icon);

	add_subwindow(listwin = new ParamlistSubWindow(0, 10, PARAMLIST_WIN_MAXH, params));
	listwin->draw_list();

	int w = listwin->get_w();
	int h = listwin->get_h() + BC_OKButton::calculate_h() + 40;

	int max_w = get_root_w(1) - 100;
	if(w > max_w)
	{
		add_subwindow(listwin->set_scrollbar(0, listwin->get_h() + 10, max_w));
		w = max_w;
	}
	reposition_window((get_root_w(1) - w) / 2, (get_root_h(1) - h) / 2,
		w, h);
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
}

ParamlistThread::ParamlistThread(Paramlist **paramp, const char *name,
	VFrame *window_icon, BC_WindowBase **list_window)
 : Thread()
{
	strcpy(window_title, name);
	this->paramp = paramp;
	this->window_icon = window_icon;
	this->list_window = list_window;
	window = 0;
	win_result = 1;
	window_lock = new Mutex("ParamlistThread::window_lock");
}

void ParamlistThread::set_window_title(const char *name)
{
	strcpy(window_title, name);
}

void ParamlistThread::run()
{
	window_lock->lock("ParamlistThread::run");
	win_result = 1;
	window = new ParamlistWindow(*paramp, window_title, window_icon);
	if(list_window)
		*list_window = window;
	win_result = window->run_window();
	delete window;
	window = 0;
	window_lock->unlock();
}

void ParamlistThread::cancel_window()
{
	if(running() && window)
		window->set_done(1);
	wait_window();
}

void ParamlistThread::wait_window()
{
	window_lock->lock("ParamlistThread::wait_window");
	window_lock->unlock();
}

ParamlistThread::~ParamlistThread()
{
	cancel_window();
	window_lock->lock("ParamlistThread::~ParamlistThread");
	delete window_lock;
}

void ParamlistThread::show_window()
{
	if(!running())
		start();
	else if(window)
		window->raise_window();
}

ParamResetButton::ParamResetButton(int x, int y, ParamlistSubWindow *parent)
 : BC_GenericButton(x, y, _("Reset"))
{
	parentwindow = parent;
}

int ParamResetButton::handle_event()
{
	return parentwindow->handle_reset();
}

int ParamResetButton::keypress_event()
{

	int k = get_keypress();

	if(k == 'r' || k == 'R')
		return handle_event();

	return 0;
}
