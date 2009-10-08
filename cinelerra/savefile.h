
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

#ifndef SAVEFILE_H
#define SAVEFILE_H

#include "guicast.h"
#include "mainmenu.inc"
#include "mwindow.inc"
#include "savefile.inc"

class SaveBackup : public BC_MenuItem
{
public:
	SaveBackup(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class Save : public BC_MenuItem
{
public:
	Save(MWindow *mwindow);
	int handle_event();
	int create_objects(SaveAs *saveas);
	int save_before_quit();
	
	int quit_now;
	MWindow *mwindow;
	SaveAs *saveas;
};

class SaveAs : public BC_MenuItem, public Thread
{
public:
	SaveAs(MWindow *mwindow);
	int set_mainmenu(MainMenu *mmenu);
	int handle_event();
	void run();
	
	int quit_now;
	MWindow *mwindow;
	MainMenu *mmenu;
};

class SaveFileWindow : public BC_FileBox
{
public:
	SaveFileWindow(MWindow *mwindow, char *init_directory);
	~SaveFileWindow();
	MWindow *mwindow;
};

#endif
