
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

#include "assets.h"
#include "bchash.h"
#include "edl.h"
#include "errorbox.h"
#include "file.h"
#include "filesystem.h"
#include "indexfile.h"
#include "language.h"
#include "loadfile.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainundo.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"



#include <string.h>

Load::Load(MWindow *mwindow, MainMenu *mainmenu)
 : BC_MenuItem(_("Load files..."), "o", 'o')
{ 
	this->mwindow = mwindow;
	this->mainmenu = mainmenu;
}

Load::~Load()
{
	delete thread;
}

int Load::create_objects()
{
	thread = new LoadFileThread(mwindow, this);
	return 0;
}

int Load::handle_event() 
{
//printf("Load::handle_event 1\n");
	if(!thread->running())
	{
//printf("Load::handle_event 2\n");
		thread->start();
	}
	return 1;
}






LoadFileThread::LoadFileThread(MWindow *mwindow, Load *load)
 : Thread()
{
	this->mwindow = mwindow;
	this->load = load;
}

LoadFileThread::~LoadFileThread()
{
}

void LoadFileThread::run()
{
	int result;
	ArrayList<BC_ListBoxItem*> *dirlist;
	FileSystem fs;
	ArrayList<char*> path_list;
	path_list.set_array_delete();
	char default_path[BCTEXTLEN];
	char *reel_name = 0;
	int reel_number = 0;
	int overwrite_reel = 0;
	
	sprintf(default_path, "~");
	mwindow->defaults->get("DEFAULT_LOADPATH", default_path);
	load_mode = mwindow->defaults->get("LOAD_MODE", LOAD_REPLACE);

	{
		LoadFileWindow window(mwindow, this, default_path);
		window.create_objects();
		result = window.run_window();

		if ((!result) && (load_mode == LOAD_REPLACE)) {
			mwindow->set_filename(window.get_path(0));
		}

// Collect all selected files
		if(!result)
		{
			char *in_path, *out_path;
			int i = 0;

			while((in_path = window.get_path(i)))
			{
				int j;
				for(j = 0; j < path_list.total; j++)
				{
					if(!strcmp(in_path, path_list.values[j])) break;
				}
				
				if(j == path_list.total)
				{
					path_list.append(out_path = new char[strlen(in_path) + 1]);
					strcpy(out_path, in_path);
				}
				i++;
			}
		}

		mwindow->defaults->update("DEFAULT_LOADPATH", 
			window.get_submitted_path());
		mwindow->defaults->update("LOAD_MODE", 
			load_mode);
	}

// No file selected
	if(path_list.total == 0 || result == 1)
	{
		return;
	}

//	{
//		ReelWindow rwindow(mwindow);
//		rwindow.create_objects();
//		result = rwindow.run_window();

//		if(result)
//		{
//			return;
//		}
		
//		reel_name = rwindow.reel_name->get_text();
//		reel_number = atol(rwindow.reel_number->get_text());
//		overwrite_reel = rwindow.overwrite_reel;
//	}

	reel_name = "none";
	reel_number = 0;
	overwrite_reel = 0;

	mwindow->interrupt_indexes();
	mwindow->gui->lock_window("LoadFileThread::run");
	result = mwindow->load_filenames(&path_list, load_mode, 0, reel_name, reel_number, overwrite_reel);
	mwindow->gui->mainmenu->add_load(path_list.values[0]);
	mwindow->gui->unlock_window();
	path_list.remove_all_objects();


	mwindow->save_backup();

	mwindow->restart_brender();
//	mwindow->undo->update_undo(_("load"), LOAD_ALL);

	if(load_mode == LOAD_REPLACE || load_mode == LOAD_REPLACE_CONCATENATE)
		mwindow->session->changes_made = 0;
	else
		mwindow->session->changes_made = 1;

	return;
}








LoadFileWindow::LoadFileWindow(MWindow *mwindow, 
	LoadFileThread *thread,
	char *init_directory)
 : BC_FileBox(mwindow->gui->get_abs_cursor_x(1),
 		mwindow->gui->get_abs_cursor_y(1) - BC_WindowBase::get_resources()->filebox_h / 2,
		init_directory, 
		PROGRAM_NAME ": Load",
		_("Select files to load:"), 
		0,
		0,
		1,
		mwindow->theme->loadfile_pad)
{
	this->thread = thread;
	this->mwindow = mwindow; 
}

