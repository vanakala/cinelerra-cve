
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

#include "gwindow.h"
#include "gwindowgui.h"
#include "mwindow.h"
#include "mwindowgui.h"

GWindow::GWindow(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
}

void GWindow::run()
{
	gui->run_window();
}

void GWindow::create_objects()
{
	int w, h;


	GWindowGUI::calculate_extents(mwindow->gui, &w, &h);
	gui = new GWindowGUI(mwindow, w, h);
	gui->create_objects();
}



