
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

#ifndef PLUGINACLIENTLAD_H
#define PLUGINACLIENTLAD_H

#include "bcdisplayinfo.h"
#include "guicast.h"
#include "ladspa.h"
#include "pluginaclient.h"
#include "pluginaclientlad.inc"


class PluginAClientConfig
{
public:
	PluginAClientConfig();
	~PluginAClientConfig();

	int equivalent(PluginAClientConfig &that);
	void copy_from(PluginAClientConfig &that);
	void interpolate(PluginAClientConfig &prev, 
		PluginAClientConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void reset();
	void delete_objects();
// Create the port tables based on the LAD descriptor
	void initialize(PluginServer *server);
;
//  Need total_ports record to avoid saving default data buffer ports.
	int total_ports;
	enum
	{
		PORT_NORMAL,
		PORT_FREQ_INDEX,
		PORT_TOGGLE,
		PORT_INTEGER
	};
	int *port_type;
// Frequencies are stored in units of frequency to aid the GUI.
	LADSPA_Data *port_data;
};

class PluginACLientToggle : public BC_CheckBox
{
public:
	PluginACLientToggle(PluginAClientLAD *plugin,
		int x,
		int y,
		LADSPA_Data *output);
	int handle_event();
	PluginAClientLAD *plugin;
	LADSPA_Data *output;
};

class PluginACLientILinear : public BC_IPot
{
public:
	PluginACLientILinear(PluginAClientLAD *plugin,
		int x,
		int y,
		LADSPA_Data *output,
		int min,
		int max);
	int handle_event();
	PluginAClientLAD *plugin;
	LADSPA_Data *output;
};

class PluginACLientFLinear : public BC_FPot
{
public:
	PluginACLientFLinear(PluginAClientLAD *plugin,
		int x,
		int y,
		LADSPA_Data *output,
		float min,
		float max);
	int handle_event();
	PluginAClientLAD *plugin;
	LADSPA_Data *output;
};

class PluginACLientFreq : public BC_QPot
{
public:
	PluginACLientFreq(PluginAClientLAD *plugin,
		int x,
		int y,
		LADSPA_Data *output,
		int translate_linear);
	int handle_event();
	PluginAClientLAD *plugin;
	LADSPA_Data *output;
// Decode LAD frequency table
	int translate_linear;
};


class PluginAClientWindow : public BC_Window
{
public:
	PluginAClientWindow(PluginAClientLAD *plugin, 
		int x, 
		int y);
	~PluginAClientWindow();
	
	int create_objects();
	int close_event();



	ArrayList<PluginACLientToggle*> toggles;
	ArrayList<PluginACLientILinear*> ipots;
	ArrayList<PluginACLientFLinear*> fpots;
	ArrayList<PluginACLientFreq*> freqs;

	PluginAClientLAD *plugin;
};

PLUGIN_THREAD_HEADER(PluginAClientLAD, PluginAClientThread, PluginAClientWindow)

class PluginAClientLAD : public PluginAClient
{
public:
	PluginAClientLAD(PluginServer *server);
	~PluginAClientLAD();

	int process_realtime(int64_t size, 
		double *input_ptr, 
		double *output_ptr);
	int process_realtime(int64_t size, 
		double **input_ptr, 
		double **output_ptr);
// Update output pointers as well
	void update_gui();
	int is_realtime();
	int is_multichannel();
	int is_synthesis();
	int uses_gui();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	PLUGIN_CLASS_MEMBERS(PluginAClientConfig, PluginAClientThread)


	static char* lad_to_string(char *string, char *input);
	static char* lad_to_upper(char *string, char *input);
	int get_inchannels();
	int get_outchannels();
	void delete_buffers();
	void delete_plugin();
	void init_plugin(int total_in, int total_out, int size);

// Temporaries for LAD data
	LADSPA_Data **in_buffers;
	int total_inbuffers;
	LADSPA_Data **out_buffers;
	int total_outbuffers;
	int buffer_allocation;
	LADSPA_Handle lad_instance;
	LADSPA_Data dummy_control_output;
};

#endif
