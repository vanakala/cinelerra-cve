// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcresources.h"
#include "bctitle.h"
#include "bcsignals.h"
#include "bcresources.h"
#include <string.h>
#include <unistd.h>
#include <wchar.h>

BC_Title::BC_Title(int x, 
		int y, 
		const char *text, 
		int font, 
		int color, 
		int centered,
		int fixed_w)
 : BC_SubWindow(x, y, -1, -1, -1)
{
	this->font = font;
	if(color < 0) 
		this->color = resources.default_text_color;
	else
		this->color = color;
	this->centered = centered;
	this->fixed_w = fixed_w;
	if(text)
		strcpy(this->text, text);
	else
		this->text[0] = 0;
	wide_title = 0;
}

BC_Title::BC_Title(int x,
		int y,
		const wchar_t *text,
		int font,
		int color,
		int centered,
		int fixed_w)
 : BC_SubWindow(x, y, -1, -1, -1)
{
	this->font = font;
	if(color < 0)
		this->color = resources.default_text_color;
	else
		this->color = color;
	this->centered = centered;
	this->fixed_w = fixed_w;
	wide_title = new wchar_t[BCTEXTLEN];
	wcscpy(wide_title, text);
}

BC_Title::BC_Title(int x,
		int y,
		int value,
		int font,
		int color,
		int centered,
		int fixed_w)
 : BC_SubWindow(x, y, -1, -1, -1)
{
	this->font = font;
	if(color < 0)
		this->color = resources.default_text_color;
	else
		this->color = color;
	this->centered = centered;
	this->fixed_w = fixed_w;
	sprintf(text, "%6d", value);
	wide_title = 0;
}

BC_Title::~BC_Title()
{
	delete [] wide_title;
}


void BC_Title::initialize()
{
	if(w <= 0 || h <= 0)
	{
		if(wide_title)
			get_size(this, font, wide_title, fixed_w, w, h);
		else
			get_size(this, font, text, fixed_w, w, h);
	}

	if(centered) x -= w / 2;

	BC_SubWindow::initialize();
	draw();
}

void BC_Title::set_color(int color)
{
	this->color = color;
	draw();
}

void BC_Title::resize(int w, int h)
{
	resize_window(w, h);
	draw();
}

void BC_Title::reposition(int x, int y)
{
	reposition_window(x, y, w, h);
	draw();
}


void BC_Title::update(const char *text)
{
	int new_w, new_h;

	if(wide_title)
	{
		delete [] wide_title;
		wide_title = 0;
	}
	strcpy(this->text, text);
	get_size(this, font, text, fixed_w, new_w, new_h);
	if(new_w > w || new_h > h)
	{
		resize_window(new_w, new_h);
	}
	draw();
}

void BC_Title::update(const wchar_t *text)
{
	int new_w, new_h;

	if(!wide_title)
		wide_title = new wchar_t[BCTEXTLEN];

	wcscpy(wide_title, text);
	get_size(this, font, wide_title, fixed_w, new_w, new_h);
	if(new_w > w || new_h > h)
	{
		resize_window(new_w, new_h);
	}
	draw();
}

void BC_Title::update(int value)
{
	char string[BCTEXTLEN];
	sprintf(string, "%6d", value);
	update(string);
}

void BC_Title::update(double value, int precision)
{
	char string[BCTEXTLEN];
	sprintf(string, "%.*f", precision, value);
	update(string);
}

char* BC_Title::get_text()
{
	return text;
}

void BC_Title::draw()
{
	int i, j, x, y, l;

	top_level->lock_window("BC_Title::draw");
// Fix background for block fonts.
// This should eventually be included in a BC_WindowBase::is_blocked_font()

	if(font == MEDIUM_7SEGMENT)
	{
	//leave it up to the theme to decide if we need a background or not.
		if(resources.draw_clock_background)
		{
			BC_WindowBase::set_color(BLACK);
			draw_box(0, 0, w, h);
		}
	}
	else
		draw_top_background(parent_window, 0, 0, w, h);

	set_font(font);
	BC_WindowBase::set_color(color);
	j = x = 0;
	y = get_text_ascent(font);

	if(wide_title)
	{
		l = wcslen(wide_title);

		for(i = 0; i <= l; i++)
		{
			if(wide_title[i] == '\n' || wide_title[i] == 0)
			{
				if(centered)
				{
					draw_center_text(get_w() / 2,
						y,
						&wide_title[j],
						i - j);
					j = i + 1;
				}
				else
				{
					draw_text(x,
						y,
						&wide_title[j],
						i - j);
					j = i + 1;
				}
				y += get_text_height(font);
			}
		}
	}
	else
	{
		l = strlen(text);
		for(i = 0; i <= l; i++)
		{
			if(text[i] == '\n' || text[i] == 0)
			{
				if(centered)
				{
					draw_center_text(get_w() / 2,
						y,
						&text[j],
						i - j);
					j = i + 1;
				}
				else
				{
					draw_text(x,
						y,
						&text[j],
						i - j);
					j = i + 1;
				}
				y += get_text_height(font);
			}
		}
	}
	set_font(MEDIUMFONT);    // reset
	flash();
	top_level->unlock_window();
}

int BC_Title::calculate_w(BC_WindowBase *gui, const char *text, int font)
{
	int temp_w, temp_h;
	get_size(gui, font, text, 0, temp_w, temp_h);
	return temp_w;
}

int BC_Title::calculate_h(BC_WindowBase *gui, const char *text, int font)
{
	int temp_w, temp_h;
	get_size(gui, font, text, 0, temp_w, temp_h);
	return temp_h;
}

void BC_Title::get_size(BC_WindowBase *gui, int font, const char *text, int fixed_w, int &w, int &h)
{
	int i, j, x, y, line_w = 0;
	w = 0;
	h = 0;

	for(i = 0, j = 0; i <= strlen(text); i++)
	{
		line_w = 0;
		if(text[i] == '\n')
		{
			h++;
			line_w = gui->get_text_width(font, &text[j], i - j);
			j = i + 1;
		}
		else
		if(text[i] == 0)
		{
			h++;
			line_w = gui->get_text_width(font, &text[j]);
		}
		if(line_w > w) w = line_w;
	}

	h *= gui->get_text_height(font);
	w += 5;
	if(fixed_w > 0) w = fixed_w;
}

void BC_Title::get_size(BC_WindowBase *gui, int font, const wchar_t *text, int fixed_w, int &w, int &h)
{
	int i, j, x, y, line_w = 0;
	w = 0;
	h = 0;

	for(i = 0, j = 0; i <= wcslen(text); i++)
	{
		line_w = 0;
		if(text[i] == '\n')
		{
			h++;
			line_w = gui->get_text_width(font, &text[j], i - j);
			j = i + 1;
		}
		else
		if(text[i] == 0)
		{
			h++;
			line_w = gui->get_text_width(font, &text[j]);
		}
		if(line_w > w) w = line_w;
	}

	h *= gui->get_text_height(font);
	w += 5;
	if(fixed_w > 0) w = fixed_w;
}
