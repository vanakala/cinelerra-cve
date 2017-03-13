
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

#ifndef BATCHRENDER_H
#define BATCHRENDER_H

#include "arraylist.h"
#include "asset.inc"
#include "batchrender.inc"
#include "bcdialog.h"
#include "browsebutton.inc"
#include "filexml.inc"
#include "formattools.h"
#include "mwindow.inc"
#include "pluginserver.inc"
#include "preferences.inc"
#include "render.inc"

#define BATCHRENDER_COLUMNS 4


class BatchRenderMenuItem : public BC_MenuItem
{
public:
	BatchRenderMenuItem(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};


class BatchRenderJob
{
public:
	BatchRenderJob(Preferences *preferences, int jobnum);
	~BatchRenderJob();

	void copy_from(BatchRenderJob *src);
	void load(const char *profile_path);
	void save();
	void fix_strategy();
	void dump(int indent = 0);

// Source EDL to render
	char edl_path[BCTEXTLEN];
// Destination file for output
	Asset *asset;
	int strategy;
	int enabled;
	int jobnum;
// Amount of time elapsed in last render operation
	double elapsed;
	Preferences *preferences;
};


class BatchRenderThread : public BC_DialogThread
{
public:
	BatchRenderThread(MWindow *mwindow);
	BatchRenderThread();
	~BatchRenderThread();

	void handle_close_event(int result);
	BC_Window* new_gui();

	int test_edl_files();
	void calculate_dest_paths(ArrayList<char*> *paths,
		Preferences *preferences,
		ArrayList<PluginServer*> *plugindb);

// Load batch rendering jobs
	void load_jobs(Preferences *preferences);
// Not applicable to western civilizations
	void save_jobs();
	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);
	BatchRenderJob *merge_jobs(int jnum, Preferences *preferences);
	BatchRenderJob *new_job();
	void delete_job();
// Conditionally returns the job or the default job based on current_job
	BatchRenderJob* get_current_job();
	Asset* get_current_asset();
	char* get_current_edl();
// For command line usage
	void start_rendering(char *config_path, char *batch_path);
// For GUI usage
	void start_rendering();
	void stop_rendering();
// Highlight the currently rendering job.
	void update_active(int number);
	void update_done(int number, int create_list, double elapsed_time);
	void move_batch(int src, int dst);

	MWindow *mwindow;
	double current_start;
	double current_end;
	ArrayList<BatchRenderJob*> jobs;
	ArrayList<PluginServer*> *plugindb;

	BC_Hash *boot_defaults;
	Preferences *preferences;
	Render *render;

	BatchRenderGUI *gui;
	int column_width[BATCHRENDER_COLUMNS];
// job being edited
	int current_job;
// job being rendered
	int rendering_job;
	int is_rendering;
	char *profile_end;
	char profile_path[BCTEXTLEN];
};


class BatchRenderEDLPath : public BC_TextBox
{
public:
	BatchRenderEDLPath(BatchRenderThread *thread, 
		int x, 
		int y, 
		int w, 
		char *text);

	int handle_event();
	BatchRenderThread *thread;
};


class BatchRenderNew : public BC_GenericButton
{
public:
	BatchRenderNew(BatchRenderThread *thread, 
		int x, 
		int y);

	int handle_event();
	BatchRenderThread *thread;
};


class BatchRenderDelete : public BC_GenericButton
{
public:
	BatchRenderDelete(BatchRenderThread *thread, 
		int x, 
		int y);

	int handle_event();
	BatchRenderThread *thread;
};

class BatchRenderList : public BC_ListBox
{
public:
	BatchRenderList(BatchRenderThread *thread, 
		int x, 
		int y,
		int w,
		int h,
		ArrayList<BC_ListBoxItem*> *list_columns);

	int handle_event();
	void selection_changed();
	int column_resize_event();
	int drag_start_event();
	void drag_stop_event();
	int dragging_item;
	BatchRenderThread *thread;
};


class BatchRenderStart : public BC_GenericButton
{
public:
	BatchRenderStart(BatchRenderThread *thread, 
		int x, 
		int y);
	int handle_event();
	BatchRenderThread *thread;
};


class BatchRenderStop : public BC_GenericButton
{
public:
	BatchRenderStop(BatchRenderThread *thread, 
		int x, 
		int y);
	int handle_event();
	BatchRenderThread *thread;
};


class BatchRenderCancel : public BC_GenericButton
{
public:
	BatchRenderCancel(BatchRenderThread *thread, 
		int x, 
		int y);
	int handle_event();
	int keypress_event();
	BatchRenderThread *thread;
};


class BatchFormat : public FormatTools
{
public:
	BatchFormat(MWindow *mwindow,
			BatchRenderGUI *gui,
			Asset *asset,
			int &init_x,
			int &init_y,
			int support,
			int checkbox,
			int details,
			int *strategy);

	int handle_event();

	BatchRenderGUI *gui;
	MWindow *mwindow;
};


class BatchRenderGUI : public BC_Window
{
public:
	BatchRenderGUI(MWindow *mwindow, 
		BatchRenderThread *thread,
		int x,
		int y,
		int w,
		int h);
	~BatchRenderGUI();

	void resize_event(int w, int h);
	void translation_event();
	void close_event();
	void create_list(int update_widget);
	void change_job();

	ArrayList<BC_ListBoxItem*> list_columns[BATCHRENDER_COLUMNS];

	MWindow *mwindow;
	BatchRenderThread *thread;
	BC_Title *output_path_title;
	BatchFormat *format_tools;
	BrowseButton *edl_path_browse;
	BatchRenderEDLPath *edl_path_text;
	BC_Title *edl_path_title;
	BC_Title *list_title;
	BatchRenderNew *new_batch;
	BatchRenderDelete *delete_batch;
	BatchRenderList *batch_list;
	BatchRenderStart *start_button;
	BatchRenderStop *stop_button;
	BatchRenderCancel *cancel_button;
};

#endif
