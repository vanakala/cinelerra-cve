#ifndef PACKAGERENDERER_H
#define PACKAGERENDERER_H


#include "assets.inc"
#include "bcwindowbase.inc"
#include "cache.inc"
#include "edit.inc"
#include "edl.inc"
#include "file.inc"
#include "mwindow.inc"
#include "playabletracks.inc"
#include "playbackconfig.inc"
#include "pluginserver.inc"
#include "preferences.inc"
#include "renderengine.inc"
#include "track.inc"
#include "transportque.inc"
#include "videodevice.inc"


class RenderPackage
{
public:
	RenderPackage();
	~RenderPackage();

// Path of output without remote prefix
	char path[BCTEXTLEN];

// Range not including preroll
	long audio_start;
	long audio_end;
	long video_start;
	long video_end;
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
		long current_position, 
		Track* playable_track,  // The one track which is playable
		Edit* &playable_edit, // The edit which is playing
		File *file);   // Output file
	int direct_frame_copy(EDL *edl, 
		long &video_position, 
		File *file,
		int &result);

// Get result status from server
	virtual int get_result();
	virtual void set_result(int value);
	virtual void set_progress(long total_samples);
// Used by background rendering
	virtual void set_video_map(long position, int value);
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
	long audio_position;
	long audio_preroll;
	long audio_read_length;
	File *file;
	int result;
	VFrame ***video_output;
// A nonzero mwindow signals master render engine to the engine.
	MWindow *mwindow;
	double *audio_output_ptr[MAX_CHANNELS];
	CICache *audio_cache;
	CICache *video_cache;
	VFrame *compressed_output;
	VideoOutConfig *vconfig;
//	PlaybackConfig *playback_config;
	PlayableTracks *playable_tracks;
	RenderEngine *render_engine;
	RenderPackage *package;
	TransportCommand *command;
	int direct_frame_copying;
	VideoDevice *video_device;
	VFrame *video_output_ptr[MAX_CHANNELS];
	long video_preroll;
	long video_position;
	long video_read_length;
	long video_write_length;
	long video_write_position;
};




#endif
