#ifndef SVG_H
#define SVG_H

// the simplest plugin possible

class SvgMain;
class SvgThread;

#include "defaults.h"
#include "mutex.h"
#include "svgwin.h"
#include "overlayframe.h"
#include "pluginvclient.h"
#include "thread.h"

class SvgConfig
{
public:
	SvgConfig();
	int equivalent(SvgConfig &that);
	void copy_from(SvgConfig &that);
	void interpolate(SvgConfig &prev, 
		SvgConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	float in_x, in_y, in_w, in_h, out_x, out_y, out_w, out_h;
	char svg_file[BCTEXTLEN];
	time_t last_load;
};


class SvgMain : public PluginVClient
{
public:
	SvgMain(PluginServer *server);
	~SvgMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	char* plugin_title();
	int show_gui();
	void raise_window();
	void update_gui();
	int set_string();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	VFrame* new_picon();
	int load_defaults();
	int save_defaults();
	int load_configuration();

// a thread for the GUI
	SvgThread *thread;

	OverlayFrame *overlayer;   // To translate images
	VFrame *temp_frame;        // Used if buffers are the same
	Defaults *defaults;
	SvgConfig config;
	int need_reconfigure;
	int force_png_render;     //force rendering of PNG on first start
};


#endif
