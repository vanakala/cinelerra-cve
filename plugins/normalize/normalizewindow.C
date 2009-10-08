
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

#include "mwindow.inc"
#include "normalizewindow.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

NormalizeWindow::NormalizeWindow(int x, int y)
 : BC_Window(PROGRAM_NAME ": Normalize", 
 				x - 160,
				y - 75,
 				320, 
				150, 
				320, 
				150,
				0,
				0,
				1)
{ 
}

NormalizeWindow::~NormalizeWindow()
{
}

int NormalizeWindow::create_objects(float *db_over, int *separate_tracks)
{
	int x = 10, y = 10;
	this->db_over = db_over;
	this->separate_tracks = separate_tracks;
	add_subwindow(new BC_Title(x, y, _("Enter the DB to overload by:")));
	y += 20;
	add_subwindow(new NormalizeWindowOverload(x, y, this->db_over));
	y += 30;
	add_subwindow(new NormalizeWindowSeparate(x, y, this->separate_tracks));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	flush();
	return 0;
}

int NormalizeWindow::close_event()
{
	set_done(1);
	return 1;
}

NormalizeWindowOverload::NormalizeWindowOverload(int x, int y, float *db_over)
 : BC_TextBox(x, y, 200, 1, *db_over)
{
	this->db_over = db_over;
}

NormalizeWindowOverload::~NormalizeWindowOverload()
{
}
	
int NormalizeWindowOverload::handle_event()
{
	*db_over = atof(get_text());
	return 1;
}


NormalizeWindowSeparate::NormalizeWindowSeparate(int x, int y, int *separate_tracks)
 : BC_CheckBox(x, y, *separate_tracks, _("Treat tracks independantly"))
{
	this->separate_tracks = separate_tracks;
}

NormalizeWindowSeparate::~NormalizeWindowSeparate()
{
}
	
int NormalizeWindowSeparate::handle_event()
{
	*separate_tracks = get_value();
	return 1;
}
