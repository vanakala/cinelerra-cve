#ifndef VPLUGINARRAY_H
#define VPLUGINARRAY_H

#include "edl.inc"
#include "pluginarray.h"
#include "pluginserver.inc"
#include "recordablevtracks.inc"
#include "track.inc"
#include "vframe.inc"
#include "vmodule.inc"

class VPluginArray : public PluginArray
{
public:
	VPluginArray();
	~VPluginArray();

	long get_bufsize();
	void create_buffers();
	void create_modules();
	void load_module(int module, long input_position, long len);
	void process_realtime(int module, long input_position, long len);
	int process_loop(int module, long &write_length);
	int write_buffers(long len);
	int total_tracks();
	void get_recordable_tracks();
	Track* track_number(int number);

	RecordableVTracks *tracks;
// fake buffer for plugin output
	VFrame ***buffer;
// Buffer for reading and writing to file
	VFrame ***realtime_buffers;
};

#endif