LoadFileWindow::~LoadFileWindow() 
{
	delete loadmode;
}

int LoadFileWindow::create_objects()
{
	BC_FileBox::create_objects();

	int x = get_w() / 2 - 200;
	int y = get_cancel_button()->get_y() - 50;
	loadmode = new LoadMode(mwindow, this, x, y, &thread->load_mode, 0);
	loadmode->create_objects();

	return 0;
}

int LoadFileWindow::resize_event(int w, int h)
{
	int x = w / 2 - 200;
	int y = get_cancel_button()->get_y() - 50;
	draw_background(0, 0, w, h);

	loadmode->reposition_window(x, y);

	return BC_FileBox::resize_event(w, h);
}







NewTimeline::NewTimeline(int x, int y, LoadFileWindow *window)
 : BC_Radial(x, 
 	y, 
	window->thread->load_mode == LOAD_REPLACE,
	_("Replace current project."))
{
	this->window = window;
}
int NewTimeline::handle_event()
{
	window->newtracks->set_value(0);
	window->newconcatenate->set_value(0);
	window->concatenate->set_value(0);
	window->resourcesonly->set_value(0);
	return 1;
}

NewConcatenate::NewConcatenate(int x, int y, LoadFileWindow *window)
 : BC_Radial(x, 
 	y, 
	window->thread->load_mode == LOAD_REPLACE_CONCATENATE,
	_("Replace current project and concatenate tracks."))
{
	this->window = window;
}
int NewConcatenate::handle_event()
{
	window->newtimeline->set_value(0);
	window->newtracks->set_value(0);
	window->concatenate->set_value(0);
	window->resourcesonly->set_value(0);
	return 1;
}

AppendNewTracks::AppendNewTracks(int x, int y, LoadFileWindow *window)
 : BC_Radial(x, 
 	y, 
	window->thread->load_mode == LOAD_NEW_TRACKS,
	_("Append in new tracks."))
{
	this->window = window;
}
int AppendNewTracks::handle_event()
{
	window->newtimeline->set_value(0);
	window->newconcatenate->set_value(0);
	window->concatenate->set_value(0);
	window->resourcesonly->set_value(0);
	return 1;
}

EndofTracks::EndofTracks(int x, int y, LoadFileWindow *window)
 : BC_Radial(x, 
 	y, 
	window->thread->load_mode == LOAD_CONCATENATE,
	_("Concatenate to existing tracks."))
{
	this->window = window;
}
int EndofTracks::handle_event()
{
	window->newtimeline->set_value(0);
	window->newconcatenate->set_value(0);
	window->newtracks->set_value(0);
	window->resourcesonly->set_value(0);
	return 1;
}

ResourcesOnly::ResourcesOnly(int x, int y, LoadFileWindow *window)
 : BC_Radial(x, 
 	y, 
	window->thread->load_mode == LOAD_RESOURCESONLY,
	_("Create new resources only."))
{
	this->window = window;
}
int ResourcesOnly::handle_event()
{
	set_value(1);
	window->newtimeline->set_value(0);
	window->newconcatenate->set_value(0);
	window->newtracks->set_value(0);
	window->concatenate->set_value(0);
	return 1;
}







LocateFileWindow::LocateFileWindow(MWindow *mwindow, 
	char *init_directory, 
	char *old_filename)
 : BC_FileBox(mwindow->gui->get_abs_cursor_x(1),
 		mwindow->gui->get_abs_cursor_y(1), 
		init_directory, 
		PROGRAM_NAME ": Locate file", 
		old_filename)
{ 
	this->mwindow = mwindow; 
}

LocateFileWindow::~LocateFileWindow()
{
}







LoadPrevious::LoadPrevious(MWindow *mwindow)
 : BC_MenuItem(""), Thread()
{ 
	this->mwindow = mwindow;
	this->loadfile = loadfile; 
}

