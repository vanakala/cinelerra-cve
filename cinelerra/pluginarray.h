
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

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
	PluginArray(int data_type);
	virtual ~PluginArray();

	void start_plugins(MWindow *mwindow, 
		EDL *edl, 
		PluginServer *plugin_server, 
		KeyFrame *keyframe,
		ptstime start,
		ptstime end,
		File *file);
	int run_plugins();
	void stop_plugins();
	virtual void create_modules() {};
	virtual void create_buffers() {};
	virtual void get_buffers() {};
	virtual void process_realtime(int module, ptstime pts, ptstime len) {};
	virtual int process_loop(int module, int &write_length) {};
	virtual int write_buffers(int len) { return 0; };
	virtual int get_bufsize() { return 0; };
	virtual int total_tracks() { return 0; };
	virtual void get_recordable_tracks() {};
	virtual Track* track_number(int number) { return 0; };
	virtual posnum to_units(ptstime pts) { return 0; };
	virtual ptstime from_units(posnum pos) { return 0; };

	Module **modules;
	MWindow *mwindow;
	CICache *cache;
	EDL *edl;
	PluginServer *plugin_server;
	KeyFrame *keyframe;
// output file
	File *file;
	int buffer_size;
// Start and end of segment in units
	ptstime start;
	ptstime end;
	int done;
	int error;
	int data_type;
};

#endif
