#ifndef POLAR_H
#define POLAR_H

#define MAXDEPTH 100
#define MAXANGLE 360

class PolarMain;
class PolarEngine;

#include "bcbase.h"
#include "polarwindow.h"
#include "pluginvclient.h"


class PolarMain : public PluginVClient
{
public:
	PolarMain(int argc, char *argv[]);
	~PolarMain();

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

// parameters needed
	int reconfigure();    // Rebuild tables
	int depth;
	int angle;
	int polar_to_rectangular;
	int backwards;
	int inverse;
	int automated_function;
	int reconfigure_flag;
	VFrame *temp_frame;

// a thread for the GUI
	PolarThread *thread;

private:
	BC_Hash *defaults;
	PolarEngine **engine;
};

class PolarEngine : public Thread
{
public:
	PolarEngine(PolarMain *plugin, int start_y, int end_y);
	~PolarEngine();
	
	int start_process_frame(VFrame **output, VFrame **input, int size);
	int wait_process_frame();
	void run();
	void get_pixel(const int &x, const int &y, VPixel *pixel, VPixel **input_rows);
	int calc_undistorted_coords(int wx,
			 int wy,
			 double &x,
			 double &y);
	inline VWORD bilinear(double &x, double &y, VWORD *values)
	{
		double m0, m1;
		x = fmod(x, 1.0);
		y = fmod(y, 1.0);

		if(x < 0.0) x += 1.0;
		if(y < 0.0) y += 1.0;

		m0 = (double)values[0] + x * ((double)values[1] - values[0]);
		m1 = (double)values[2] + x * ((double)values[3] - values[2]);
		return (VWORD)(m0 + y * (m1 - m0));
	}
	
	PolarMain *plugin;
	int start_y;
	int end_y;
	int size;
	double cen_x, cen_y;
	VFrame **output, **input;
	int last_frame;
	Mutex input_lock, output_lock;
};


#endif
