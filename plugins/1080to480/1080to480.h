#ifndef _1080TO480_H
#define _1080TO480_H


#include "overlayframe.inc"
#include "pluginvclient.h"

class _1080to480Main;
class _1080to480Option;

class _1080to480Window : public BC_Window
{
public:
	_1080to480Window(_1080to480Main *client, int x, int y);
	~_1080to480Window();
	
	int create_objects();
	int close_event();
	int set_first_field(int first_field, int send_event);
	
	_1080to480Main *client;
	_1080to480Option *odd_first;
	_1080to480Option *even_first;
};

PLUGIN_THREAD_HEADER(_1080to480Main, _1080to480Thread, _1080to480Window);

class _1080to480Option : public BC_Radial
{
public:
	_1080to480Option(_1080to480Main *client, 
		_1080to480Window *window, 
		int output, 
		int x, 
		int y, 
		char *text);
	int handle_event();

	_1080to480Main *client;
	_1080to480Window *window;
	int output;
};

class _1080to480Config
{
public:
	_1080to480Config();

	int equivalent(_1080to480Config &that);
	void copy_from(_1080to480Config &that);
	void interpolate(_1080to480Config &prev, 
		_1080to480Config &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	int first_field;
};

class _1080to480Main : public PluginVClient
{
public:
	_1080to480Main(PluginServer *server);
	~_1080to480Main();


	PLUGIN_CLASS_MEMBERS(_1080to480Config, _1080to480Thread)
	

// required for all realtime plugins
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	int hide_gui();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();

	void reduce_field(VFrame *output, VFrame *input, int src_field, int dst_field);

	VFrame *temp;
};


#endif
