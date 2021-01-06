// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef PARAMLISTWINDOW_H
#define PARAMLISTWINDOW_H

#include "bcbutton.h"
#include "bcsubwindow.h"
#include "bctextbox.h"
#include "bctoggle.h"
#include "bcwindow.h"
#include "paramlist.h"
#include "paramlistwindow.inc"
#include "subselection.h"
#include "thread.h"


class ParamlistSubWindow : public BC_SubWindow
{
public:
	ParamlistSubWindow(int x, int y, int max_h, Paramlist *params);

	void draw_list();
	BC_WindowBase *set_scrollbar(int x, int y, int w);

	static int max_name_size(Paramlist *list, BC_WindowBase *win, int mxlen = 0);
	int handle_reset();
private:
	void calc_pos(int h, int w);

	Paramlist *params;
	int win_x;
	int win_y;
	int top;
	int left;
	int base_w;
	int base_y;
	int bottom_margin;
	int new_column;
	int bot_max;
};


class ParamWindowScroll : public BC_ScrollBar
{
public:
	ParamWindowScroll(ParamlistSubWindow *listwin,
		int x, int y, int pixels, int length);

	int handle_event();
private:
	ParamlistSubWindow *paramwin;
	int param_x;
	int param_y;
	double zoom;
};

class Parami64Txtbx : public BC_TextBox
{
public:
	Parami64Txtbx(int x, int y, Param *param, int64_t *val);

	int handle_event();
private:
	Param *param;
	int64_t *valptr;
};


class ParamStrTxtbx : public BC_TextBox
{
public:
	ParamStrTxtbx(int x, int y, Param *param, const char *str);
	~ParamStrTxtbx();

private:
	Param *param;
};

class ParamDblTxtbx : public BC_TextBox
{
public:
	ParamDblTxtbx(int x, int y, Param *param, double *val);

	int handle_event();
private:
	Param *param;
	double *valptr;
};


class ParamChkBox : public BC_CheckBox
{
public:
	ParamChkBox(int x, int y, Param *param, int64_t *val);

	int handle_event();

private:
	Param *param;
	int64_t *valptr;
};


class ParamlistWindow : public BC_Window
{
public:
	ParamlistWindow(Paramlist *params, const char *winname,
		VFrame *window_icon = 0);
};

class ParamlistThread : public Thread
{
public:
	ParamlistThread(Paramlist **paramp, const char *name,
		VFrame *window_icon = 0, BC_WindowBase **list_window = 0);
	~ParamlistThread();

	void run();
	void cancel_window();
	void wait_window();
	void set_window_title(const char *name);
	void show_window();

	ParamlistWindow *window;
	int win_result;

private:
	char window_title[BCTEXTLEN];
	Paramlist **paramp;
	VFrame *window_icon;
	BC_WindowBase **list_window;
	Mutex *window_lock;
};

class ParamResetButton : public BC_GenericButton
{
public:
	ParamResetButton(int x, int y, ParamlistSubWindow *parent);

	int handle_event();
	int keypress_event();

private:
	ParamlistSubWindow *parentwindow;
};

#endif
