#ifndef SWAPCHANNELS_H
#define SWAPCHANNELS_H

#include "defaults.inc"
#include "guicast.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "vframe.inc"





class SwapMain;

#define RED_SRC 0
#define GREEN_SRC 1
#define BLUE_SRC 2
#define ALPHA_SRC 3
#define NO_SRC 4
#define MAX_SRC 5



class SwapConfig
{
public:
	SwapConfig();

	int equivalent(SwapConfig &that);
	void copy_from(SwapConfig &that);

	int red;
	int green;
	int blue;
	int alpha;
};



class SwapMenu : public BC_PopupMenu
{
public:
	SwapMenu(SwapMain *client, int *output, int x, int y);


	int handle_event();
	int create_objects();

	SwapMain *client;
	int *output;
};


class SwapItem : public BC_MenuItem
{
public:
	SwapItem(SwapMenu *menu, char *title);

	int handle_event();

	SwapMenu *menu;
	char *title;
};

class SwapWindow : public BC_Window
{
public:
	SwapWindow(SwapMain *plugin, int x, int y);
	~SwapWindow();


	void create_objects();
	int close_event();

	SwapMain *plugin;
	SwapMenu *red;
	SwapMenu *green;
	SwapMenu *blue;
	SwapMenu *alpha;
};




PLUGIN_THREAD_HEADER(SwapMain, SwapThread, SwapWindow)



class SwapMain : public PluginVClient
{
public:
	SwapMain(PluginServer *server);
	~SwapMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	int is_synthesis();
	char* plugin_title();
	VFrame* new_picon();
	int show_gui();
	void raise_window();
	void update_gui();
	int set_string();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);







	void reset();
	void load_configuration();
	int load_defaults();
	int save_defaults();




// parameters needed for processor
	char* output_to_text(int value);
	int text_to_output(char *text);

	VFrame *temp;
	SwapConfig config;
	SwapThread *thread;
	Defaults *defaults;
};


#endif
