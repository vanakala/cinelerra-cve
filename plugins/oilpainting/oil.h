#ifndef OIL_H
#define OIL_H

class OilMain;

#include "bcbase.h"
#include "oilwindow.h"
#include "pluginvclient.h"

typedef struct
{
	float red;
    float green;
    float blue;
    float alpha;
} pixel_f;

class OilMain : public PluginVClient
{
public:
	OilMain(int argc, char *argv[]);
	~OilMain();

// required for all realtime plugins
	int process_realtime(long size, VFrame **input_ptr, VFrame **output_ptr);
	int plugin_is_realtime();
	int plugin_is_multi_channel();
	char* plugin_title();
	int start_gui();
	int stop_gui();
	int show_gui();
	int hide_gui();
	int set_string();
	int load_defaults();
	int save_defaults();
	int save_data(char *text);
	int read_data(char *text);

// parameters needed for oil painting
	int reconfigure();
	int oil_rgb(VPixel **in_rows, VPixel **out_rows, int use_intensity);

	int radius;
	int use_intensity;
    int redo_buffers;

// a thread for the GUI
	OilThread *thread;

private:
	Defaults *defaults;
	VFrame *temp_frame;
};


#endif
