#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "keyframe.h"
#include "loadbalance.h"
#include "max_picon_png.h"
#include "mid_picon_png.h"
#include "min_picon_png.h"
#include "picon_png.h"
#include "../colors/plugincolors.h"
#include "pluginvclient.h"
#include "vframe.h"

class HistogramMain;
class HistogramEngine;
class HistogramWindow;

// modes
#define HISTOGRAM_RED 0
#define HISTOGRAM_GREEN 1
#define HISTOGRAM_BLUE 2
#define HISTOGRAM_ALPHA 3
#define HISTOGRAM_VALUE 4

// range
#define HISTOGRAM_RANGE 0x10000

#define THRESHOLD_SCALE 1000

class HistogramConfig
{
public:
	HistogramConfig();

	int equivalent(HistogramConfig &that);
	void copy_from(HistogramConfig &that);
	void interpolate(HistogramConfig &prev, 
		HistogramConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void reset(int do_mode);

	int input_min[5];
	int input_mid[5];
	int input_max[5];
	int output_min[5];
	int output_max[5];
	int automatic;
	int mode;
	int threshold;
};



class HistogramSlider : public BC_SubWindow
{
public:
	HistogramSlider(HistogramMain *plugin, 
		HistogramWindow *gui,
		int x, 
		int y, 
		int w,
		int h,
		int is_input);

	void update();
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();

	int operation;
	enum
	{
		NONE,
		DRAG_MIN_INPUT,
		DRAG_MID_INPUT,
		DRAG_MAX_INPUT,
		DRAG_MIN_OUTPUT,
		DRAG_MAX_OUTPUT,
	};
	int is_input;
	HistogramMain *plugin;
	HistogramWindow *gui;
};

class HistogramAuto : public BC_CheckBox
{
public:
	HistogramAuto(HistogramMain *plugin, 
		int x, 
		int y);
	int handle_event();
	HistogramMain *plugin;
};

class HistogramMode : public BC_Radial
{
public:
	HistogramMode(HistogramMain *plugin, 
		int x, 
		int y,
		int value,
		char *text);
	int handle_event();
	HistogramMain *plugin;
	int value;
};

class HistogramReset : public BC_GenericButton
{
public:
	HistogramReset(HistogramMain *plugin, 
		int x,
		int y);
	int handle_event();
	HistogramMain *plugin;
};

class HistogramText : public BC_TumbleTextBox
{
public:
	HistogramText(HistogramMain *plugin,
		HistogramWindow *gui,
		int x,
		int y,
		int *output);
	int handle_event();
	HistogramMain *plugin;
	int *output;
};

class HistogramWindow : public BC_Window
{
public:
	HistogramWindow(HistogramMain *plugin, int x, int y);
	~HistogramWindow();

	int create_objects();
	int close_event();
	void update(int do_input);
	void update_mode();
	void update_canvas();
	void update_input();
	void update_output();

	HistogramSlider *input, *output;
	HistogramAuto *automatic;
	HistogramMode *mode_v, *mode_r, *mode_g, *mode_b,  *mode_a;
	HistogramText *input_min;
	HistogramText *input_mid;
	HistogramText *input_max;
	HistogramText *output_min;
	HistogramText *output_max;
	HistogramText *threshold;
	BC_SubWindow *canvas;
	HistogramMain *plugin;
	BC_Pixmap *max_picon, *mid_picon, *min_picon;
};



PLUGIN_THREAD_HEADER(HistogramMain, HistogramThread, HistogramWindow)


class HistogramMain : public PluginVClient
{
public:
	HistogramMain(PluginServer *server);
	~HistogramMain();

	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	void render_gui(void *data);
	PLUGIN_CLASS_MEMBERS(HistogramConfig, HistogramThread)

// Convert input to input curve
	float calculate_curve(float input, int mode);
// Calculate automatic settings
	void calculate_automatic(VFrame *data);
// Calculate histogram
	void calculate_histogram(VFrame *data);



	YUV yuv;
	VFrame *input, *output;
	HistogramEngine *engine;
	int *lookup[4];
	int64_t *accum[5];
};

class HistogramPackage : public LoadPackage
{
public:
	HistogramPackage();
	int start, end;
};

class HistogramUnit : public LoadClient
{
public:
	HistogramUnit(HistogramEngine *server, HistogramMain *plugin);
	~HistogramUnit();
	void process_package(LoadPackage *package);
	HistogramEngine *server;
	HistogramMain *plugin;
	int64_t *accum[5];
};

class HistogramEngine : public LoadServer
{
public:
	HistogramEngine(HistogramMain *plugin, 
		int total_clients, 
		int total_packages);
	void process_packages(int operation, VFrame *data);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	HistogramMain *plugin;

