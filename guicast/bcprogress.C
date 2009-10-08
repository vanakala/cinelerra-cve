
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

#include <stdio.h>
#include <unistd.h>

#define PROGRESS_UP 0
#define PROGRESS_HI 1

#include "colors.h"
#include "fonts.h"
#include "bcprogress.h"
#include "bcpixmap.h"
#include "bcresources.h"

BC_ProgressBar::BC_ProgressBar(int x, int y, int w, int64_t length, int do_text)
 : BC_SubWindow(x, y, w, 0, -1)
{
	this->length = length;
	this->do_text = do_text;
	position = 0;
	pixel = 0;
	for(int i = 0; i < 2; i++) images[i] = 0;
	do_text = 1;
}

BC_ProgressBar::~BC_ProgressBar()
{
    for(int i = 0; i < 2; i++)
  	    if (images[i]) delete images[i];
}

int BC_ProgressBar::initialize()
{
	set_images();
	h = images[PROGRESS_UP]->get_h();

	BC_SubWindow::initialize();
	draw(1);
	return 0;
}

int BC_ProgressBar::reposition_window(int x, int y, int w, int h)
{
	if(w < 0) w = get_w();
	if(h < 0) h = get_h();
	BC_WindowBase::reposition_window(x, y, w, h);
	draw(1);
}

void BC_ProgressBar::set_do_text(int value)
{
	this->do_text = value;
}

int BC_ProgressBar::set_images()
{
	for(int i = 0; i < 2; i++)
		if(images[i]) delete images[i];

	for(int i = 0; i < 2; i++)
	{
		images[i] = new BC_Pixmap(parent_window, 
			get_resources()->progress_images[i], 
			PIXMAP_ALPHA);
	}
	return 0;
}


int BC_ProgressBar::draw(int force)
{
	char string[32];
	int new_pixel;

	new_pixel = (int)(((float)position / length) * get_w());

	if(new_pixel != pixel || force)
	{
		pixel = new_pixel;
// Clear background
		draw_top_background(parent_window, 0, 0, get_w(), get_h());
		draw_3segmenth(0, 0, pixel, 0, get_w(), images[PROGRESS_HI]);
		draw_3segmenth(pixel, 0, get_w() - pixel, 0, get_w(), images[PROGRESS_UP]);


		if(do_text)
		{
			set_font(MEDIUMFONT);
			set_color(get_resources()->progress_text);     // draw decimal percentage
			sprintf(string, "%d%%", (int)(100 * (float)position / length + 0.5 / w));
			draw_center_text(w / 2, h / 2 + get_text_ascent(MEDIUMFONT) / 2, string);
		}
		flash();
	}
	return 0;
}

int BC_ProgressBar::update(int64_t position)
{
	this->position = position;
	draw();
	return 0;
}

int BC_ProgressBar::update_length(int64_t length)
{
	this->length = length;
	position = 0;

	draw();
	return 0;
}

