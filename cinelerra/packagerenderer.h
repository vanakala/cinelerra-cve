#ifndef PACKAGERENDERER_H
#define PACKAGERENDERER_H


#include "assets.inc"
#include "bcwindowbase.inc"
#include "cache.inc"
#include "edit.inc"
#include "edl.inc"
#include "file.inc"
#include "maxchannels.h"
#include "mwindow.inc"
#include "playabletracks.inc"
#include "playbackconfig.inc"
#include "pluginserver.inc"
#include "preferences.inc"
#include "renderengine.inc"
#include "track.inc"
#include "transportque.inc"
#include "vframe.inc"
#include "videodevice.inc"


#include <stdint.h>

class RenderPackage
{
public:
	RenderPackage();
	~RenderPackage();

// Path of output without remote prefix
	char path[BCTEXTLEN];

// Range not including preroll
	int64_t audio_start;
	int64_t audio_end;
	int64_t video_start;
	int64_t video_end;
	int done;
	int use_brender;
};




// Used by Render and BRender to do packages.
class PackageRenderer
{
public:
	PackageRenderer();
	~PackageRenderer();

// Initialize stuff which is reused between packages
	int initialize(MWindow *mwindow,
		EDL *edl, 
		Preferences *preferences, 
		Asset *default_asset,
		ArrayList<PluginServer*> *plugindb);

// Aborts and returns 1 if an error is encountered.
	int render_package(RenderPackage *package);

	int direct_copy_possible(EDL *edl,
		int64_t current_position, 
		Track* playable_track,  // The one track which is playable
		Edit* &playable_edit, // The edit which is playing
		File *file);   // Output file
	int direct_frame_copy(EDL *edl, 
		int64_t &video_position, 
		File *file,
		int &result);

// Get result status from server
	virtual int get_result();
	virtual void set_result(int value);
	virtual void set_progress(int64_t total_samples);
// Used by background rendering
	virtual void set_video_map(int64_t position, int value);
	virtual int progress_cancelled();

	void create_output();
	void create_engine();
	void do_audio();
	void do_video();
	void stop_engine();
	void stop_output();
	void close_output();


// Passed in from outside
	EDL *edl;
	Preferences *preferences;
	Asset *default_asset;
	ArrayList<PluginServer*> *plugindb;

// Created locally
	Asset *asset;
	double **audio_output;
	int64_t audio_position;
	int64_t audio_preroll;
	int64_t audio_read_length;
	File *file;
	int result;
	VFrame ***video_output;
// A nonzero mwindow signals master render engine to the engine.
// A zero mwindow signals client or non interactive.
	MWindow *mwindow;
	double *audio_output_ptr[MAX_CHANNELS];
	CICache *audio_cache;
	CICache *video_cache;
	VFrame *compressed_output;
	AudioOutConfig *aconfig;
	VideoOutConfig *vconfig;
//	PlaybackConfig *playback_config;
	PlayableTracks *playable_tracks;
	RenderEngine *render_engine;
	RenderPackage *package;
	TransportCommand *command;
	int direct_frame_copying;
	VideoDevice *video_device;
	VFrame *video_output_ptr[MAX_CHANNELS];
	int64_t video_preroll;
	int64_t video_position;
	int64_t video_read_length;
	int64_t video_write_length;
	int64_t video_write_position;
};




#endif
