#include "bctitle.h"
#include <string.h>

BC_Title::BC_Title(int x, 
		int y, 
		char *text, 
		int font, 
		int color, 
		int centered,
		int fixed_w)
 : BC_SubWindow(x, y, -1, -1, -1)
{
	this->font = font;
	this->color = color;
	this->centered = centered;
	this->fixed_w = fixed_w;
	strcpy(this->text, text);
}

BC_Title::~BC_Title()
{
}


int BC_Title::initialize()
{
	if(w <= 0 || h <= 0)
		get_size(w, h);

	if(centered) x -= w / 2;

	BC_SubWindow::initialize();
	draw();
	return 0;
}

int BC_Title::set_color(int color)
{
	this->color = color;
	draw();
	return 0;
}

int BC_Title::resize(int w, int h)
{
	resize_window(w, h);
	draw();
	return 0;
}

int BC_Title::reposition(int x, int y)
{
	reposition_window(x, y, w, h);
	draw();
	return 0;
}


int BC_Title::update(char *text)
{
	int new_w, new_h;

	strcpy(this->text, text);
	get_size(new_w, new_h);
	if(new_w > w || new_h > h)
	{
		resize_window(new_w, new_h);
	}

	draw();
	return 0;
}

int BC_Title::draw()
{
	int i, j, x, y;

// Fix background for block fonts.
// This should eventually be included in a BC_WindowBase::is_blocked_font()

 	if(font == MEDIUM_7SEGMENT)
 	{
 		BC_WindowBase::set_color(BLACK);
 		draw_box(0, 0, w, h);
 	}
	else
 		draw_top_background(parent_window, 0, 0, w, h);

	set_font(font);
	BC_WindowBase::set_color(color);
	for(i = 0, j = 0, x = 0, y = get_text_ascent(font); 
		i <= strlen(text); 
		i++)
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
	set_font(MEDIUMFONT);    // reset
	flash();
	return 0;
}

int BC_Title::get_size(int &w, int &h)
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
			line_w = get_text_width(font, &text[j], i - j);
			j = i + 1;
		}
		else
		if(text[i] == 0)
		{
			h++;
			line_w = get_text_width(font, &text[j]);
		}
		if(line_w > w) w = line_w;
	}

	h *= get_text_height(font);
	w += 5;
	if(fixed_w > 0) w = fixed_w;
//printf("BC_Title::get_size %d\n", w);
	return 0;
}
