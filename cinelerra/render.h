#ifndef RENDER_H
#define RENDER_H


#include "assets.inc"
#include "bitspopup.h"
#include "browsebutton.h"
#include "cache.inc"
#include "compresspopup.h"
#include "defaults.inc"
#include "edit.inc"
#include "errorbox.inc"
#include "file.inc"
#include "formatpopup.inc"
#include "formattools.inc"
#include "guicast.h"
#include "loadmode.inc"
#include "mainprogress.inc"
#include "maxchannels.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "packagedispatcher.inc"
#include "packagerenderer.h"
#include "playabletracks.inc"
#include "preferences.inc"
#include "bcprogressbox.inc"
#include "render.inc"
#include "track.inc"
#include "transportque.inc"
#include "vframe.inc"

class RenderItem : public BC_MenuItem
{
public:
	RenderItem(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class RenderProgress : public Thread
{
public:
	RenderProgress(MWindow *mwindow, Render *render);
	~RenderProgress();
	
	void run();
	
	
	MWindow *mwindow;
	Render *render;
	
	int64_t last_value;
};



class MainPackageRenderer : public PackageRenderer
{
public:
	MainPackageRenderer(Render *render);
	~MainPackageRenderer();
	
	
	int get_result();
	void set_result(int value);
	void set_progress(int64_t value);
	int progress_cancelled();

	Render *render;
};




class Render : public Thread
{
public:
	Render(MWindow *mwindow);
	~Render();

	void run();
	int load_defaults(Asset *asset);
	int save_defaults(Asset *asset);
// force asset parameters regardless of window
// This should be integrated into the Asset Class.
	static int check_asset(EDL *edl, Asset &asset); 
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
	static int test_existence(MWindow *mwindow, Asset *asset);
	int direct_frame_copy(EDL *edl, int64_t &render_video_position, File *file);
	int direct_copy_possible(EDL *edl, 
			int64_t current_position, 
			Track* playable_track,  // The one track which is playable
			Edit* &playable_edit, // The edit which is playing
			File *file);   // Output file

	void start_progress();
	void stop_progress();

	int load_mode;
	int in_progress;
// Background compression must be disabled when direct frame copying and reenabled afterwards
	int direct_frame_copying;

	CICache *audio_cache, *video_cache;
	VFrame *compressed_output;
	MainProgressBar *progress;
	RenderProgress *render_progress;
	MWindow *mwindow;
	PlayableTracks *playable_tracks;
	PackageDispatcher *packages;
	Mutex *package_lock, *counter_lock;
	Preferences *preferences;
	int strategy;
// Total selection to render in seconds
	double total_start, total_end;
// External Render farm checks this every frame.
	int result;
	Asset *default_asset;
	TransportCommand *command;

// Total samples updated by the render farm and the local renderer.
// This avoids rounding errors and complies with the use of samples for
// timing.
	int64_t total_rendered;
// Speed for the master node
	double frames_per_second;
};

class RenderToTracks;

class RenderWindow : public BC_Window
{
public:
	RenderWindow(MWindow *mwindow, Render *render, Asset *asset);
	~RenderWindow();

	int create_objects();


	LoadMode *loadmode;
	FormatTools *format_tools;

	MWindow *mwindow;
	Render *render;
	Asset *asset;
};













#endif
