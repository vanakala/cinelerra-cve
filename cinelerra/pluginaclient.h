
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

#ifndef PLUGINACLIENT_H
#define PLUGINACLIENT_H

#include "aframe.h"
#include "maxbuffers.h"
#include "pluginclient.h"

class PluginAClient : public PluginClient
{
public:
	PluginAClient(PluginServer *server);
	virtual ~PluginAClient();

	void init_realtime_parameters();

	int is_audio();

// Multichannel buffer process for backwards compatibility
	virtual void process_realtime(AFrame *input, AFrame *output) {};
	virtual void process_realtime(AFrame **input, AFrame **output) {};

// Process buffer using pull method.  By default this loads the input into the
// frame and calls process_frame with input and output pointing to frame.
	virtual void process_frame(AFrame *aframe);
	virtual void process_frame(AFrame **aframe);

	virtual int process_loop(AFrame *aframe, int &write_length) { return 1; };
	virtual int process_loop(AFrame **aframes, int &write_length) { return 1; };
	int plugin_process_loop(AFrame **buffers, int &write_length);

// Called by plugin to read audio from previous entity
	void get_aframe(AFrame *frame);

// Get the sample rate of the EDL
	int get_project_samplerate();

	void send_render_gui(void *data, int size);
	void plugin_render_gui(void *data, int size);
	virtual void render_gui(void *data, int size) {};

// sample rate of EDL.
	int project_sample_rate;
};

#endif
