#ifndef DBX_H
#define DBX_H



#include "defaults.inc"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"

class DBXEffect;





class DBXSize : public BC_TextBox
{
public:
	DBXSize(DBXEffect *plugin, int x, int y);
	int handle_events();
	DBXEffect *plugin;
};


class DBXGain : public BC_TextBox
{
public:
	DBXGain(DBXEffect *plugin, int x, int y);
	int handle_events();
	DBXEffect *plugin;
};


class DBXWindow : public BC_Window
{
public:
	DBXWindow(DBXEffect *plugin, int x, int y);
	void create_objects();
	void update();
	void update_textboxes();
	void update_canvas();
	int close_event();
	void draw_scales();
	
	
	DBXCanvas *canvas;
	DBXPreview *preview;
	DBXReaction *reaction;
	DBXClear *clear;
	DBXX *x_text;
	DBXY *y_text;
	DBXTrigger *trigger;
	DBXEffect *plugin;
};

class DBXThread : public Thread
{
public:
	DBXThread(DBXEffect *plugin);
	~DBXThread();
	void run();
	Mutex completion;
	DBXWindow *window;
	DBXEffect *plugin;
};


typedef struct
{
// Units are linear from 0 - 1
	double x, y;
} compressor_point_t;

class DBXConfig
{
public:
	DBXConfig();

	int total_points();
	void remove_point(int number);
	void optimize();
	double get_y(int number);
	double get_x(int number);
// Returns linear output given linear input
	double calculate_linear(double x);
	int set_point(double x, double y);
	void dump();

	int trigger;
	double min_db;
	double preview_len;
	double reaction_len;
	double min_x, min_y;
	double max_x, max_y;
	ArrayList<compressor_point_t> levels;
};

class DBXEffect : public PluginAClient
{
public:
	DBXEffect(PluginServer *server);
	~DBXEffect();

	VFrame* new_picon();
	char* plugin_title();
	int is_multichannel();
	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_realtime(long size, double **input_ptr, double **output_ptr);
	int show_gui();
	void raise_window();
	int set_string();




	int load_defaults();
	int save_defaults();
	void load_configuration();
	void reset();
	void update_gui();
	void delete_dsp();


	double **input_buffer;
	long input_size;
	long input_allocated;
	double *reaction_buffer;
	long reaction_allocated;
	long reaction_position;
	double current_coef;

// Same coefs are applied to all channels
	double *coefs;
	long coefs_allocated;



	Defaults *defaults;
	DBXThread *thread;
	DBXConfig config;
};


#endif
