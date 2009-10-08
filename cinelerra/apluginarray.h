
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

#ifndef APLUGINARRAY_H
#define APLUGINARRAY_H

#include "amodule.inc"
#include "edl.inc"
#include "pluginarray.h"
#include "pluginserver.inc"
#include "recordableatracks.inc"
#include "track.inc"

class APluginArray : public PluginArray
{
public:
	APluginArray();
	~APluginArray();

	int64_t get_bufsize();
	void create_buffers();
	void create_modules();
	void get_buffers();
//	void load_module(int module, int64_t input_position, int64_t len);
	void process_realtime(int module, int64_t input_position, int64_t len);
	int process_loop(int module, int64_t &write_length);
	int write_buffers(int64_t len);
	int total_tracks();
	void get_recordable_tracks();
	Track* track_number(int number);

	RecordableATracks *tracks;
// Pointers to plugin buffers for plugin output
	double **buffer;         // Buffer for processing
// Pointer to file output
	double **output_buffer;
	double **realtime_buffers;
};

#endif
