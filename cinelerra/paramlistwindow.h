
/*
 * CINELERRA
 * Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#ifndef PARAMLISTWINDOW_H
#define PARAMLISTWINDOW_H

#include "bcbutton.h"
#include "bcsubwindow.h"
#include "bctextbox.h"
#include "bctoggle.h"
#include "paramlist.h"
#include "paramlistwindow.inc"
#include "subselection.h"
#include "thread.h"


class ParamlistWindow : public BC_SubWindow
{
public:
	ParamlistWindow(int x, int y, int max_h, Paramlist *params);

	void draw_list();
	void calc_pos(int h, int w);

private:
	Paramlist *params;
	int top;
	int left;
	int base_w;
	int base_y;
	int bottom_margin;
	int new_column;
	int bot_max;
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

#endif