	int operation;
	enum
	{
		HISTOGRAM,
		TABULATE,
		APPLY
	};
	VFrame *data;
};



















REGISTER_PLUGIN(HistogramMain)



HistogramConfig::HistogramConfig()
{
	reset(1);
}

void HistogramConfig::reset(int do_mode)
{
	for(int i = 0; i < 5; i++)
	{
		input_min[i] = 0;
		input_mid[i] = 0x8000;
		input_max[i] = 0xffff;
		output_min[i] = 0;
		output_max[i] = 0xffff;
	}
	if(do_mode) 
	{
		mode = HISTOGRAM_VALUE;
		automatic = 0;
		threshold = 10;
	}
}

int HistogramConfig::equivalent(HistogramConfig &that)
{
	for(int i = 0; i < 5; i++)
	{
		if(input_min[i] != that.input_min[i] ||
			input_mid[i] != that.input_mid[i] ||
			input_max[i] != that.input_max[i] ||
			output_min[i] != that.output_min[i] ||
			output_max[i] != that.output_max[i]) return 0;
	}

	if(automatic != that.automatic ||
		mode != that.mode ||
		threshold != that.threshold) return 0;

	return 1;
}

void HistogramConfig::copy_from(HistogramConfig &that)
{
	for(int i = 0; i < 5; i++)
	{
		input_min[i] = that.input_min[i];
		input_mid[i] = that.input_mid[i];
		input_max[i] = that.input_max[i];
		output_min[i] = that.output_min[i];
		output_max[i] = that.output_max[i];
	}

	automatic = that.automatic;
	mode = that.mode;
	threshold = that.threshold;
}

void HistogramConfig::interpolate(HistogramConfig &prev, 
	HistogramConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	for(int i = 0; i < 5; i++)
	{
		input_min[i] = (int)(prev.input_min[i] * prev_scale + next.input_min[i] * next_scale);
		input_mid[i] = (int)(prev.input_mid[i] * prev_scale + next.input_mid[i] * next_scale);
		input_max[i] = (int)(prev.input_max[i] * prev_scale + next.input_max[i] * next_scale);
		output_min[i] = (int)(prev.output_min[i] * prev_scale + next.output_min[i] * next_scale);
		output_max[i] = (int)(prev.output_max[i] * prev_scale + next.output_max[i] * next_scale);
	}
	threshold = (int)(prev.threshold * prev_scale + next.threshold * next_scale);
	automatic = prev.automatic;
	mode = prev.mode;
}







PLUGIN_THREAD_OBJECT(HistogramMain, HistogramThread, HistogramWindow)



HistogramWindow::HistogramWindow(HistogramMain *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x,
	y,
	440, 
	480, 
	440, 
	480, 
	0, 
	1)
{
	this->plugin = plugin; 
}

HistogramWindow::~HistogramWindow()
{
}

static VFrame max_picon_image(max_picon_png);
static VFrame mid_picon_image(mid_picon_png);
static VFrame min_picon_image(min_picon_png);

int HistogramWindow::create_objects()
{
	int x = 10, y = 10, x1 = 10;
	int subscript = plugin->config.mode;

	max_picon = new BC_Pixmap(this, &max_picon_image);
	mid_picon = new BC_Pixmap(this, &mid_picon_image);
	min_picon = new BC_Pixmap(this, &min_picon_image);
	add_subwindow(mode_v = new HistogramMode(plugin, 
		x, 
		y,
		HISTOGRAM_VALUE,
		"Value"));
	x += 70;
	add_subwindow(mode_r = new HistogramMode(plugin, 
		x, 
		y,
		HISTOGRAM_RED,
		"Red"));
	x += 70;
	add_subwindow(mode_g = new HistogramMode(plugin, 
		x, 
		y,
		HISTOGRAM_GREEN,
		"Green"));
	x += 70;
	add_subwindow(mode_b = new HistogramMode(plugin, 
		x, 
		y,
		HISTOGRAM_BLUE,
		"Blue"));
	x += 70;
	add_subwindow(mode_a = new HistogramMode(plugin, 
		x, 
		y,
		HISTOGRAM_ALPHA,
		"Alpha"));

	x = x1;
	y += 30;
	add_subwindow(new BC_Title(x, y, "Input min:"));
	x += 80;
	input_min = new HistogramText(plugin,
		this,
		x,
		y,
		&plugin->config.input_min[subscript]);
	input_min->create_objects();
	x += 90;
	add_subwindow(new BC_Title(x, y, "Mid:"));
	x += 40;
	input_mid = new HistogramText(plugin,
		this,
		x,
		y,
		&plugin->config.input_mid[subscript]);
	input_mid->create_objects();
	input_mid->update((int64_t)plugin->config.input_mid[subscript]);
	x += 90;
	add_subwindow(new BC_Title(x, y, "Max:"));
	x += 40;
	input_max = new HistogramText(plugin,
		this,
		x,
		y,
		&plugin->config.input_max[subscript]);
	input_max->create_objects();

	x = x1;
	y += 30;
	add_subwindow(canvas = new BC_SubWindow(x, 
		y, 
		get_w() - x - x, 
		get_h() - y - 150,
		0xffffff));

	y += canvas->get_h() + 10;
	add_subwindow(input = new HistogramSlider(plugin, 
		this,
		x, 
		y, 
		get_w() - 20,
		30,
		1));
	input->update();

	y += input->get_h() + 10;
	add_subwindow(new BC_Title(x, y, "Output min:"));
	x += 90;
	output_min = new HistogramText(plugin,
		this,
		x,
		y,
		&plugin->config.output_min[subscript]);
	output_min->create_objects();
	x += 90;
	add_subwindow(new BC_Title(x, y, "Max:"));
	x += 40;
	output_max = new HistogramText(plugin,
		this,
		x,
		y,
		&plugin->config.output_max[subscript]);
	output_max->create_objects();

	x = x1;
	y += 30;



	add_subwindow(output = new HistogramSlider(plugin, 
		this,
		x, 
		y, 
		get_w() - 20,
		30,
		0));
	output->update();
	y += 40;


	add_subwindow(automatic = new HistogramAuto(plugin, 
		x, 
		y));

	x += 120;
	add_subwindow(new HistogramReset(plugin, 
		x, 
		y));
	x += 100;
	add_subwindow(new BC_Title(x, y, "Threshold:"));
	x += 100;
	threshold = new HistogramText(plugin,
		this,
		x,
		y,
		&plugin->config.threshold);
	threshold->create_objects();

	show_window();
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(HistogramWindow)

void HistogramWindow::update(int do_input)
{
	automatic->update(plugin->config.automatic);
	threshold->update((int64_t)plugin->config.threshold);
	update_mode();

	if(do_input) update_input();
	update_output();
}

void HistogramWindow::update_input()
{
	int subscript = plugin->config.mode;
	input->update();
	input_min->update((int64_t)plugin->config.input_min[subscript]);
	input_mid->update((int64_t)plugin->config.input_mid[subscript]);
	input_max->update((int64_t)plugin->config.input_max[subscript]);
}

void HistogramWindow::update_output()
{
	int subscript = plugin->config.mode;
	output->update();
	output_min->update((int64_t)plugin->config.output_min[subscript]);
	output_max->update((int64_t)plugin->config.output_max[subscript]);
}

void HistogramWindow::update_mode()
{
	mode_v->update(plugin->config.mode == HISTOGRAM_VALUE ? 1 : 0);
	mode_r->update(plugin->config.mode == HISTOGRAM_RED ? 1 : 0);
	mode_g->update(plugin->config.mode == HISTOGRAM_GREEN ? 1 : 0);
	mode_b->update(plugin->config.mode == HISTOGRAM_BLUE ? 1 : 0);
	mode_a->update(plugin->config.mode == HISTOGRAM_ALPHA ? 1 : 0);
	input_min->output = &plugin->config.input_min[plugin->config.mode];
	input_mid->output = &plugin->config.input_mid[plugin->config.mode];
	input_max->output = &plugin->config.input_max[plugin->config.mode];
	output_min->output = &plugin->config.output_min[plugin->config.mode];
	output_max->output = &plugin->config.output_max[plugin->config.mode];
}

void HistogramWindow::update_canvas()
{
	int64_t *accum = plugin->accum[plugin->config.mode];
	int canvas_w = canvas->get_w();
	int canvas_h = canvas->get_h();
	int accum_per_canvas_i = HISTOGRAM_RANGE / canvas_w + 1;
	float accum_per_canvas_f = (float)HISTOGRAM_RANGE / canvas_w;
	int normalize = 0;
	int max = 0;
	for(int i = 0; i < HISTOGRAM_RANGE; i++)
	{
		if(accum[i] > normalize) normalize = accum[i];
	}


	if(normalize)
	{
		for(int i = 0; i < canvas_w; i++)
		{
			int accum_start = (int)(accum_per_canvas_f * i);
			int accum_end = accum_start + accum_per_canvas_i;
			max = 0;
			for(int j = accum_start; j < accum_end; j++)
			{
				max = MAX(accum[j], max);
			}
//printf("HistogramWindow::update_canvas 1 %d %d\n", i, max);

//			max = max * canvas_h / normalize;
			max = (int)(log(max) / log(normalize) * canvas_h);


			canvas->set_color(0xffffff);
			canvas->draw_line(i, 0, i, canvas_h - max);
			canvas->set_color(0x000000);
			canvas->draw_line(i, canvas_h - max, i, canvas_h);
		}
	}
	else
	{
		canvas->set_color(0xffffff);
		canvas->draw_box(0, 0, canvas_w, canvas_h);
	}

	canvas->set_color(0x00ff00);
	int y1;
	for(int i = 0; i < canvas_w; i++)
	{
		int y2 = canvas_h - (int)(plugin->calculate_curve((float)i / canvas_w * (HISTOGRAM_RANGE - 1), 
			plugin->config.mode) * canvas_h / (HISTOGRAM_RANGE - 1));
		if(i > 0)
		{
			canvas->draw_line(i - 1, y1, i, y2);
		}
		y1 = y2;
	}

	canvas->flash();
}








HistogramReset::HistogramReset(HistogramMain *plugin, 
	int x,
	int y)
 : BC_GenericButton(x, y, "Reset")
{
	this->plugin = plugin;
}
int HistogramReset::handle_event()
{
	plugin->config.reset(0);
	plugin->thread->window->update(1);
	plugin->send_configure_change();
	return 1;
}









HistogramSlider::HistogramSlider(HistogramMain *plugin, 
	HistogramWindow *gui,
	int x, 
	int y, 
	int w,
	int h,
	int is_input)
 : BC_SubWindow(x, y, w, h)
{
	this->plugin = plugin;
	this->gui = gui;
	this->is_input = is_input;
	operation = NONE;
}

int HistogramSlider::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
		int subscript = plugin->config.mode;
		int min;
		int max;
		int w = get_w();
		int h = get_h();
		int half_h = get_h() / 2;

		if(is_input)
		{
			int x1 = (int)(plugin->config.input_mid[subscript] * w / 0xffff) - 
				gui->mid_picon->get_w() / 2;
			int x2 = x1 + gui->mid_picon->get_w();
			if(get_cursor_x() >= x1 && get_cursor_x() < x2 &&
				get_cursor_y() >= half_h && get_cursor_y() < h)
			{
				operation = DRAG_MID_INPUT;
			}
		}

		if(operation == NONE)
		{
			if(is_input)
			{
				int x1 = (int)(plugin->config.input_min[subscript] * w / 0xffff) - 
					gui->mid_picon->get_w() / 2;
				int x2 = x1 + gui->mid_picon->get_w();
				if(get_cursor_x() >= x1 && get_cursor_x() < x2 &&
					get_cursor_y() >= half_h && get_cursor_y() < h)
				{
					operation = DRAG_MIN_INPUT;
				}
			}
			else
			{
				int x1 = (int)(plugin->config.output_min[subscript] * w / 0xffff) - 
					gui->mid_picon->get_w() / 2;
				int x2 = x1 + gui->mid_picon->get_w();
				if(get_cursor_x() >= x1 && get_cursor_x() < x2 &&
					get_cursor_y() >= half_h && get_cursor_y() < h)
				{
					operation = DRAG_MIN_OUTPUT;
				}
			}
		}

		if(operation == NONE)
		{
			if(is_input)
			{
				int x1 = (int)(plugin->config.input_max[subscript] * w / 0xffff) - 
					gui->mid_picon->get_w() / 2;
				int x2 = x1 + gui->mid_picon->get_w();
				if(get_cursor_x() >= x1 && get_cursor_x() < x2 &&
					get_cursor_y() >= half_h && get_cursor_y() < h)
				{
					operation = DRAG_MAX_INPUT;
				}
			}
			else
			{
				int x1 = (int)(plugin->config.output_max[subscript] * w / 0xffff) - 
					gui->mid_picon->get_w() / 2;
				int x2 = x1 + gui->mid_picon->get_w();
				if(get_cursor_x() >= x1 && get_cursor_x() < x2 &&
					get_cursor_y() >= half_h && get_cursor_y() < h)
				{
					operation = DRAG_MAX_OUTPUT;
				}
			}
		}
		return 1;
	}
	return 0;
}

