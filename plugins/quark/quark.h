#ifndef QUARK_H
#define QUARK_H

class QuarkMain;
#define MAXSHARPNESS 100

#include "defaults.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "sharpenwindow.h"

#include <sys/types.h>



class QuarkEngine : public Thread
{
public:
	QuarkEngine(QuarkMain *plugin);
	~QuarkEngine();

	int start_process_frame(VFrame *output, VFrame *input, int row1, int row2);
	int wait_process_frame();
	void run();

	void filter(int components,
		int wordsize,
		int vmax,
		int w, 
		unsigned char *src, 
		unsigned char *dst,
		int *neg0, 
		int *neg1, 
		int *neg2);
	void filter(int components,
		int wordsize,
		int vmax,
		int w, 
		u_int16_t *src, 
		u_int16_t *dst,
		int *neg0, 
		int *neg1, 
		int *neg2);


	void filter_888(int w, 
		unsigned char *src, 
		unsigned char *dst,
		int *neg0, 
		int *neg1, 
		int *neg2);
	void filter_8888(int w, 
		unsigned char *src, 
		unsigned char *dst,
		int *neg0, 
		int *neg1, 
		int *neg2);
	void filter_161616(int w, 
		u_int16_t *src, 
		u_int16_t *dst,
		int *neg0, 
		int *neg1, 
		int *neg2);
	void filter_16161616(int w, 
		u_int16_t *src, 
		u_int16_t *dst,
		int *neg0, 
		int *neg1, 
		int *neg2);

	void sharpen_888();
	void sharpen_161616();
	void sharpen_8888();
	void sharpen_16161616();

	int filter(int w, 
		unsigned char *src, 
		unsigned char *dst, 
		int *neg0, 
		int *neg1, 
		int *neg2);

	QuarkMain *plugin;
	int field;
	VFrame *output, *input;
	int last_frame;
	Mutex input_lock, output_lock;
	unsigned char *src_rows[4], *dst_row;
	int *neg_rows[4];
	int row1, row2;
};

class QuarkMain : public PluginVClient
{
public:
	QuarkMain(PluginServer *server);
	~QuarkMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	char* plugin_title();
	int show_gui();
	void raise_window();
	int set_string();
	void load_configuration();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	VFrame* new_picon();

// parameters needed for sharpness
	float sharpness; // Range from 0 to 100
	float last_sharpness;
	int interlace;
	int horizontal;
	int row_step;
	int luminance;

// a thread for the GUI
	QuarkThread *thread;
	int pos_lut[0x10000], neg_lut[0x10000];

private:
	int get_luts(int *pos_lut, int *neg_lut, int color_model);
	Defaults *defaults;
	QuarkEngine **engine;
	int total_engines;
};

#endif
