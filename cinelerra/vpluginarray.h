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

	int64_t get_bufsize();
	void create_buffers();
	void create_modules();
	void load_module(int module, int64_t input_position, int64_t len);
	void process_realtime(int module, int64_t input_position, int64_t len);
	int process_loop(int module, int64_t &write_length);
	int write_buffers(int64_t len);
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