int HistogramSlider::button_release_event()
{
	if(operation != NONE)
	{
		operation = NONE;
		return 1;
	}
	return 0;
}

int HistogramSlider::cursor_motion_event()
{
//printf("HistogramSlider::cursor_motion_event 1\n");
	if(operation != NONE)
	{
		float value = (float)get_cursor_x() * 0xffff / get_w();
		CLAMP(value, 0, 0xffff);
		int subscript = plugin->config.mode;
		float input_min = plugin->config.input_min[subscript];
		float input_max = plugin->config.input_max[subscript];
		float input_mid = plugin->config.input_mid[subscript];
		float input_mid_fraction = (input_mid - input_min) / (input_max - input_min);

		switch(operation)
		{
			case DRAG_MIN_INPUT:
				input_min = MIN(input_max, value);
				plugin->config.input_min[subscript] = (int)input_min;
				input_mid = input_min + (input_max - input_min) * input_mid_fraction;
				break;
			case DRAG_MID_INPUT:
				CLAMP(value, input_min, input_max);
				input_mid = (int)value;
				break;
			case DRAG_MAX_INPUT:
				input_max = MAX(input_mid, value);
				input_mid = input_min + (input_max - input_min) * input_mid_fraction;
				break;
			case DRAG_MIN_OUTPUT:
				value = MIN(plugin->config.output_max[subscript], value);
				plugin->config.output_min[subscript] = (int)value;
				break;
			case DRAG_MAX_OUTPUT:
				value = MAX(plugin->config.output_min[subscript], value);
				plugin->config.output_max[subscript] = (int)value;
				break;
		}
	
		if(operation == DRAG_MIN_INPUT ||
			operation == DRAG_MID_INPUT ||
			operation == DRAG_MAX_INPUT)
		{
			plugin->config.input_mid[subscript] = (int)input_mid;
			plugin->config.input_min[subscript] = (int)input_min;
			plugin->config.input_max[subscript] = (int)input_max;
			gui->update_input();
		}
		else
		{
			gui->update_output();
		}

		gui->unlock_window();
		plugin->send_configure_change();
//printf("HistogramSlider::cursor_motion_event 2\n");
		gui->lock_window();
//printf("HistogramSlider::cursor_motion_event 3\n");
		return 1;
	}
	return 0;
}

