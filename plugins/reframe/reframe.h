#ifndef REFRAME_H
#define REFRAME_H


#include "guicast.h"
#include "pluginvclient.h"


class ReFrame;


class ReFrameOutput : public BC_TextBox
{
public:
	ReFrameOutput(ReFrame *plugin, int x, int y);
	int handle_event();
	ReFrame *plugin;
};



class ReFrameWindow : public BC_Window
{
public:
	ReFrameWindow(ReFrame *plugin, int x, int y);
	~ReFrameWindow();

	void create_objects();
	int close_event();

	ReFrame *plugin;
};


class ReFrame : public PluginVClient
{
public:
	ReFrame(PluginServer *server);
	~ReFrame();


	char* plugin_title();
	VFrame* new_picon();
	int get_parameters();
	int load_defaults();  
	int save_defaults();  
	int start_loop();
	int stop_loop();
	int process_loop(VFrame *buffer);


	double scale;

	Defaults *defaults;
	MainProgressBar *progress;
	int64_t current_position;
};






#endif
