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
		long start,
		long end,
		File *file);
	int run_plugins();
	int stop_plugins();
	virtual void create_modules() {};
	virtual void create_buffers() {};
	virtual void load_module(int module, long input_position, long len) {};
	virtual void process_realtime(int module, long input_position, long len) {};
	virtual int process_loop(int module, long &write_length) {};
	virtual int write_buffers(long len) { return 0; };
	virtual long get_bufsize() { return 0; };
	virtual int total_tracks() { return 0; };
	virtual void get_recordable_tracks() {};
	virtual Track* track_number(int number) { return 0; };
	virtual int write_samples_derived(long samples_written) { return 0; };
	virtual int write_frames_derived(long frames_written) { return 0; };
	virtual int start_plugins_derived() { return 0; };
	virtual int start_realtime_plugins_derived() { return 0; };
	virtual int stop_plugins_derived() { return 0; };
	virtual int render_track(int track, long fragment_len, long position) { return 0; };

	Module **modules;
	MWindow *mwindow;
	CICache *cache;
	EDL *edl;
	PluginServer *plugin_server;
	KeyFrame *keyframe;
// output file
	File *file;             
	long buffer_size;  
// Start and end of segment in units
	long start, end;
	int done;
	int error;
};



#endif