void HistogramSlider::update()
{
	int w = get_w();
	int h = get_h();
	int half_h = get_h() / 2;
	int quarter_h = get_h() / 4;
	int mode = plugin->config.mode;
	int r = 0xff;
	int g = 0xff;
	int b = 0xff;
	int subscript = plugin->config.mode;

	clear_box(0, 0, w, h);

	switch(mode)
	{
		case HISTOGRAM_RED:
			g = b = 0x00;
			break;
		case HISTOGRAM_GREEN:
			r = b = 0x00;
			break;
		case HISTOGRAM_BLUE:
			r = g = 0x00;
			break;
	}

	for(int i = 0; i < w; i++)
	{
		int color = (int)(i * 0xff / w);
		set_color(((r * color / 0xff) << 16) | 
			((g * color / 0xff) << 8) | 
			(b * color / 0xff));

		if(is_input)
		{
			draw_line(i, quarter_h, i, half_h);
			color = (int)plugin->calculate_curve(i * 0xffff / w, 
				subscript);
			set_color(((r * color / 0xffff) << 16) | 
				((g * color / 0xffff) << 8) | 
				(b * color / 0xffff));
			draw_line(i, 0, i, quarter_h);
		}
		else
			draw_line(i, 0, i, half_h);

	}

	int min;
	int max;
	if(is_input)
	{
		
		draw_pixmap(gui->mid_picon,
			(int)(plugin->config.input_mid[subscript] * w / 0xffff) - 
				gui->mid_picon->get_w() / 2,
			half_h + 1);
		min = plugin->config.input_min[subscript];
		max = plugin->config.input_max[subscript];
	}
	else
	{
		min = plugin->config.output_min[subscript];
		max = plugin->config.output_max[subscript];
	}

	draw_pixmap(gui->min_picon,
		min * w / 0xffff - gui->min_picon->get_w() / 2,
		half_h + 1);
	draw_pixmap(gui->max_picon,
		max * w / 0xffff - gui->max_picon->get_w() / 2,
		half_h + 1);

// printf("HistogramSlider::update %d %d\n", min, max);
	flash();
	flush();
}









HistogramAuto::HistogramAuto(HistogramMain *plugin, 
	int x, 
	int y)
 : BC_CheckBox(x, y, plugin->config.automatic, "Automatic")
{
	this->plugin = plugin;
}

int HistogramAuto::handle_event()
{
	plugin->config.automatic = get_value();
	plugin->send_configure_change();
	return 1;
}




HistogramMode::HistogramMode(HistogramMain *plugin, 
	int x, 
	int y,
	int value,
	char *text)
 : BC_Radial(x, y, plugin->config.mode == value, text)
{
	this->plugin = plugin;
	this->value = value;
}
int HistogramMode::handle_event()
{
	plugin->config.mode = value;
	plugin->thread->window->update_mode();
	plugin->thread->window->input->update();
	plugin->thread->window->output->update();
	plugin->send_configure_change();
	return 1;
}









HistogramText::HistogramText(HistogramMain *plugin,
	HistogramWindow *gui,
	int x,
	int y,
	int *output)
 : BC_TumbleTextBox(gui, 
		(int64_t)*output,
		(int64_t)0,
		(int64_t)0xff,
		x, 
		y, 
		60)
{
	this->plugin = plugin;
	this->output = output;
}


int HistogramText::handle_event()
{
	if(output)
	{
		*output = atol(get_text());
	}
	plugin->thread->window->input->update();
	plugin->thread->window->output->update();
	plugin->send_configure_change();
	return 1;
}






















HistogramMain::HistogramMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
	lookup[0] = lookup[1] = lookup[2] = lookup[3] = 0;
	accum[0] = accum[1] = accum[2] = accum[3] = accum[3] = 0;
}

