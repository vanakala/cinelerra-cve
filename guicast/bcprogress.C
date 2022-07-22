// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "fonts.h"
#include "bcprogress.h"
#include "bcpixmap.h"
#include "bcresources.h"

#include <math.h>

BC_ProgressBar::BC_ProgressBar(int x, int y, int w, double length, int do_text)
 : BC_SubWindow(x, y, w, 0, -1)
{
	this->length = length;
	this->do_text = do_text;
	position = 0;
	pixel = 0;
	image_up = 0;
	image_hi = 0;
	do_text = 1;
}

BC_ProgressBar::~BC_ProgressBar()
{
	delete image_up;
	delete image_hi;
}

void BC_ProgressBar::initialize()
{
	set_images();
	h = image_up->get_h();

	BC_SubWindow::initialize();
	draw(1);
}

void BC_ProgressBar::reposition_window(int x, int y, int w, int h)
{
	if(w < 0)
		w = get_w();
	if(h < 0)
		h = get_h();
	BC_WindowBase::reposition_window(x, y, w, h);
	draw(1);
}

void BC_ProgressBar::set_do_text(int value)
{
	this->do_text = value;
}

void BC_ProgressBar::set_images()
{
	delete image_up;
	delete image_hi;

	image_up = new BC_Pixmap(parent_window, resources.progress_images[0],
		PIXMAP_ALPHA);
	image_hi = new BC_Pixmap(parent_window, resources.progress_images[1],
		PIXMAP_ALPHA);
}

void BC_ProgressBar::draw(int force)
{
	char string[32];
	int new_pixel;

	new_pixel = round((position / length) * w);

	if(new_pixel != pixel || force)
	{
		top_level->lock_window("BC_ProgressBar::draw");
		pixel = new_pixel;
// Clear background
		draw_top_background(parent_window, 0, 0, w, h);
		draw_3segmenth(0, 0, pixel, 0, w, image_hi);
		draw_3segmenth(pixel, 0, w - pixel, 0, w, image_up);

		if(do_text)
		{
			set_font(MEDIUMFONT);
			set_color(resources.progress_text);     // draw decimal percentage
			sprintf(string, "%d%%", (int)round(100 * position / length / w));
			draw_center_text(w / 2, h / 2 + get_text_ascent(MEDIUMFONT) / 2, string);
		}
		flash();
		top_level->unlock_window();
	}
}

void BC_ProgressBar::update(double position)
{
	this->position = position;
	draw();
}

void BC_ProgressBar::update_length(double length)
{
	this->length = length;
	position = 0;

	draw();
}
