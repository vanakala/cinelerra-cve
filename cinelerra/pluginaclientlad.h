// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PLUGINACLIENTLAD_H
#define PLUGINACLIENTLAD_H

#include "config.h"

#ifdef HAVE_LADSPA

#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_SYNTHESIS
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE ((const char*)server->lad_descriptor->Name)
#define PLUGIN_CLASS PluginAClientLAD
#define PLUGIN_CONFIG_CLASS PluginAClientConfig
#define PLUGIN_THREAD_CLASS PluginAClientThread
#define PLUGIN_GUI_CLASS PluginAClientWindow

#include "pluginmacros.h"

#include "bcdisplayinfo.h"
#include "bcpot.h"
#include "bctoggle.h"
#include "pluginaclient.h"
#include "pluginaclientlad.inc"
#include "pluginserver.h"
#include "pluginwindow.h"

#include <ladspa.h>


class PluginAClientConfig
{
public:
	PluginAClientConfig();
	~PluginAClientConfig();

	int equivalent(PluginAClientConfig &that);
	void copy_from(PluginAClientConfig &that);
	void interpolate(PluginAClientConfig &prev, 
		PluginAClientConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
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
	PLUGIN_CONFIG_CLASS_MEMBERS
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
		double min,
		double max);

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


class PluginAClientWindow : public PluginWindow
{
public:
	PluginAClientWindow(PluginAClientLAD *plugin, 
		int x, 
		int y);
	~PluginAClientWindow();

	void update();

	ArrayList<PluginACLientToggle*> toggles;
	ArrayList<PluginACLientILinear*> ipots;
	ArrayList<PluginACLientFLinear*> fpots;
	ArrayList<PluginACLientFreq*> freqs;

	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class PluginAClientLAD : public PluginAClient
{
public:
	PluginAClientLAD(PluginServer *server);
	~PluginAClientLAD();

	AFrame *process_tmpframe(AFrame *frame);
	void process_tmpframes(AFrame **frames);

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	PLUGIN_CLASS_MEMBERS
	int is_multichannel();

	static char* lad_to_string(char *string, const char *input);
	static char* lad_to_upper(char *string, const char *input);
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

#endif /* HAVE_LADSPA */
#endif