HistogramMain::~HistogramMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(lookup[0]) delete [] lookup[0];
	if(lookup[1]) delete [] lookup[1];
	if(lookup[2]) delete [] lookup[2];
	if(lookup[3]) delete [] lookup[3];
	if(accum[0]) delete [] accum[0];
	if(accum[1]) delete [] accum[1];
	if(accum[2]) delete [] accum[2];
	if(accum[3]) delete [] accum[3];
	if(accum[4]) delete [] accum[4];
	if(engine) delete engine;
}

char* HistogramMain::plugin_title() { return "Histogram"; }
int HistogramMain::is_realtime() { return 1; }


NEW_PICON_MACRO(HistogramMain)

SHOW_GUI_MACRO(HistogramMain, HistogramThread)

SET_STRING_MACRO(HistogramMain)

RAISE_WINDOW_MACRO(HistogramMain)

LOAD_CONFIGURATION_MACRO(HistogramMain, HistogramConfig)

void HistogramMain::render_gui(void *data)
{

	if(thread)
	{
//printf("HistogramMain::render_gui 1\n");
		thread->window->lock_window();
//printf("HistogramMain::render_gui 2\n");
		calculate_histogram((VFrame*)data);
//printf("HistogramMain::render_gui 3\n");
		if(config.automatic)
		{
			calculate_automatic((VFrame*)data);
		}
//printf("HistogramMain::render_gui 3\n");

		thread->window->update_canvas();
//printf("HistogramMain::render_gui 3\n");
		if(config.automatic)
		{
			thread->window->update_input();
		}
//printf("HistogramMain::render_gui 3\n");
		thread->window->unlock_window();
	}
//printf("HistogramMain::render_gui 4\n");
}

void HistogramMain::update_gui()
{
	if(thread)
	{
		thread->window->lock_window();
		int reconfigure = load_configuration();
		if(reconfigure) 
		{
			thread->window->update(0);
			if(!config.automatic)
			{
				thread->window->update_input();
			}
		}
		thread->window->unlock_window();
	}
}


int HistogramMain::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%shistogram.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	for(int i = 0; i < 5; i++)
	{
		sprintf(string, "INPUT_MIN_%d", i);
		config.input_min[i] = defaults->get(string, config.input_min[i]);
		sprintf(string, "INPUT_MID_%d", i);
		config.input_mid[i] = defaults->get(string, config.input_mid[i]);
		sprintf(string, "INPUT_MAX_%d", i);
		config.input_max[i] = defaults->get(string, config.input_max[i]);
		sprintf(string, "OUTPUT_MIN_%d", i);
		config.output_min[i] = defaults->get(string, config.output_min[i]);
		sprintf(string, "OUTPUT_MAX_%d", i);
		config.output_max[i] = defaults->get(string, config.output_max[i]);
//printf("HistogramMain::load_defaults %d %f %d\n", config.input_min[i], config.input_mid[i], config.input_max[i]);
	}
	config.automatic = defaults->get("AUTOMATIC", config.automatic);
	config.mode = defaults->get("MODE", config.mode);
	config.threshold = defaults->get("THRESHOLD", config.threshold);
	return 0;
}


int HistogramMain::save_defaults()
{
	char string[BCTEXTLEN];
//printf("HistogramMain::save_defaults 1 %p\n", defaults);
	for(int i = 0; i < 5; i++)
	{
//printf("HistogramMain::save_defaults 1 %d\n", i);
		sprintf(string, "INPUT_MIN_%d", i);
		defaults->update(string, config.input_min[i]);
//printf("HistogramMain::save_defaults 1 %d\n", i);
		sprintf(string, "INPUT_MID_%d", i);
		defaults->update(string, config.input_mid[i]);
//printf("HistogramMain::save_defaults 1 %d\n", i);
		sprintf(string, "INPUT_MAX_%d", i);
//printf("HistogramMain::save_defaults 1 %d\n", config.input_max[i]);
		defaults->update(string, config.input_max[i]);
//printf("HistogramMain::save_defaults 1 %d\n", i);
		sprintf(string, "OUTPUT_MIN_%d", i);
		defaults->update(string, config.output_min[i]);
//printf("HistogramMain::save_defaults 1 %d\n", i);
		sprintf(string, "OUTPUT_MAX_%d", i);
	   	defaults->update(string, config.output_max[i]);
//printf("HistogramMain::save_defaults %d %f %d\n", config.input_min[i], config.input_mid[i], config.input_max[i]);
	}
//printf("HistogramMain::save_defaults 3\n");
	defaults->update("AUTOMATIC", config.automatic);
//printf("HistogramMain::save_defaults 4\n");
	defaults->update("MODE", config.mode);
	defaults->update("THRESHOLD", config.threshold);
//printf("HistogramMain::save_defaults 5\n");
	defaults->save();
//printf("HistogramMain::save_defaults 6\n");
	return 0;
}



void HistogramMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("HISTOGRAM");

	char string[BCTEXTLEN];
	for(int i = 0; i < 5; i++)
	{
		sprintf(string, "INPUT_MIN_%d", i);
		output.tag.set_property(string, config.input_min[i]);
		sprintf(string, "INPUT_MID_%d", i);
		output.tag.set_property(string, config.input_mid[i]);
		sprintf(string, "INPUT_MAX_%d", i);
		output.tag.set_property(string, config.input_max[i]);
		sprintf(string, "OUTPUT_MIN_%d", i);
		output.tag.set_property(string, config.output_min[i]);
		sprintf(string, "OUTPUT_MAX_%d", i);
	   	output.tag.set_property(string, config.output_max[i]);
//printf("HistogramMain::save_data %d %f %d\n", config.input_min[i], config.input_mid[i], config.input_max[i]);
	}
	output.tag.set_property("AUTOMATIC", config.automatic);
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.append_tag();
	output.terminate_string();
}

void HistogramMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("HISTOGRAM"))
			{
				char string[BCTEXTLEN];
				for(int i = 0; i < 5; i++)
				{
					sprintf(string, "INPUT_MIN_%d", i);
					config.input_min[i] = input.tag.get_property(string, config.input_min[i]);
					sprintf(string, "INPUT_MID_%d", i);
					config.input_mid[i] = input.tag.get_property(string, config.input_mid[i]);
					sprintf(string, "INPUT_MAX_%d", i);
					config.input_max[i] = input.tag.get_property(string, config.input_max[i]);
					sprintf(string, "OUTPUT_MIN_%d", i);
					config.output_min[i] = input.tag.get_property(string, config.output_min[i]);
					sprintf(string, "OUTPUT_MAX_%d", i);
					config.output_max[i] = input.tag.get_property(string, config.output_max[i]);
//printf("HistogramMain::read_data %d %f %d\n", config.input_min[i], config.input_mid[i], config.input_max[i]);
				}
				config.automatic = input.tag.get_property("AUTOMATIC", config.automatic);
				config.mode = input.tag.get_property("MODE", config.mode);
				config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
			}
		}
	}
}

float HistogramMain::calculate_curve(float input, int subscript)
{
	float y1, y2, y3, y4;
	float min = (float)config.input_min[subscript];
	float max = (float)config.input_max[subscript];
	float mid = (float)config.input_mid[subscript];
	float half = (float)HISTOGRAM_RANGE / 2;
	float output, output_perfect;
	float control = 1.0 / M_PI;
	float output_linear;

// Below minimum
	if(input < min) return 0;

// Above maximum
	if(input >= max) return HISTOGRAM_RANGE - 1;

	float slope1 = half / (mid - min);
	float slope2 = half / (max - mid);
	float min_slope = MIN(slope1, slope2);

// value of 45` diagonal with midpoint
	output_perfect = half + min_slope * (input - mid);
// Left hand side
	if(input < mid)
	{
// Fraction of perfect diagonal to use
		float mid_fraction = (input - min) / (mid - min);
// value of line connecting min to mid
		output_linear = mid_fraction * half;
// Blend perfect diagonal with linear
		output = output_linear * (1.0 - mid_fraction) + output_perfect * mid_fraction;
	}
	else
	{
// Fraction of perfect diagonal to use
		float mid_fraction = (max - input) / (max - mid);
// value of line connecting max to mid
		output_linear = half + (1.0 - mid_fraction) * half;
// Blend perfect diagonal with linear
		output = output_linear * (1.0 - mid_fraction) + output_perfect * mid_fraction;
	}







// printf("HistogramMain::calculate_curve 1 %.0f %.0f %.0f %.0f %.0f\n",
// output, 
// input,
// min,
// mid,
// max);
	return output;
}

void HistogramMain::calculate_histogram(VFrame *data)
{
	if(!engine) engine = new HistogramEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);

	if(!accum[0])
	{
		for(int i = 0; i < 5; i++)
			accum[i] = new int64_t[HISTOGRAM_RANGE];
	}

	engine->process_packages(HistogramEngine::HISTOGRAM, data);

	for(int i = 0; i < engine->get_total_clients(); i++)
	{
		HistogramUnit *unit = (HistogramUnit*)engine->get_client(i);
		if(i == 0)
		{
			for(int j = 0; j < 5; j++)
				memcpy(accum[j], unit->accum[j], sizeof(int64_t) * HISTOGRAM_RANGE);
		}
		else
		{
			for(int j = 0; j < 5; j++)
			{
				int64_t *out = accum[j];
				int64_t *in = unit->accum[j];
				for(int k = 0; k < HISTOGRAM_RANGE; k++)
					out[k] += in[k];
			}
		}
	}

// Remove top and bottom from calculations.  Doesn't work in high
// precision colormodels.
	for(int i = 0; i < 5; i++)
	{
		accum[i][0] = 0;
		accum[i][HISTOGRAM_RANGE - 1] = 0;
	}
}


void HistogramMain::calculate_automatic(VFrame *data)
{
	calculate_histogram(data);


	for(int i = 0; i < 3; i++)
	{
		int64_t *accum = this->accum[i];
		int max = 0;

		for(int j = 0; j < HISTOGRAM_RANGE; j++)
		{
			max = MAX(accum[j], max);
		}

		int threshold = config.threshold * max / THRESHOLD_SCALE;


// Minimums
		config.input_min[i] = 0;
		for(int j = 0; j < HISTOGRAM_RANGE; j++)
		{
			if(accum[j] > threshold)
			{
				config.input_min[i] = j;
				break;
			}
		}


// Maximums
		config.input_max[i] = 0xffff;
		for(int j = HISTOGRAM_RANGE - 1; j >= 0; j--)
		{
			if(accum[j] > threshold)
			{
				config.input_max[i] = j;
				break;
			}
		}

		config.input_mid[i] = (config.input_min[i] + config.input_max[i]) / 2;
	}
}






int HistogramMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
//printf("HistogramMain::process_realtime 1\n");
TRON("HistogramMain::process_realtime");
//printf("HistogramMain::process_realtime 1\n");
	int need_reconfigure = load_configuration();


	if(!engine) engine = new HistogramEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);
	this->input = input_ptr;
	this->output = output_ptr;

//printf("HistogramMain::process_realtime 1\n");
	send_render_gui(input_ptr);
//printf("HistogramMain::process_realtime 1\n");

	if(input_ptr->get_rows()[0] != output_ptr->get_rows()[0])
	{
		output_ptr->copy_from(input_ptr);
	}
//printf("HistogramMain::process_realtime 1\n");

// Generate tables here.  The same table is used by many packages to render
// each horizontal stripe.  Need to cover the entire output range in  each
// table to avoid green borders
	if(need_reconfigure || !lookup[0] || config.automatic)
	{
		if(!lookup[0])
			for(int i = 0; i < 4; i++)
				lookup[i] = new int[HISTOGRAM_RANGE];

// Calculate new curves
		if(config.automatic)
		{
			calculate_automatic(input);
		}

		engine->process_packages(HistogramEngine::TABULATE, input);



// Convert 16 bit lookup table to 8 bits
		switch(input->get_color_model())
		{
			case BC_RGB888:
			case BC_RGBA8888:
				for(int i = 0; i < 0x100; i++)
				{
					int subscript = (i << 8) | i;
					lookup[0][i] = lookup[0][subscript];
					lookup[1][i] = lookup[1][subscript];
					lookup[2][i] = lookup[2][subscript];
					lookup[3][i] = lookup[3][subscript];
				}
				break;
		}
	}
