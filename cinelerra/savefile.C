
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

#include "confirmsave.h"
#include "bchash.h"
#include "bcresources.h"
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


SaveBackup::SaveBackup(MWindow *mwindow)
 : BC_MenuItem(_("Save backup"))
{
	this->mwindow = mwindow;
}

int SaveBackup::handle_event()
{
	mwindow->save_backup();
	mwindow->gui->show_message(_("Saved backup."));
	return 1;
}


Save::Save(MWindow *mwindow) : BC_MenuItem(_("Save"), "s", 's')
{ 
	this->mwindow = mwindow; 
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
// save it
// TODO: Move this into mwindow.
		FileXML file;
		master_edl->save_xml(&file,
			mainsession->filename,
			0,
			0);
		file.terminate_string();

		if(file.write_to_file(mainsession->filename))
		{
			errorbox(_("Couldn't open %s"), mainsession->filename);
			return 1;
		}
		else
			mwindow->gui->show_message(_("\"%s\" %dC written"), mainsession->filename, strlen(file.string));

		mainsession->changes_made = 0;
		if(saveas->quit_now)
			mwindow->glthread->quit();
	}
	return 1;
}

void Save::save_before_quit()
{
	saveas->quit_now = 1;
	handle_event();
}

SaveAs::SaveAs(MWindow *mwindow)
 : BC_MenuItem(_("Save as..."), ""), Thread()
{ 
	this->mwindow = mwindow; 
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
	char directory[1024], filename[1024];
	strcpy(directory, "~");
	mwindow->defaults->get("DIRECTORY", directory);

// Loop if file exists
	do{
		SaveFileWindow *window;

		window = new SaveFileWindow(mwindow, directory);
		result = window->run_window();
		mwindow->defaults->update("DIRECTORY", window->get_submitted_path());
		strcpy(filename, window->get_submitted_path());
		delete window;

// Extend the filename with .xml
		if(strlen(filename) < 4 || 
			strcasecmp(&filename[strlen(filename) - 4], ".xml"))
		{
			strcat(filename, ".xml");
		}

// ======================================= try to save it
		if(filename[0] == 0) return;              // no filename given
		if(result == 1) return;          // user cancelled
		result = ConfirmSave::test_file(filename);
	}while(result);        // file exists so repeat

// save it
	FileXML file;
	mwindow->set_filename(filename);      // update the project name
	master_edl->save_xml(&file, filename, 0, 0);
	file.terminate_string();

	if(file.write_to_file(filename))
	{
		mwindow->set_filename("");      // update the project name
		errorbox(_("Couldn't open %s."), filename);
		return;
	}
	else
		mwindow->gui->show_message(_("\"%s\" %dC written"), filename, strlen(file.string));

	mainsession->changes_made = 0;
	mmenu->add_load(filename);
	if(quit_now)
		mwindow->glthread->quit();
	return;
}


SaveFileWindow::SaveFileWindow(MWindow *mwindow, char *init_directory)
 : BC_FileBox(mwindow->gui->get_abs_cursor_x(1),
	mwindow->gui->get_abs_cursor_y(1) - BC_WindowBase::get_resources()->filebox_h / 2,
	init_directory, 
	MWindow::create_title(N_("Save")),
	_("Enter a filename to save as"))
{ 
	this->mwindow = mwindow; 
	set_icon(mwindow->theme->get_image("mwindow_icon"));
}
