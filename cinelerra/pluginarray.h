#ifndef PLUGINARRAY_H
#define PLUGINARRAY_H


#include "arraylist.h"
#include "cache.inc"
#include "edl.inc"
#include "file.inc"
#include "keyframe.inc"
#include "module.inc"
#include "mwindow.inc"
#include "pluginserver.inc"
#include "recordableatracks.inc"
#include "track.inc"


#include <stdint.h>
// The plugin array does the real work of a non realtime effect.


class PluginArray : public ArrayList<PluginServer*>
{
public:
	PluginArray();
	virtual ~PluginArray();

	PluginServer* scan_plugindb(char *title);
	int start_plugins(MWindow *mwindow, 
		EDL *edl, 
		PluginServer *plugin_server, 
		KeyFrame *keyframe,
		int64_t start,
		int64_t end,
		File *file);
	int run_plugins();
	int stop_plugins();
	virtual void create_modules() {};
	virtual void create_buffers() {};
	virtual void load_module(int module, int64_t input_position, int64_t len) {};
	virtual void process_realtime(int module, int64_t input_position, int64_t len) {};
	virtual int process_loop(int module, int64_t &write_length) {};
	virtual int write_buffers(int64_t len) { return 0; };
	virtual int64_t get_bufsize() { return 0; };
	virtual int total_tracks() { return 0; };
	virtual void get_recordable_tracks() {};
	virtual Track* track_number(int number) { return 0; };
	virtual int write_samples_derived(int64_t samples_written) { return 0; };
	virtual int write_frames_derived(int64_t frames_written) { return 0; };
	virtual int start_plugins_derived() { return 0; };
	virtual int start_realtime_plugins_derived() { return 0; };
	virtual int stop_plugins_derived() { return 0; };
	virtual int render_track(int track, int64_t fragment_len, int64_t position) { return 0; };

	Module **modules;
	MWindow *mwindow;
	CICache *cache;
	EDL *edl;
	PluginServer *plugin_server;
	KeyFrame *keyframe;
// output file
	File *file;             
	int64_t buffer_size;  
// Start and end of segment in units
	int64_t start, end;
	int done;
	int error;
};



#endif
