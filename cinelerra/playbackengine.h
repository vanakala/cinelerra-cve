#ifndef PLAYBACKENGINE_H
#define PLAYBACKENGINE_H

#include "arraylist.h"
#include "audiodevice.inc"
#include "cache.inc"
#include "canvas.inc"
#include "channeldb.inc"
#include "condition.inc"
#include "defaults.inc"
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
#include "videodevice.inc"

class PlaybackEngine : public Thread
{
public:
	PlaybackEngine(MWindow *mwindow, Canvas *output);
	virtual ~PlaybackEngine();

	int create_objects();
	virtual int create_render_engines();
	void delete_render_engines();
	void arm_render_engines();
	void start_render_engines();
	void wait_render_engines();
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
// Render engine set
	ArrayList<RenderEngine*> render_engines;













	int reset_parameters();

// ================= position information ======================
// get exact position in samples corrected for speed and direction
	long get_position(int sync_time = 1);
// get total samples rendered since last start with no speed correction
// Sync_time uses the video thread to get the current position.
// Otherwise a timer or audio device is used.
	long absolute_position(int sync_time = 1);

// stop and start for user changes
	int start_reconfigure();
	int stop_reconfigure();

// generic render routine
	int render_audio();
	int reset_buttons();

// ============================ cursor
	int lock_playback_cursor();
	int unlock_playback_cursor();
	int lock_playback_movement();
	int unlock_playback_movement();
	int move_right(long distance);

	Tracking *cursor;


// ============================== playback config
	int infinite;        // for infinite playback	
	int follow_loop;     // 1 if mwindow's looping setting is followed
	int is_playing_back;    // 0 - no playback  1 - armed but stopped  2 - playing back
	int reconfigure_status;    // 0 - no playback  1 - armed but stopped  2 - playing back
							      // otherwise use DSP chip if audio tracks are being played
	int update_button;            // flag for thread on exit
	int use_buttons;              // flag to update buttons when done or not
	long last_position;         // starting position for restarting playback
	long playback_start, playback_end;  // range for current playback
	int reverse;                // direction of current playback
	float speed;                // speed of current playback.  A speed of FRAME_SPEED causes frame advance
	long playback_buffer;
	long audio_module_fragment;

// ================================== audio config
//	long input_length;        // number of samples to read from disk at a time
//						      // multiple of playback_buffer greater than read_buffer
//	long output_length;       // # of samples to write to device adjusted for speed
	int shared_audio;    // for duplex audio recording

// ================================== video config

	long correction_factor;   // number of frames to skip to get synchronized

	RenderEngine *render_engine;
	AudioDevice *audio;
	VideoDevice *video;

private:
	int init_parameters();
	int init_audio_device();
	int init_video_device();
	Timer timer;    // timer for position information
	long loop_adjustment;       // for factoring loops into current position
};





#endif