//printf("HistogramMain::process_realtime 1\n");




// Apply histogram
	engine->process_packages(HistogramEngine::APPLY, input);

//printf("HistogramMain::process_realtime 100\n");


TROFF("HistogramMain::process_realtime");

	return 0;
}






HistogramPackage::HistogramPackage()
 : LoadPackage()
{
}




HistogramUnit::HistogramUnit(HistogramEngine *server, 
	HistogramMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
	bzero(accum, sizeof(accum));
}

HistogramUnit::~HistogramUnit()
{
	if(accum[0]) delete [] accum[0];
	if(accum[1]) delete [] accum[1];
	if(accum[2]) delete [] accum[2];
	if(accum[3]) delete [] accum[3];
	if(accum[4]) delete [] accum[4];
}

void HistogramUnit::process_package(LoadPackage *package)
{
	HistogramPackage *pkg = (HistogramPackage*)package;

	if(server->operation == HistogramEngine::HISTOGRAM)
	{

#define HISTOGRAM_HEAD(type) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		type *row = (type*)data->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{

#define HISTOGRAM_TAIL(components) \
			v = MAX(r, g); \
			v = MAX(v, b); \
			accum_r[r]++; \
			accum_g[g]++; \
			accum_b[b]++; \
			if(components == 4) accum_a[row[3]]++; \
			accum_v[v]++; \
			row += components; \
		} \
	} \
}


		if(!accum[0])
		{
			for(int i = 0; i < 5; i++)
				accum[i] = new int64_t[HISTOGRAM_RANGE];
		}

		for(int i = 0; i < 5; i++)
			bzero(accum[i], sizeof(int64_t) * HISTOGRAM_RANGE);

		VFrame *data = server->data;
		int w = data->get_w();
		int h = data->get_h();
		int64_t *accum_r = accum[HISTOGRAM_RED];
		int64_t *accum_g = accum[HISTOGRAM_GREEN];
		int64_t *accum_b = accum[HISTOGRAM_BLUE];
		int64_t *accum_a = accum[HISTOGRAM_ALPHA];
		int64_t *accum_v = accum[HISTOGRAM_VALUE];
		int r, g, b, a, y, u, v;

		switch(data->get_color_model())
		{
			case BC_RGB888:
				HISTOGRAM_HEAD(unsigned char)
				r = (row[0] << 8) | row[0];
				g = (row[1] << 8) | row[1];
				b = (row[2] << 8) | row[2];
				HISTOGRAM_TAIL(3)
				break;
			case BC_YUV888:
				HISTOGRAM_HEAD(unsigned char)
				y = (row[0] << 8) | row[0];
				u = (row[1] << 8) | row[1];
				v = (row[2] << 8) | row[2];
				plugin->yuv.yuv_to_rgb_16(r, g, b, y, u, v);
				HISTOGRAM_TAIL(3)
				break;
			case BC_RGBA8888:
				HISTOGRAM_HEAD(unsigned char)
				r = (row[0] << 8) | row[0];
				g = (row[1] << 8) | row[1];
				b = (row[2] << 8) | row[2];
				HISTOGRAM_TAIL(4)
				break;
			case BC_YUVA8888:
				HISTOGRAM_HEAD(unsigned char)
				y = (row[0] << 8) | row[0];
				u = (row[1] << 8) | row[1];
				v = (row[2] << 8) | row[2];
				plugin->yuv.yuv_to_rgb_16(r, g, b, y, u, v);
				HISTOGRAM_TAIL(4)
				break;
			case BC_RGB161616:
				HISTOGRAM_HEAD(uint16_t)
				r = row[0];
				g = row[1];
				b = row[2];
				HISTOGRAM_TAIL(3)
				break;
			case BC_YUV161616:
				HISTOGRAM_HEAD(uint16_t)
				y = row[0];
				u = row[1];
				v = row[2];
				plugin->yuv.yuv_to_rgb_16(r, g, b, y, u, v);
				HISTOGRAM_TAIL(3)
				break;
			case BC_RGBA16161616:
				HISTOGRAM_HEAD(uint16_t)
				r = row[0];
				g = row[1];
				b = row[2];
				HISTOGRAM_TAIL(3)
				break;
			case BC_YUVA16161616:
				HISTOGRAM_HEAD(uint16_t)
				y = row[0];
				u = row[1];
				v = row[2];
				plugin->yuv.yuv_to_rgb_16(r, g, b, y, u, v);
				HISTOGRAM_TAIL(4)
				break;
		}
	}
	else
	if(server->operation == HistogramEngine::APPLY)
	{



#define PROCESS(type, components) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		type *row = (type*)input->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			row[0] = lookup_r[row[0]]; \
			row[1] = lookup_g[row[1]]; \
			row[2] = lookup_b[row[2]]; \
			if(components == 4) row[3] = lookup_a[row[3]]; \
			row += components; \
		} \
	} \
}

#define PROCESS_YUV(type, components, max) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		type *row = (type*)input->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
/* Convert to 16 bit RGB */ \
			if(max == 0xff) \
			{ \
				y = (row[0] << 8) | row[0]; \
				u = (row[1] << 8) | row[1]; \
				v = (row[2] << 8) | row[2]; \
				if(components == 4) a = (row[3] << 8) | row[3]; \
			} \
			else \
			{ \
				y = row[0]; \
				u = row[1]; \
				v = row[2]; \
				if(components == 4) a = row[3]; \
			} \
 \
			plugin->yuv.yuv_to_rgb_16(r, g, b, y, u, v); \
 \
/* Look up in RGB domain */ \
			r = lookup_r[r]; \
			g = lookup_g[g]; \
			b = lookup_b[b]; \
			if(components == 4) a = lookup_a[a]; \
 \
