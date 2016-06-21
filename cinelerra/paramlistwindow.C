
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

#include "asset.h"
#include "bcsubwindow.h"
#include "bcsignals.h"
#include "language.h"
#include "paramlist.h"
#include "paramlistwindow.h"
#include "subselection.h"
#include "theme.h"

#include <unistd.h>

ParamlistWindow::ParamlistWindow(int x, int y, int max_h, Paramlist *params)
 : BC_SubWindow(x,
	y,
	200,
	200)
{
	this->params = params;
	bottom_margin = max_h;
}

void ParamlistWindow::draw_list()
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

	name_width += 5;

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
					top, name_width - 5, this, current));
				w1 = win->get_w();
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

	int w = left + base_w + PARAMLIST_WIN_MARGIN;
	if(new_column && left > base_w)
		w -= base_w;
	reposition_window(0, 0, w, bot_max);
}

void ParamlistWindow::calc_pos(int h, int w)
{
	top += h + 5;

	if(w > base_w)
		base_w = w;

	if(bot_max < top)
		bot_max = top;

	if(top > bottom_margin)
	{
		top = base_y;
		left += base_w + 20;
		new_column = 1;
	}
	else
		new_column = 0;
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
