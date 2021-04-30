
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

#ifndef LOADFILE_H
#define LOADFILE_H

#include "bcfilebox.h"
#include "bcmenuitem.h"
#include "bctoggle.h"
#include "loadmode.inc"
#include "mainmenu.inc"
#include "thread.h"

class LoadFileThread;
class LoadFileWindow;

class Load : public BC_MenuItem
{
public:
	Load(MainMenu *mainmenu);
	~Load();

	int handle_event();

	MainMenu *mainmenu;
	LoadFileThread *thread;
};

class LoadFileThread : public Thread
{
public:
	LoadFileThread(Load *menuitem);

	void run();

	int load_mode;
	LoadFileWindow *window;
private:
	Load *load;
};

class NewTimeline;
class NewConcatenate;
class AppendNewTracks;
class EndofTracks;
class ResourcesOnly;

class LoadFileWindow : public BC_FileBox
{
public:
	LoadFileWindow(LoadFileThread *thread,
		int absx, int absy,
		char *init_directory);
	~LoadFileWindow();

	void resize_event(int w, int h);

	LoadFileThread *thread;
	LoadMode *loadmode;
	NewTimeline *newtimeline;
	NewConcatenate *newconcatenate;
	AppendNewTracks *newtracks;
	EndofTracks *concatenate;
	ResourcesOnly *resourcesonly;
};

class NewTimeline : public BC_Radial
{
public:
	NewTimeline(int x, int y, LoadFileWindow *window);

	int handle_event();
private:
	LoadFileWindow *window;
};

class NewConcatenate : public BC_Radial
{
public:
	NewConcatenate(int x, int y, LoadFileWindow *window);

	int handle_event();
private:
	LoadFileWindow *window;
};

class AppendNewTracks : public BC_Radial
{
public:
	AppendNewTracks(int x, int y, LoadFileWindow *window);

	int handle_event();
private:
	LoadFileWindow *window;
};

class EndofTracks : public BC_Radial
{
public:
	EndofTracks(int x, int y, LoadFileWindow *window);

	int handle_event();
private:
	LoadFileWindow *window;
};

class ResourcesOnly : public BC_Radial
{
public:
	ResourcesOnly(int x, int y, LoadFileWindow *window);

	int handle_event();
private:
	LoadFileWindow *window;
};


class LoadPrevious : public BC_MenuItem
{
public:
	LoadPrevious();

	int handle_event();
	void set_path(const char *path);
private:
	char path[BCTEXTLEN];
};


class LoadBackup : public BC_MenuItem
{
public:
	LoadBackup();

	int handle_event();
};

#endif
