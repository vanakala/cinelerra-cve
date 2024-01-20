// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bchash.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "edl.h"
#include "filesystem.h"
#include "language.h"
#include "loadfile.h"
#include "loadmode.h"
#include "mainmenu.h"
#include "mainundo.h"
#include "mainsession.h"
#include "mwindow.h"
#include "theme.h"

#include <string.h>

Load::Load(MainMenu *mainmenu)
 : BC_MenuItem(_("Load files..."), "o", 'o')
{ 
	this->mainmenu = mainmenu;
	thread = new LoadFileThread(this);
}

Load::~Load()
{
	delete thread;
}

int Load::handle_event() 
{
	if(!thread->running())
		thread->start();
	else if(thread->window)
		thread->window->raise_window();
	return 1;
}


LoadFileThread::LoadFileThread(Load *load)
 : Thread()
{
	this->load = load;
}

void LoadFileThread::run()
{
	int result;
	ArrayList<BC_ListBoxItem*> *dirlist;
	FileSystem fs;
	ArrayList<char*> path_list;
	path_list.set_array_delete();
	int cx, cy;
	LoadFileWindow *win;
	char default_path[BCTEXTLEN];

	strcpy(default_path, "~");
	mwindow_global->defaults->get("DEFAULT_LOADPATH", default_path);
	load_mode = mwindow_global->defaults->get("LOAD_MODE", LOADMODE_REPLACE);

	mwindow_global->get_abs_cursor_pos(&cx, &cy);
	win = window = new LoadFileWindow(this, cx, cy, default_path);
	result = window->run_window();

// Collect all selected files
	if(!result)
	{
		char *in_path, *out_path;
		int i = 0;

		while((in_path = window->get_path(i)))
		{
			int j;
			for(j = 0; j < path_list.total; j++)
			{
				if(!strcmp(in_path, path_list.values[j]))
					break;
			}

			if(j == path_list.total)
			{
				path_list.append(out_path = new char[strlen(in_path) + 1]);
				strcpy(out_path, in_path);
			}
			i++;
		}
	}

	mwindow_global->defaults->update("DEFAULT_LOADPATH",
		window->get_submitted_path());
	mwindow_global->defaults->update("LOAD_MODE", load_mode);

	window = 0;
	delete win;

// No file selected
	if(path_list.total == 0 || result == 1)
		return;

	mwindow_global->interrupt_indexes();
	mwindow_global->load_filenames(&path_list, load_mode);
	mwindow_global->add_load_path(path_list.values[0]);
	path_list.remove_all_objects();

	mwindow_global->save_backup();

	mwindow_global->restart_brender();

	if(load_mode == LOADMODE_REPLACE || load_mode == LOADMODE_REPLACE_CONCATENATE)
		mainsession->changes_made = 0;
	else
		mainsession->changes_made = 1;
}


LoadFileWindow::LoadFileWindow(LoadFileThread *thread, int absx, int absy,
	char *init_directory)
 : BC_FileBox(absx,
		absy - BC_WindowBase::get_resources()->filebox_h / 2,
		init_directory, 
		MWindow::create_title(N_("Load")),
		_("Select files to load:"), 
		0,
		0,
		1,
		mwindow_global->theme->loadfile_pad)
{
	int x = get_w() / 2 - 200;
	int y = get_cancel_button()->get_y() - 20;

	this->thread = thread;
	set_icon(mwindow_global->get_window_icon());
	loadmode = new LoadMode(this, x, y, &thread->load_mode, 0);
}

LoadFileWindow::~LoadFileWindow() 
{
	delete loadmode;
}

void LoadFileWindow::resize_event(int w, int h)
{
	int x = w / 2 - 200;
	int y = get_cancel_button()->get_y() - 20;
	draw_background(0, 0, w, h);

	loadmode->reposition_window(x, y);

	BC_FileBox::resize_event(w, h);
}


NewTimeline::NewTimeline(int x, int y, LoadFileWindow *window)
 : BC_Radial(x, 
	y, 
	window->thread->load_mode == LOADMODE_REPLACE,
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
	window->thread->load_mode == LOADMODE_REPLACE_CONCATENATE,
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
	window->thread->load_mode == LOADMODE_NEW_TRACKS,
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
	window->thread->load_mode == LOADMODE_CONCATENATE,
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
	window->thread->load_mode == LOADMODE_RESOURCESONLY,
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


LoadPrevious::LoadPrevious()
 : BC_MenuItem("")
{ 
}

int LoadPrevious::handle_event()
{
	ArrayList<char*> path_list;
	path_list.set_array_delete();
	char *out_path;

	path_list.append(out_path = new char[strlen(path) + 1]);
	strcpy(out_path, path);
	mwindow_global->load_filenames(&path_list, LOADMODE_REPLACE);
	mwindow_global->add_load_path(path_list.values[0]);
	path_list.remove_all_objects();

	mwindow_global->undo->update_undo(_("load previous"), LOAD_ALL);
	mwindow_global->save_backup();
	mainsession->changes_made = 0;
	return 1;
}

void LoadPrevious::set_path(const char *path)
{
	strcpy(this->path, path);
}


LoadBackup::LoadBackup()
 : BC_MenuItem(_("Load backup"))
{
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

	mwindow_global->load_filenames(&path_list, LOADMODE_REPLACE);
	path_list.remove_all_objects();
	mwindow_global->set_filename(master_edl->project_path);
	mwindow_global->undo->update_undo(_("load backup"), LOAD_ALL, 0, 0);
	mwindow_global->save_backup();
// We deliberately mark the project changed, because the backup is most likely
// not identical to the project file that it refers to.
	mainsession->changes_made = 1;

	return 1;
}
