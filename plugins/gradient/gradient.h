#ifndef GRADIENT_H
#define GRADIENT_H

class GradientMain;
class GradientEngine;
class GradientThread;
class GradientWindow;
class GradientServer;


#define MAXRADIUS 10000

#include "colorpicker.h"
#include "defaults.inc"
#include "filexml.inc"
#include "guicast.h"
#include "loadbalance.h"
#include "overlayframe.inc"
#include "plugincolors.h"
#include "pluginvclient.h"
#include "thread.h"
#include "vframe.inc"

class GradientConfig
{
public:
	GradientConfig();

	int equivalent(GradientConfig &that);
	void copy_from(GradientConfig &that);
	void interpolate(GradientConfig &prev, 
		GradientConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);
// Int to hex triplet conversion
	int get_in_color();
	int get_out_color();

	double angle;
	double in_radius;
	double out_radius;
	int in_r, in_g, in_b, in_a;
	int out_r, out_g, out_b, out_a;
};


class GradientAngle : public BC_FPot
{
public:
	GradientAngle(GradientMain *plugin, int x, int y);
	int handle_event();
	GradientMain *plugin;
};

class GradientInRadius : public BC_FSlider
{
public:
	GradientInRadius(GradientMain *plugin, int x, int y);
	int handle_event();
	GradientMain *plugin;
};

class GradientOutRadius : public BC_FSlider
{
public:
	GradientOutRadius(GradientMain *plugin, int x, int y);
	int handle_event();
	GradientMain *plugin;
};

class GradientInColorButton : public BC_GenericButton
{
public:
	GradientInColorButton(GradientMain *plugin, GradientWindow *window, int x, int y);
	int handle_event();
	GradientMain *plugin;
	GradientWindow *window;
};


class GradientOutColorButton : public BC_GenericButton
{
public:
	GradientOutColorButton(GradientMain *plugin, GradientWindow *window, int x, int y);
	int handle_event();
	GradientMain *plugin;
	GradientWindow *window;
};


class GradientInColorThread : public ColorThread
{
public:
	GradientInColorThread(GradientMain *plugin, GradientWindow *window);
	int handle_event(int output);
	GradientMain *plugin;
	GradientWindow *window;
};


class GradientOutColorThread : public ColorThread
{
public:
	GradientOutColorThread(GradientMain *plugin, GradientWindow *window);
	int handle_event(int output);
	GradientMain *plugin;
	GradientWindow *window;
};



class GradientWindow : public BC_Window
{
public:
	GradientWindow(GradientMain *plugin, int x, int y);
	~GradientWindow();
	
	int create_objects();
	int close_event();
	void update_in_color();
	void update_out_color();

	GradientMain *plugin;
	GradientAngle *angle;
	GradientInRadius *in_radius;
	GradientOutRadius *out_radius;
	GradientInColorButton *in_color;
	GradientOutColorButton *out_color;
	GradientInColorThread *in_color_thread;
	GradientOutColorThread *out_color_thread;
	int in_color_x, in_color_y;
	int out_color_x, out_color_y;
};



PLUGIN_THREAD_HEADER(GradientMain, GradientThread, GradientWindow)


class GradientMain : public PluginVClient
{
public:
	GradientMain(PluginServer *server);
	~GradientMain();

	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_synthesis();

	PLUGIN_CLASS_MEMBERS(GradientConfig, GradientThread)

	int need_reconfigure;

	OverlayFrame *overlayer;
	VFrame *gradient;
	VFrame *input, *output;
	GradientServer *engine;
};

class GradientPackage : public LoadPackage
{
public:
	GradientPackage();
	int y1;
	int y2;
};

class GradientUnit : public LoadClient
{
public:
	GradientUnit(GradientServer *server, GradientMain *plugin);
	void process_package(LoadPackage *package);
	GradientServer *server;
	GradientMain *plugin;
	YUV yuv;
};

class GradientServer : public LoadServer
{
public:
	GradientServer(GradientMain *plugin, int total_clients, int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	GradientMain *plugin;
};



#endif
