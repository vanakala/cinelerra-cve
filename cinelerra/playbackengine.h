#ifndef PLAYBACKENGINE_H
#define PLAYBACKENGINE_H

#include "arraylist.h"
#include "audiodevice.inc"
#include "cache.inc"
#include "canvas.inc"
#include "channeldb.inc"
#include "condition.inc"
#include "bchash.inc"
#include "edl.inc"
#include "mwindow.inc"
#include "maxchannels.h"
#include "mutex.inc"
#include "tracking.inc"
#include "preferences.inc"
#include "renderengine.inc"
#include "thread.h"
#include "bctimer.h"
#include "transportque.inc"

class PlaybackEngine : public Thread
{
public:
	PlaybackEngine(MWindow *mwindow, Canvas *output);
	virtual ~PlaybackEngine();

	int create_objects();
	virtual int create_render_engine();
	void delete_render_engine();
	void arm_render_engine();
	void start_render_engine();
	void wait_render_engine();
	void create_cache();
	void perform_change();
	void sync_parameters(EDL *edl);
// Set wait_tracking for events that change the cursor location but
// be sure to unlock the windows
	void interrupt_playback(int wait_tracking = 0);
// Get levels for tracking.  Return 0 if no audio.
	int get_output_levels(double *levels, long position);
	int get_module_levels(ArrayList<double> *module_levels, long position);
// The MWindow starts the playback cursor loop
// The other windows start a slider loop
// For pausing only the cursor is run
	virtual void init_cursor();
	virtual void stop_cursor();
	virtual int brender_available(long position);
// For normal playback tracking and the cursor are started
	virtual void init_tracking();
	virtual void stop_tracking();
// The playback cursor calls this to calculate the current tracking position
	virtual double get_tracking_position();
// Reset the transport after completion
	virtual void update_transport(int command, int paused);
// The render engines call this to update tracking variables in the playback engine.
	void update_tracking(double position);
// Get the output channel table for the current device or 0 if none exists.
	ChannelDB* get_channeldb();

	void run();

// Maintain caches through console changes
	CICache *audio_cache, *video_cache;
// Maintain playback cursor on GUI
	int tracking_active;
// Tracking variables updated by render engines
	double tracking_position;
// Not accurate until the first update_tracking, at which time
// tracking_active is incremented to 2.
	Timer tracking_timer;
// Lock access to tracking data
	Mutex *tracking_lock;
// Block returns until tracking loop is finished
	Condition *tracking_done;
// Pause the main loop for the PAUSE command
	Condition *pause_lock;
// Wait until thread has started
	Condition *start_lock;

	MWindow *mwindow;
	Canvas *output;
// Copy of main preferences
	Preferences *preferences;
// Next command
	TransportQue *que;
// Currently executing command
	TransportCommand *command;
// Last command which affected transport
	int last_command;
	int done;
	int do_cwindow;
// Render engine
	RenderEngine *render_engine;

// Used by label commands to get current position
	int is_playing_back;

// General purpose debugging register
	int debug;
};





#endif
