
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

#include "levelwindow.h"
#include "levelwindowgui.h"
#include "mainmenu.h"
#include "mwindow.h"

LevelWindow::LevelWindow(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	Thread::set_synchronous(1);
}

LevelWindow::~LevelWindow()
{
	if(gui) 
	{
		gui->set_done(0);
		join();
	}
	if(gui) delete gui;
}

int LevelWindow::create_objects()
{
	gui = new LevelWindowGUI(mwindow, this);
	gui->create_objects();
	return 0;
}


void LevelWindow::run()
{
	if(gui) gui->run_window();
}
