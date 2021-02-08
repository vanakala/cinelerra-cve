// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "confirmsave.h"
#include "bchash.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "glthread.h"
#include "edl.h"
#include "mainerror.h"
#include "file.h"
#include "filexml.h"
#include "language.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "savefile.h"
#include "mainsession.h"
#include "theme.h"

#include <string.h>


SaveBackup::SaveBackup()
 : BC_MenuItem(_("Save backup"))
{
}

int SaveBackup::handle_event()
{
	mwindow_global->save_backup(1);
	mwindow_global->show_message(_("Saved backup."));
	return 1;
}


Save::Save()
 : BC_MenuItem(_("Save"), "s", 's')
{ 
	quit_now = 0; 
}

void Save::set_saveas(SaveAs *saveas)
{
	this->saveas = saveas;
}

int Save::handle_event()
{
	if(mainsession->filename[0] == 0)
	{
		saveas->start();
	}
	else
	{
		FileXML file;
		master_edl->save_xml(&file,
			mainsession->filename,
			0,
			0);

		if(file.write_to_file(mainsession->filename))
		{
			errorbox(_("Couldn't open %s"), mainsession->filename);
			return 1;
		}
		else
			mwindow_global->show_message(_("\"%s\" %dC written"), mainsession->filename, strlen(file.string));

		mainsession->changes_made = 0;
		if(saveas->quit_now)
			mwindow_global->glthread->quit();
	}
	return 1;
}

void Save::save_before_quit()
{
	saveas->quit_now = 1;
	handle_event();
}

SaveAs::SaveAs()
 : BC_MenuItem(_("Save as..."), ""), Thread()
{ 
	quit_now = 0;
}

void SaveAs::set_mainmenu(MainMenu *mmenu)
{
	this->mmenu = mmenu;
}

int SaveAs::handle_event() 
{ 
	quit_now = 0;
	start();
	return 1;
}

void SaveAs::run()
{
// ======================================= get path from user
	int result;
	char filename[BCTEXTLEN];
	int cx, cy;
	char *nameptr;

// Loop if file exists
	do{
		SaveFileWindow *window;

		mwindow_global->get_abs_cursor_pos(&cx, &cy);
		if(mainsession->filename[0])
			nameptr = mainsession->filename;
		else
		{
			filename[0] = 0;
			mwindow_global->defaults->get("DEFAULT_LOADPATH", filename);
			if(nameptr = strrchr(filename, '/'))
				*nameptr = 0;
			nameptr = filename;
		}
		window = new SaveFileWindow(cx, cy, nameptr);
		result = window->run_window();
		mwindow_global->defaults->delete_key("DIRECTORY");
		strcpy(filename, window->get_submitted_path());
		delete window;
// Extend the filename with .xml
		if(strlen(filename) < 4 || 
				strcasecmp(&filename[strlen(filename) - 4], ".xml"))
			strcat(filename, ".xml");

// ======================================= try to save it
		if(filename[0] == 0)
			return;              // no filename given
		if(result == 1)
			return;          // user cancelled
		result = ConfirmSave::test_file(filename);
	}while(result);        // file exists so repeat

// save it
	FileXML file;
	mwindow_global->set_filename(filename);      // update the project name
	strcpy(master_edl->project_path, filename);
	master_edl->save_xml(&file, filename, 0, 0);

	if(file.write_to_file(filename))
	{
		mwindow_global->set_filename(0);      // update the project name
		errorbox(_("Couldn't open %s."), filename);
		return;
	}
	else
		mwindow_global->show_message(_("\"%s\" %dC written"), filename, strlen(file.string));

	mainsession->changes_made = 0;
	mmenu->add_load(filename);
	if(quit_now)
		mwindow_global->glthread->quit();
	return;
}


SaveFileWindow::SaveFileWindow(int absx, int absy,
	const char *init_directory)
 : BC_FileBox(absx,
	absy - BC_WindowBase::get_resources()->filebox_h / 2,
	init_directory, 
	MWindow::create_title(N_("Save")),
	_("Enter a filename to save as"))
{ 
	set_icon(mwindow_global->get_window_icon());
}
