// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "asset.inc"
#include "bchash.inc"
#include "bcwindowbase.inc"
#include "cinelerra.h"
#include "datatype.h"
#include "mutex.inc"
#include "preferences.inc"


class Preferences
{
public:
	Preferences();
	~Preferences();

	Preferences& operator=(Preferences &that);
	void copy_from(Preferences *that);
	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);
	void boundaries();

	static void print_channels(char *string, 
		int *channel_positions, 
		int channels);
	static void scan_channels(char *string, 
		int *channel_positions, 
		int channels);

	void add_node(char *text, int port, int enabled, double rate);
	void delete_node(int number);
	void delete_nodes();
	void reset_rates();
// Get average frame rate or 1.0
	double get_avg_rate(int use_master_node);
	void sort_nodes();
	void edit_node(int number, char *new_text, int port, int enabled);
	int get_enabled_nodes();
	int get_brender_node();
	const char* get_node_hostname(int number);
	int get_node_port(int number);

// Copy frame rates.  Always used where the argument is the renderfarm and this is
// the master preferences.  This way, the value for master node is properly 
// translated from a unix socket to the local_rate.
	void copy_rates_from(Preferences *preferences);

// Set frame rate for a node.  Node -1 is the master node.
// The node number is relative to the enabled nodes.
	void set_rate(double rate, int node);

// Calculate the number of cpus to use.
// Determined by /proc/cpuinfo.
	void calculate_processors();

// ================================= Performance ================================
// directory to look in for indexes
	char index_directory[BCTEXTLEN];
// size of index file in kilobytes
	int index_size;
	int index_count;
// Use thumbnails in AWindow assets.
	int use_thumbnails;
// Title of theme
	char theme[BCTEXTLEN];
	ptstime render_preroll;
	int brender_preroll;
// Maximum threads (1 .. 2 * processors)
	int max_threads;
// Number of processors in system
	int real_processors;

// Default positions for channels
	int channel_positions[MAXCHANNELS * MAXCHANNELS];

	Asset *brender_asset;
	int use_brender;
// Number of frames in a brender job.
	int brender_fragment;
// Size of cache in bytes.
// Several caches of cache_size exist so multiply by 4.
// rendering, playback, timeline, preview
	int cache_size;

	int use_renderfarm;
	int renderfarm_port;
// If the node starts with a / it's on the localhost using a path as the socket.
	ArrayList<char*> renderfarm_nodes;
	ArrayList<int>   renderfarm_ports;
	ArrayList<int>   renderfarm_enabled;
	ArrayList<double> renderfarm_rate;
// Rate of master node
	double local_rate;
// Use virtual filesystem
	int renderfarm_vfs;
// Jobs per node
	int renderfarm_job_count;
// Tip of the day
	int use_tipwindow;

// ====================================== Plugin Set ==============================
	char global_plugin_dir[BCTEXTLEN];

// Required when updating renderfarm rates
	Mutex *preferences_lock;
};

#endif
