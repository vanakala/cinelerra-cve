#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "asset.inc"
#include "audioconfig.inc"
#include "defaults.inc"
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
	int load_defaults(Defaults *defaults);
	int save_defaults(Defaults *defaults);

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
	int calculate_processors();

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
// number of cpus to use - 1
	int processors;


	Asset *brender_asset;
	int use_brender;
// Number of frames in a brender job.
	int brender_fragment;
// Number of items to store in cache
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

// ====================================== Plugin Set ==============================
	char global_plugin_dir[BCTEXTLEN];
	char local_plugin_dir[BCTEXTLEN];

// Required when updating renderfarm rates
	Mutex *preferences_lock;
};

#endif
