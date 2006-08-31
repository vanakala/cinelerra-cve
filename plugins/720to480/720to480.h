#ifndef _720TO480_H
#define _720TO480_H


#include "pluginvclient.h"

class _720to480Main;
class _720to480Order;
class _720to480Direction;

class _720to480Window : public BC_Window
{
public:
	_720to480Window(_720to480Main *client, int x, int y);
	~_720to480Window();
	
	int create_objects();
	int close_event();
	int set_first_field(int first_field);
	int set_direction(int direction);

	_720to480Main *client;
	_720to480Order *odd_first;
	_720to480Order *even_first;
	_720to480Direction *forward;
	_720to480Direction *reverse;
};


class _720to480Order : public BC_Radial
{
public:
	_720to480Order(_720to480Main *client, 
		_720to480Window *window, 
		int output, 
		int x, 
		int y, 
		char *text);
	int handle_event();

	_720to480Main *client;
	_720to480Window *window;
	int output;
};

class _720to480Direction : public BC_Radial
{
public:
	_720to480Direction(_720to480Main *client, 
		_720to480Window *window, 
		int output, 
		int x, 
		int y, 
		char *text);
	int handle_event();

	_720to480Main *client;
	_720to480Window *window;
	int output;
};

class _720to480Config
{
public:
	_720to480Config();

	int first_field;
	int direction;
};

class _720to480Main : public PluginVClient
{
public:
	_720to480Main(PluginServer *server);
	~_720to480Main();


	

// required for all non realtime plugins
	char* plugin_title();
	int get_parameters();
	int start_loop();
	int stop_loop();
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	double get_framerate();
	int process_loop(VFrame *output);

	void reduce_field(VFrame *output, VFrame *input, int dest_row);


	BC_Hash *defaults;
	MainProgressBar *progress;

	_720to480Config config;
	VFrame *temp;
	int input_position;
};


#endif
