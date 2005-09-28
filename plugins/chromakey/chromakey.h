#ifndef CHROMAKEY_H
#define CHROMAKEY_H




#include "colorpicker.h"
#include "guicast.h"
#include "loadbalance.h"
#include "pluginvclient.h"


class ChromaKey;
class ChromaKey;
class ChromaKeyWindow;

class ChromaKeyConfig
{
public:
	ChromaKeyConfig();

	void copy_from(ChromaKeyConfig &src);
	int equivalent(ChromaKeyConfig &src);
	void interpolate(ChromaKeyConfig &prev, 
		ChromaKeyConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	int get_color();

	float red;
	float green;
	float blue;
	float threshold;
	float slope;
	int use_value;
};

class ChromaKeyColor : public BC_GenericButton
{
public:
	ChromaKeyColor(ChromaKey *plugin, 
		ChromaKeyWindow *gui, 
		int x, 
		int y);

	int handle_event();

	ChromaKey *plugin;
	ChromaKeyWindow *gui;
};

class ChromaKeyThreshold : public BC_FSlider
{
public:
	ChromaKeyThreshold(ChromaKey *plugin, int x, int y);
	int handle_event();
	ChromaKey *plugin;
};
class ChromaKeySlope : public BC_FSlider
{
public:
	ChromaKeySlope(ChromaKey *plugin, int x, int y);
	int handle_event();
	ChromaKey *plugin;
};
class ChromaKeyUseValue : public BC_CheckBox
{
public:
	ChromaKeyUseValue(ChromaKey *plugin, int x, int y);
	int handle_event();
	ChromaKey *plugin;
};

class ChromaKeyUseColorPicker : public BC_GenericButton
{
public:
	ChromaKeyUseColorPicker(ChromaKey *plugin, ChromaKeyWindow *gui, int x, int y);
	int handle_event();
	ChromaKey *plugin;
	ChromaKeyWindow *gui;
};


class ChromaKeyColorThread : public ColorThread
{
public:
	ChromaKeyColorThread(ChromaKey *plugin, ChromaKeyWindow *gui);
	int handle_new_color(int output, int alpha);
	ChromaKey *plugin;
	ChromaKeyWindow *gui;
};


class ChromaKeyWindow : public BC_Window
{
public:
	ChromaKeyWindow(ChromaKey *plugin, int x, int y);
	~ChromaKeyWindow();

	void create_objects();
	int close_event();
	void update_sample();

	ChromaKeyColor *color;
	ChromaKeyThreshold *threshold;
	ChromaKeyUseValue *use_value;
	ChromaKeyUseColorPicker *use_colorpicker;
	ChromaKeySlope *slope;
	BC_SubWindow *sample;
	ChromaKey *plugin;
	ChromaKeyColorThread *color_thread;
};



PLUGIN_THREAD_HEADER(ChromaKey, ChromaKeyThread, ChromaKeyWindow)


class ChromaKeyServer : public LoadServer
{
public:
	ChromaKeyServer(ChromaKey *plugin);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	ChromaKey *plugin;
};

class ChromaKeyPackage : public LoadPackage
{
public:
	ChromaKeyPackage();
	int y1, y2;
};

class ChromaKeyUnit : public LoadClient
{
public:
	ChromaKeyUnit(ChromaKey *plugin, ChromaKeyServer *server);
	void process_package(LoadPackage *package);
	ChromaKey *plugin;
};




class ChromaKey : public PluginVClient
{
public:
	ChromaKey(PluginServer *server);
	~ChromaKey();
	
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int load_configuration();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int show_gui();
	int set_string();
	void raise_window();
	void update_gui();

	ChromaKeyConfig config;
	VFrame *input, *output;
	ChromaKeyServer *engine;
	ChromaKeyThread *thread;
	Defaults *defaults;
};








#endif







