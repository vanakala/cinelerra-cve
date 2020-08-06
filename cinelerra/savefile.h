// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef SAVEFILE_H
#define SAVEFILE_H

#include "bcmenuitem.h"
#include "bcfilebox.h"
#include "mainmenu.inc"
#include "savefile.inc"
#include "thread.h"

class SaveBackup : public BC_MenuItem
{
public:
	SaveBackup();

	int handle_event();
};

class Save : public BC_MenuItem
{
public:
	Save();

	int handle_event();
	void set_saveas(SaveAs *saveas);
	void save_before_quit();

	int quit_now;
	SaveAs *saveas;
};

class SaveAs : public BC_MenuItem, public Thread
{
public:
	SaveAs();

	void set_mainmenu(MainMenu *mmenu);
	int handle_event();
	void run();

	int quit_now;
	MainMenu *mmenu;
};

class SaveFileWindow : public BC_FileBox
{
public:
	SaveFileWindow(int absx, int absy, const char *init_directory);
};

#endif
