// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef RENDER_H
#define RENDER_H

#include "asset.inc"
#include "batchrender.inc"
#include "bcmenuitem.h"
#include "browsebutton.h"
#include "condition.inc"
#include "formattools.inc"
#include "loadmode.inc"
#include "mainprogress.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "packagedispatcher.inc"
#include "packagerenderer.h"
#include "preferences.inc"
#include "render.inc"
#include "track.inc"
#include "transportcommand.inc"
#include "vframe.inc"
#include "renderprofiles.inc"


class RenderItem : public BC_MenuItem
{
public:
	RenderItem();

	int handle_event();
};


class RenderProgress : public Thread
{
public:
	RenderProgress(Render *render);
	~RenderProgress();

	void run();

	Render *render;

	framenum last_value;
};


class MainPackageRenderer : public PackageRenderer
{
public:
	MainPackageRenderer(Render *render);

	int get_master();
	int get_result();
	void set_result(int value);
	void set_progress(ptstime value);
	int progress_cancelled();

	Render *render;
};

class RenderWindow;


class Render : public Thread
{
public:
	Render();
	~Render();

// Start dialogue box and render interactively.
	void start_interactive();
// Start batch rendering jobs.
// A new thread is created and the rendering is interactive.
	void start_batches(ArrayList<BatchRenderJob*> *jobs);
// The batches are processed in the foreground in non interactive mode.
	void run_batches(ArrayList<BatchRenderJob*> *jobs);
// Called by BatchRender to stop the operation.
	void stop_operation();
// Render menueffects
	void run_menueffects(Asset *asset, EDL *edl,
		int strategy, int range_type, int load_mode);

	void run();

// Render single job.  Used by run.
	int render(int test_overwrite, 
		Asset *asset,
		EDL *edl,
		int strategy,
		int range_type);

	void load_defaults(Asset *asset);
	void save_defaults(Asset *asset);
	void load_profile(Asset *asset);
// force asset parameters regardless of window
// This should be integrated into the Asset Class.
	static int check_asset(EDL *edl, Asset &asset); 
// Fix strategy to conform with using renderfarm.
	static int fix_strategy(int strategy, int use_renderfarm);
// Force filename to have a 0 padded number if rendering to a list.
	int check_numbering(Asset &asset);
	static void create_filename(char *path, 
		char *default_path, 
		int current_number,
		int total_digits,
		int number_start);
	static void get_starting_number(char *path, 
		int &current_number,
		int &number_start, 
		int &total_digits,
		int min_digits = 3);
	void start_progress();
	void stop_progress();

// Procedure the run function should use.
	int mode;
	enum
	{
		NONE,
		INTERACTIVE,
		BATCH,
		EFFECT
	};
// When batch rendering is cancelled from the batch dialog
	int batch_cancelled;


	int load_mode;
	int in_progress;
// Background compression must be disabled when direct frame copying and reenabled afterwards
	int direct_frame_copying;

	VFrame *compressed_output;
	MainProgressBar *progress;
	RenderProgress *render_progress;
	PackageDispatcher *packages;
	Mutex *package_lock, *counter_lock;
	int strategy;
	int range_type;
// Total selection to render in seconds
	ptstime total_start, total_end;
// External Render farm checks this every frame.
	int result;
	Asset *default_asset;
	TransportCommand *command;
// Jobs pointer passed to start_batches
	ArrayList<BatchRenderJob*> *jobs;
// Used by batch rendering to wait until rendering is finished
	Condition *completion;

// Total time updated by the render farm and the local renderer.
	ptstime total_rendered;
// Speed for the master node
	double frames_per_second;
// Time used in last render
	double elapsed_time;

// Current open RenderWindow
	RenderWindow *render_window;

// For non interactive mode, maintain progress here.
	ptstime progress_max;
	Timer *progress_timer;
	int64_t last_eta;
};

class RenderToTracks;

class RenderRangeProject : public BC_Radial
{
public:
	RenderRangeProject(RenderWindow *rwindow, int value, int x, int y);
	int handle_event();
	RenderWindow *rwindow;
};


class RenderRangeSelection : public BC_Radial
{
public:
	RenderRangeSelection(RenderWindow *rwindow, int value, int x, int y);
	int handle_event();
	RenderWindow *rwindow;
};


class RenderRangeInOut : public BC_Radial
{
public:
	RenderRangeInOut(RenderWindow *rwindow, int value, int x, int y);
	int handle_event();
	RenderWindow *rwindow;
};


class RenderWindow : public BC_Window
{
public:
	RenderWindow(Render *render, Asset *asset);
	~RenderWindow();

	void update_range_type(int range_type);
	void load_profile();

	RenderRangeProject *rangeproject;
	RenderRangeSelection *rangeselection;
	RenderRangeInOut *rangeinout;

	RenderProfile *renderprofile;

	LoadMode *loadmode;
	FormatTools *format_tools;

	Render *render;
	Asset *asset;
};

#endif