int LoadPrevious::handle_event()
{
	ArrayList<char*> path_list;
	path_list.set_array_delete();
	char *out_path;
	int load_mode = mwindow->defaults->get("LOAD_MODE", LOAD_REPLACE);


	path_list.append(out_path = new char[strlen(path) + 1]);
	strcpy(out_path, path);
	mwindow->load_filenames(&path_list, LOAD_REPLACE);
	mwindow->gui->mainmenu->add_load(path_list.values[0]);
	path_list.remove_all_objects();


	mwindow->defaults->update("LOAD_MODE", load_mode);
	mwindow->undo->update_undo(_("load previous"), LOAD_ALL);
	mwindow->save_backup();
	mwindow->session->changes_made = 0;
	return 1;
}







void LoadPrevious::run()
{
//	loadfile->mwindow->load(path, loadfile->append);
}

int LoadPrevious::set_path(char *path)
{
	strcpy(this->path, path);
}








LoadBackup::LoadBackup(MWindow *mwindow)
 : BC_MenuItem(_("Load backup"))
{
	this->mwindow = mwindow;
}

int LoadBackup::handle_event()
{
	ArrayList<char*> path_list;
	path_list.set_array_delete();
	char *out_path;
	char string[BCTEXTLEN];
	strcpy(string, BACKUP_PATH);
	FileSystem fs;
	fs.complete_path(string);
	
	path_list.append(out_path = new char[strlen(string) + 1]);
	strcpy(out_path, string);
	
	mwindow->load_filenames(&path_list, LOAD_REPLACE, 0);
	mwindow->edl->local_session->clip_title[0] = 0;
// This is unique to backups since the path of the backup is different than the
// path of the project.
	mwindow->set_filename(mwindow->edl->project_path);
	path_list.remove_all_objects();
	mwindow->undo->update_undo(_("load backup"), LOAD_ALL, 0, 0);
	mwindow->save_backup();
// We deliberately mark the project changed, because the backup is most likely
// not identical to the project file that it refers to.
	mwindow->session->changes_made = 1;

	return 1;
}
	


// Dialog to set reel number/name

ReelWindow::ReelWindow(MWindow *mwindow)
 : BC_Window(_("Please enter the reel name and number"),
 	mwindow->gui->get_abs_cursor_x(1) - 375 / 2,
 	mwindow->gui->get_abs_cursor_y(1) - 150 / 2,
 	375,
 	150,
 	100,
 	100,
 	0,
 	0,
 	1)
{
	this->mwindow = mwindow;
	overwrite_reel = 0; // TODO: this should be loaded from previous time
}

ReelWindow::~ReelWindow()
{
	delete reel_name_title;
	delete reel_name;
	delete reel_number_title;
	delete reel_number;
	delete checkbox;
}

int ReelWindow::create_objects()
{
	int y = 10;
	int x = 0;

	add_subwindow(checkbox = new OverwriteReel(this, x, y, !overwrite_reel));	
	y += 40;
	
	x = 10;
	add_subwindow(reel_name_title = new BC_Title(x, y, _("Reel Name:")));
	x += reel_name_title->get_w() + 20;

	add_subwindow(reel_name = new BC_TextBox(x,
		y,
		250,
		1,
		"cin0000"));
	
	y += 30;
	
	x = 10;
	
	add_subwindow(reel_number_title = new BC_Title(x, y,
																	_("Reel Number:")));
	// line up the text boxes
	x += reel_name_title->get_w() + 20;

	add_subwindow(reel_number = new BC_TextBox(x,
		y,
		50,
		1,
		"00"));

	add_subwindow(ok_button = new BC_OKButton(this));
	
	add_subwindow(cancel_button = new BC_CancelButton(this));

// Disable reel_name and reel_number if the user doesn't want to overwrite
// (overwrite == accept default as well)
	if(!overwrite_reel)
	{
		reel_name->disable();
		reel_number->disable();
	}
	show_window();

	return 0;	
}

int ReelWindow::resize_event(int w, int h)
{
// Doesn't resize
	return 0;
}

OverwriteReel::OverwriteReel(ReelWindow *rwindow,
	int x, int y, int value)
 : BC_CheckBox(x, y, value, _("Use default or previous name and number"))
{
	this->rwindow = rwindow;
}

int OverwriteReel::handle_event()
{
	rwindow->overwrite_reel = !get_value();
// If the checkbox is not enabled, we want to enable the reel_name and
// reel_number text boxes
	if(!get_value())
	{
		rwindow->reel_name->enable();
		rwindow->reel_number->enable();
	}
	else
	{
		rwindow->reel_name->disable();
		rwindow->reel_number->disable();
	}
}