/* Convert to 16 bit YUV */ \
			plugin->yuv.rgb_to_yuv_16(r, g, b, y, u, v); \
 \
			if(max == 0xff) \
			{ \
				row[0] = y >> 8; \
				row[1] = u >> 8; \
				row[2] = v >> 8; \
				if(components == 4) row[3] = a >> 8; \
			} \
			else \
			{ \
				row[0] = y; \
				row[1] = u; \
				row[2] = v; \
				if(components == 4) row[3] = a; \
			} \
			row += components; \
		} \
	} \
}




		VFrame *input = plugin->input;
		VFrame *output = plugin->output;
		int w = input->get_w();
		int h = input->get_h();
		int *lookup_r = plugin->lookup[0];
		int *lookup_g = plugin->lookup[1];
		int *lookup_b = plugin->lookup[2];
		int *lookup_a = plugin->lookup[3];
		int r, g, b, y, u, v, a;
		switch(input->get_color_model())
		{
			case BC_RGB888:
				PROCESS(unsigned char, 3)
				break;
			case BC_RGBA8888:
				PROCESS(unsigned char, 4)
				break;
			case BC_RGB161616:
				PROCESS(uint16_t, 3)
				break;
			case BC_RGBA16161616:
				PROCESS(uint16_t, 4)
				break;
			case BC_YUV888:
				PROCESS_YUV(unsigned char, 3, 0xff)
				break;
			case BC_YUVA8888:
				PROCESS_YUV(unsigned char, 4, 0xff)
				break;
			case BC_YUV161616:
				PROCESS_YUV(uint16_t, 3, 0xffff)
				break;
			case BC_YUVA16161616:
				PROCESS_YUV(uint16_t, 4, 0xffff)
				break;
		}
	}
	if(server->operation == HistogramEngine::TABULATE)
	{
// Do conversion in 16 bit YUVA
		int min_output_r = plugin->config.output_min[0];
		int min_output_g = plugin->config.output_min[1];
		int min_output_b = plugin->config.output_min[2];
		int min_output_a = plugin->config.output_min[3];
		int min_output_v = plugin->config.output_min[4];
		int max_output_r = plugin->config.output_max[0];
		int max_output_g = plugin->config.output_max[1];
		int max_output_b = plugin->config.output_max[2];
		int max_output_a = plugin->config.output_max[3];
		int max_output_v = plugin->config.output_max[4];
		int colormodel = plugin->input->get_color_model();

		for(int i = pkg->start; i < pkg->end; i++)
		{
// Expand input
			float r = plugin->calculate_curve((float)i, HISTOGRAM_RED);
			float g = plugin->calculate_curve((float)i, HISTOGRAM_GREEN);
			float b = plugin->calculate_curve((float)i, HISTOGRAM_BLUE);
			float a = plugin->calculate_curve((float)i, HISTOGRAM_ALPHA);
			int y, u, v;
// Expand value
			r = plugin->calculate_curve((float)r, HISTOGRAM_VALUE);
			g = plugin->calculate_curve((float)g, HISTOGRAM_VALUE);
			b = plugin->calculate_curve((float)b, HISTOGRAM_VALUE);
// r = i;
// g = i;
// b = i;

// Shrink output
			r = (float)min_output_r + 
				r * 
				(max_output_r - min_output_r) / 
				(HISTOGRAM_RANGE - 1);
			g = min_output_g + 
				g * 
				(max_output_g - min_output_g) / 
				(HISTOGRAM_RANGE - 1);
			b = min_output_b + 
				b * 
				(max_output_b - min_output_b) / 
				(HISTOGRAM_RANGE - 1);
			a = min_output_a + 
				a * 
				(max_output_a - min_output_a) / 
				(HISTOGRAM_RANGE - 1);
// Shrink value
//printf(" 1 %d -> ", r);
			r = min_output_v + 
				r * 
				(max_output_v - min_output_v) / 
				(HISTOGRAM_RANGE - 1);
//printf("%d\n", r);
			g = min_output_v + 
				g * 
				(max_output_v - min_output_v) / 
				(HISTOGRAM_RANGE - 1);
			b = min_output_v + 
				b * 
				(max_output_v - min_output_v) / 
				(HISTOGRAM_RANGE - 1);
			a = min_output_v + 
				a * 
				(max_output_v - min_output_v) / 
				(HISTOGRAM_RANGE - 1);



// Convert to desired colormodel
			switch(colormodel)
			{
				case BC_RGB888:
				case BC_RGBA8888:
					plugin->lookup[0][i] = ((int)r) >> 8;
					plugin->lookup[1][i] = ((int)g) >> 8;
					plugin->lookup[2][i] = ((int)b) >> 8;
					plugin->lookup[3][i] = ((int)a) >> 8;
					break;
				default:
// Can't look up yuv.
					plugin->lookup[0][i] = (int)r;
					plugin->lookup[1][i] = (int)g;
					plugin->lookup[2][i] = (int)b;
					plugin->lookup[3][i] = (int)a;
					break;
			}
		}
	}
}






HistogramEngine::HistogramEngine(HistogramMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void HistogramEngine::init_packages()
{
	int total_size;

	switch(operation)
	{
		case HISTOGRAM:
			total_size = data->get_h();
			break;
		case TABULATE:
			total_size = HISTOGRAM_RANGE;
			break;
		case APPLY:
			total_size = data->get_h();
			break;
	}
	int package_size = (int)((float)total_size / 
			total_packages + 1);
	int start = 0;
	for(int i = 0; i < total_packages; i++)
	{
		HistogramPackage *package = (HistogramPackage*)packages[i];
		package->start = start;
		package->end = start + package_size;
		if(package->end > total_size)
			package->end = total_size;
		start = package->end;
	}
}

LoadClient* HistogramEngine::new_client()
{
	return new HistogramUnit(this, plugin);
}

LoadPackage* HistogramEngine::new_package()
{
	return new HistogramPackage;
}

void HistogramEngine::process_packages(int operation, VFrame *data)
{
	this->data = data;
	this->operation = operation;
	LoadServer::process_packages();
}



