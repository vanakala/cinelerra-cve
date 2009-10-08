
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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "asset.inc"
#include "audioconfig.inc"
#include "bchash.inc"
#include "guicast.h"
#include "maxchannels.h"
#include "mutex.inc"
#include "preferences.inc"
#include "videoconfig.inc"


class Preferences
{
public:
	Preferences();
	~Preferences();

	Preferences& operator=(Preferences &that);
	void copy_from(Preferences *that);
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);
	void boundaries();

	static void print_channels(char *string, 
		int *channel_positions, 
		int channels);
	static void scan_channels(char *string, 
		int *channel_positions, 
		int channels);

	void add_node(char *text, int port, int enabled, float rate);
	void delete_node(int number);
	void delete_nodes();
	void reset_rates();
// Get average frame rate or 1.0
	float get_avg_rate(int use_master_node);
	void sort_nodes();
	void edit_node(int number, char *new_text, int port, int enabled);
	int get_enabled_nodes();
	char* get_node_hostname(int number);
	int get_node_port(int number);
// Copy frame rates.  Always used where the argument is the renderfarm and this is
// the master preferences.  This way, the value for master node is properly 
// translated from a unix socket to the local_rate.
	void copy_rates_from(Preferences *preferences);
// Set frame rate for a node.  Node -1 is the master node.
// The node number is relative to the enabled nodes.
	void set_rate(float rate, int node);
// Calculate the number of cpus to use.  
// Determined by /proc/cpuinfo and force_uniprocessor.
// interactive forces it to ignore force_uniprocessor
	int calculate_processors(int interactive = 0);

// ================================= Performance ================================
// directory to look in for indexes
	char index_directory[BCTEXTLEN];   
// size of index file in bytes
	int64_t index_size;                  
	int index_count;
// Use thumbnails in AWindow assets.
	int use_thumbnails;
// Title of theme
	char theme[BCTEXTLEN];
	double render_preroll;
	int brender_preroll;
	int force_uniprocessor;
// The number of cpus to use when rendering.
// Determined by /proc/cpuinfo and force_uniprocessor
	int processors;
// Number of processors for interactive operations.
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
	int64_t cache_size;

	int use_renderfarm;
	int renderfarm_port;
// If the node starts with a / it's on the localhost using a path as the socket.
	ArrayList<char*> renderfarm_nodes;
	ArrayList<int>   renderfarm_ports;
	ArrayList<int>   renderfarm_enabled;
	ArrayList<float> renderfarm_rate;
// Rate of master node
	float local_rate;
	char renderfarm_mountpoint[BCTEXTLEN];
// Use virtual filesystem
	int renderfarm_vfs;
// Jobs per node
	int renderfarm_job_count;
// Consolidate output files
	int renderfarm_consolidate;

// Tip of the day
	int use_tipwindow;

// ====================================== Plugin Set ==============================
	char global_plugin_dir[BCTEXTLEN];
	char local_plugin_dir[BCTEXTLEN];

// Required when updating renderfarm rates
	Mutex *preferences_lock;
};

#endif
