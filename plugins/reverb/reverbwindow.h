
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

#ifndef REVERBWINDOW_H
#define REVERBWINDOW_H

#define TOTAL_LOADS 5

class ReverbThread;
class ReverbWindow;

#include "guicast.h"
#include "mutex.h"
#include "pluginclient.h"
#include "reverb.inc"



PLUGIN_THREAD_HEADER(Reverb, ReverbThread, ReverbWindow)


class ReverbLevelInit;
class ReverbDelayInit;
class ReverbRefLevel1;
class ReverbRefLevel2;
class ReverbRefTotal;
class ReverbRefLength;
class ReverbLowPass1;
class ReverbLowPass2;
class ReverbMenu;

class ReverbWindow : public BC_Window
{
public:
	ReverbWindow(Reverb *reverb, int x, int y);
	~ReverbWindow();
	
	int create_objects();
	void close_event();
	
	Reverb *reverb;
	ReverbLevelInit *level_init;
	ReverbDelayInit *delay_init;
	ReverbRefLevel1 *ref_level1;
	ReverbRefLevel2 *ref_level2;
	ReverbRefTotal *ref_total;
	ReverbRefLength *ref_length;
	ReverbLowPass1 *lowpass1;
	ReverbLowPass2 *lowpass2;
	ReverbMenu *menu;
};

class ReverbLevelInit : public BC_FPot
{
public:
	ReverbLevelInit(Reverb *reverb, int x, int y);
	~ReverbLevelInit();
	int handle_event();
	Reverb *reverb;
};

class ReverbDelayInit : public BC_IPot
{
public:
	ReverbDelayInit(Reverb *reverb, int x, int y);
	~ReverbDelayInit();
	int handle_event();
	Reverb *reverb;
};

class ReverbRefLevel1 : public BC_FPot
{
public:
	ReverbRefLevel1(Reverb *reverb, int x, int y);
	~ReverbRefLevel1();
	int handle_event();
	Reverb *reverb;
};

class ReverbRefLevel2 : public BC_FPot
{
public:
	ReverbRefLevel2(Reverb *reverb, int x, int y);
	~ReverbRefLevel2();
	int handle_event();
	Reverb *reverb;
};

class ReverbRefTotal : public BC_IPot
{
public:
	ReverbRefTotal(Reverb *reverb, int x, int y);
	~ReverbRefTotal();
	int handle_event();
	Reverb *reverb;
};

class ReverbRefLength : public BC_IPot
{
public:
	ReverbRefLength(Reverb *reverb, int x, int y);
	~ReverbRefLength();
	int handle_event();
	Reverb *reverb;
};

class ReverbLowPass1 : public BC_QPot
{
public:
	ReverbLowPass1(Reverb *reverb, int x, int y);
	~ReverbLowPass1();
	int handle_event();
	Reverb *reverb;
};

class ReverbLowPass2 : public BC_QPot
{
public:
	ReverbLowPass2(Reverb *reverb, int x, int y);
	~ReverbLowPass2();
	int handle_event();
	Reverb *reverb;
};


class ReverbLoad;
class ReverbSave;

class ReverbLoadPrev;
class ReverbLoadPrevThread;

class ReverbMenu : public BC_MenuBar
{
public:
	ReverbMenu(Reverb *reverb, ReverbWindow *window);
	~ReverbMenu();
	
	int create_objects(BC_Hash *defaults);
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);
// most recent loads
	int add_load(char *path);
	ReverbLoadPrevThread *prev_load_thread;
	
	int total_loads;
	BC_Menu *filemenu;
	ReverbWindow *window;
	Reverb *reverb;
	ReverbLoad *load;
	ReverbSave *save;
	ReverbLoadPrev *prev_load[TOTAL_LOADS];
};

class ReverbSaveThread;
class ReverbLoadThread;

class ReverbLoad : public BC_MenuItem
{
public:
	ReverbLoad(Reverb *reverb, ReverbMenu *menu);
	~ReverbLoad();
	int handle_event();
	Reverb *reverb;
	ReverbLoadThread *thread;
	ReverbMenu *menu;
};

class ReverbSave : public BC_MenuItem
{
public:
	ReverbSave(Reverb *reverb, ReverbMenu *menu);
	~ReverbSave();
	int handle_event();
	Reverb *reverb;
	ReverbSaveThread *thread;
	ReverbMenu *menu;
};


class ReverbLoadPrev : public BC_MenuItem
{
public:
	ReverbLoadPrev(Reverb *reverb, ReverbMenu *menu, char *filename, char *path);
	ReverbLoadPrev(Reverb *reverb, ReverbMenu *menu);
	int handle_event();
	int set_path(char *path);
	char path[1024];
	Reverb *reverb;
	ReverbMenu *menu;
};


class ReverbLoadPrevThread : public Thread
{
public:
	ReverbLoadPrevThread(Reverb *reverb, ReverbMenu *menu);
	~ReverbLoadPrevThread();
	void run();
	int set_path(char *path);
	char path[1024];
	Reverb *reverb;
	ReverbMenu *menu;
};



class ReverbSaveThread : public Thread
{
public:
	ReverbSaveThread(Reverb *reverb, ReverbMenu *menu);
	~ReverbSaveThread();
	void run();
	Reverb *reverb;
	ReverbMenu *menu;
};

class ReverbSaveDialog : public BC_FileBox
{
public:
	ReverbSaveDialog(Reverb *reverb);
	~ReverbSaveDialog();

	int ok_event();
	int cancel_event();
	Reverb *reverb;
};


class ReverbLoadThread : public Thread
{
public:
	ReverbLoadThread(Reverb *reverb, ReverbMenu *menu);
	~ReverbLoadThread();
	void run();
	Reverb *reverb;
	ReverbMenu *menu;
};

class ReverbLoadDialog : public BC_FileBox
{
public:
	ReverbLoadDialog(Reverb *reverb);
	~ReverbLoadDialog();

	int ok_event();
	int cancel_event();
	Reverb *reverb;
};

#endif
