#ifndef BURN_H
#define BURN_H

class BurnMain;

#include "defaults.h"
#include "effecttv.inc"
#include "loadbalance.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "burnwindow.h"
#include <sys/types.h>

class BurnConfig
{
public:
	BurnConfig();
	int threshold;
	int decay;
	double recycle;  // Seconds to a recycle
};

class BurnPackage : public LoadPackage
{
public:
	BurnPackage();

	int row1, row2;
};

class BurnServer : public LoadServer
{
public:
	BurnServer(BurnMain *plugin, int total_clients, int total_packages);
	
	LoadClient* new_client();
	LoadPackage* new_package();
	void init_packages();
	BurnMain *plugin;
};

class BurnClient : public LoadClient
{
public:
	BurnClient(BurnServer *server);
	
	void process_package(LoadPackage *package);

	BurnMain *plugin;
};


class BurnMain : public PluginVClient
{
public:
	BurnMain(PluginServer *server);
	~BurnMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	char* plugin_title();
	int show_gui();
	void raise_window();
	int set_string();
	void load_configuration();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	
	int load_defaults();
	int save_defaults();
	VFrame* new_picon();




	void make_palette();

// a thread for the GUI
	BurnThread *thread;
	BurnServer *burn_server;
	BurnConfig config;

	int palette[3][256];
	unsigned char *buffer;
	
	int total;

	EffectTV *effecttv;
	Defaults *defaults;
	VFrame *input_ptr, *output_ptr;
};










#endif
