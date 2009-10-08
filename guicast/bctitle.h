
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

#ifndef BCTITLE_H
#define BCTITLE_H

#include "bcsubwindow.h"
#include "colors.h"
#include "fonts.h"

class BC_Title : public BC_SubWindow
{
public:
	BC_Title(int x, 
		int y, 
		char *text, 
		int font = MEDIUMFONT, 
		int color = -1, 
		int centered = 0,
		int fixed_w = 0);
	virtual ~BC_Title();
	
	int initialize();
	static int calculate_w(BC_WindowBase *gui, char *text, int font = MEDIUMFONT);
	static int calculate_h(BC_WindowBase *gui, char *text, int font = MEDIUMFONT);
	int resize(int w, int h);
	int reposition(int x, int y);
	int set_color(int color);
	int update(char *text);
	void update(float value);
	char* get_text();

private:
	int draw();
	static void get_size(BC_WindowBase *gui, int font, char *text, int fixed_w, int &w, int &h);
	
	char text[BCTEXTLEN];
	int color;
	int font;
	int centered;
// Width if fixed
	int fixed_w;
};

#endif
