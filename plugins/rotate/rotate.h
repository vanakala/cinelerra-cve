#ifndef ROTATE_H
#define ROTATE_H


#include "defaults.inc"
#include "guicast.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "rotateframe.inc"
#include "vframe.inc"





class RotateMain;
class RotateWindow;

#define MAXANGLE 360



class RotateConfig
{
public:
	RotateConfig();
	
	int equivalent(RotateConfig &that);
	void copy_from(RotateConfig &that);
	
	float angle;
};

class RotateToggle : public BC_Radial
{
public:
	RotateToggle(RotateWindow *window, 
		RotateMain *plugin, 
		int init_value, 
		int x, 
		int y, 
		int value, 
		char *string);
	int handle_event();

	RotateMain *plugin;
    RotateWindow *window;
    int value;
};

class RotateFine : public BC_FPot
{
public:
	RotateFine(RotateWindow *window, RotateMain *plugin, int x, int y);
	int handle_event();

	RotateMain *plugin;
    RotateWindow *window;
};

class RotateText : public BC_TextBox
{
public:
	RotateText(RotateWindow *window, RotateMain *plugin, int x, int y);
	int handle_event();

	RotateMain *plugin;
    RotateWindow *window;
};

class RotateWindow : public BC_Window
{
public:
	RotateWindow(RotateMain *plugin, int x, int y);

	int create_objects();
	int close_event();
	int update();
	int update_fine();
	int update_toggles();

	RotateMain *plugin;
	RotateToggle *toggle0;
	RotateToggle *toggle90;
	RotateToggle *toggle180;
	RotateToggle *toggle270;
	RotateFine *fine;
	RotateText *text;
};

class RotateThread : public Thread
{
public:
	RotateThread(RotateMain *plugin);
	~RotateThread();

	void run();

	Mutex completion; // prevent loading data until the GUI is started
	RotateMain *plugin;
	RotateWindow *window;
};


class RotateMain : public PluginVClient
{
public:
	RotateMain(PluginServer *server);
	~RotateMain();
	
	
	
	
	
	
	
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int show_gui();
	void raise_window();
	void update_gui();
	int set_string();
	void load_configuration();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);







	void reset();







	RotateConfig config;
	RotateFrame *engine;
	RotateThread *thread;
	Defaults *defaults;
	int need_reconfigure;
};


#endif
