#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "keyframe.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "vframe.h"


#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

class RadialBlurMain;
class RadialBlurEngine;





class RadialBlurConfig
{
public:
	RadialBlurConfig();

	int equivalent(RadialBlurConfig &that);
	void copy_from(RadialBlurConfig &that);
	void interpolate(RadialBlurConfig &prev, 
		RadialBlurConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	int x;
	int y;
	int steps;
	int angle;
	int r;
	int g;
	int b;
	int a;
};



class RadialBlurSize : public BC_ISlider
{
public:
	RadialBlurSize(RadialBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		int min,
		int max);
	int handle_event();
	RadialBlurMain *plugin;
	int *output;
};

class RadialBlurToggle : public BC_CheckBox
{
public:
	RadialBlurToggle(RadialBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		char *string);
	int handle_event();
	RadialBlurMain *plugin;
	int *output;
};

class RadialBlurWindow : public BC_Window
{
public:
	RadialBlurWindow(RadialBlurMain *plugin, int x, int y);
	~RadialBlurWindow();

	int create_objects();
	int close_event();

	RadialBlurSize *x, *y, *steps, *angle;
	RadialBlurToggle *r, *g, *b, *a;
	RadialBlurMain *plugin;
};



PLUGIN_THREAD_HEADER(RadialBlurMain, RadialBlurThread, RadialBlurWindow)


class RadialBlurMain : public PluginVClient
{
public:
	RadialBlurMain(PluginServer *server);
	~RadialBlurMain();

	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	PLUGIN_CLASS_MEMBERS(RadialBlurConfig, RadialBlurThread)

	VFrame *input, *output, *temp;
	RadialBlurEngine *engine;
};

class RadialBlurPackage : public LoadPackage
{
public:
	RadialBlurPackage();
	int y1, y2;
};

class RadialBlurUnit : public LoadClient
{
public:
	RadialBlurUnit(RadialBlurEngine *server, RadialBlurMain *plugin);
	void process_package(LoadPackage *package);
	RadialBlurEngine *server;
	RadialBlurMain *plugin;
};

class RadialBlurEngine : public LoadServer
{
public:
	RadialBlurEngine(RadialBlurMain *plugin, 
		int total_clients, 
		int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	RadialBlurMain *plugin;
};



















REGISTER_PLUGIN(RadialBlurMain)



RadialBlurConfig::RadialBlurConfig()
{
	x = 50;
	y = 50;
	steps = 10;
	angle = 33;
	r = 1;
	g = 1;
	b = 1;
	a = 1;
}

int RadialBlurConfig::equivalent(RadialBlurConfig &that)
{
	return 
		angle == that.angle &&
		x == that.x &&
		y == that.y &&
		steps == that.steps &&
		r == that.r &&
		g == that.g &&
		b == that.b &&
		a == that.a;
}

void RadialBlurConfig::copy_from(RadialBlurConfig &that)
{
	x = that.x;
	y = that.y;
	angle = that.angle;
	steps = that.steps;
	r = that.r;
	g = that.g;
	b = that.b;
	a = that.a;
}

void RadialBlurConfig::interpolate(RadialBlurConfig &prev, 
	RadialBlurConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->x = (int)(prev.x * prev_scale + next.x * next_scale + 0.5);
	this->y = (int)(prev.y * prev_scale + next.y * next_scale + 0.5);
	this->steps = (int)(prev.steps * prev_scale + next.steps * next_scale + 0.5);
	this->angle = (int)(prev.angle * prev_scale + next.angle * next_scale + 0.5);
	r = prev.r;
	g = prev.g;
	b = prev.b;
	a = prev.a;
}









PLUGIN_THREAD_OBJECT(RadialBlurMain, RadialBlurThread, RadialBlurWindow)



RadialBlurWindow::RadialBlurWindow(RadialBlurMain *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x,
	y,
	230, 
	340, 
	230, 
	340, 
	0, 
	1)
{
	this->plugin = plugin; 
}

RadialBlurWindow::~RadialBlurWindow()
{
}

int RadialBlurWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, _("X:")));
	y += 20;
	add_subwindow(this->x = new RadialBlurSize(plugin, x, y, &plugin->config.x, 0, 100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Y:")));
	y += 20;
	add_subwindow(this->y = new RadialBlurSize(plugin, x, y, &plugin->config.y, 0, 100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Angle:")));
	y += 20;
	add_subwindow(angle = new RadialBlurSize(plugin, x, y, &plugin->config.angle, 0, 360));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Steps:")));
	y += 20;
	add_subwindow(steps = new RadialBlurSize(plugin, x, y, &plugin->config.steps, 1, 100));
	y += 30;
	add_subwindow(r = new RadialBlurToggle(plugin, x, y, &plugin->config.r, _("Red")));
	y += 30;
	add_subwindow(g = new RadialBlurToggle(plugin, x, y, &plugin->config.g, _("Green")));
	y += 30;
	add_subwindow(b = new RadialBlurToggle(plugin, x, y, &plugin->config.b, _("Blue")));
	y += 30;
	add_subwindow(a = new RadialBlurToggle(plugin, x, y, &plugin->config.a, _("Alpha")));
	y += 30;

	show_window();
	flush();
	return 0;
}

int RadialBlurWindow::close_event()
{
// Set result to 1 to indicate a plugin side close
	set_done(1);
	return 1;
}










RadialBlurToggle::RadialBlurToggle(RadialBlurMain *plugin, 
	int x, 
	int y, 
	int *output, 
	char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int RadialBlurToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}







RadialBlurSize::RadialBlurSize(RadialBlurMain *plugin, 
	int x, 
	int y, 
	int *output,
	int min,
	int max)
 : BC_ISlider(x, y, 0, 200, 200, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
}
int RadialBlurSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}










RadialBlurMain::RadialBlurMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
	temp = 0;
}

RadialBlurMain::~RadialBlurMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(engine) delete engine;
	if(temp) delete temp;
}

char* RadialBlurMain::plugin_title() { return N_("Radial Blur"); }
int RadialBlurMain::is_realtime() { return 1; }


NEW_PICON_MACRO(RadialBlurMain)

SHOW_GUI_MACRO(RadialBlurMain, RadialBlurThread)

SET_STRING_MACRO(RadialBlurMain)

RAISE_WINDOW_MACRO(RadialBlurMain)

LOAD_CONFIGURATION_MACRO(RadialBlurMain, RadialBlurConfig)

int RadialBlurMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	load_configuration();

	if(!engine) engine = new RadialBlurEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);

	this->input = input_ptr;
	this->output = output_ptr;


	if(input_ptr->get_rows()[0] == output_ptr->get_rows()[0])
	{
		if(!temp) temp = new VFrame(0,
			input_ptr->get_w(),
			input_ptr->get_h(),
			input_ptr->get_color_model());
		temp->copy_from(input_ptr);
		this->input = temp;
	}

	engine->process_packages();
	return 0;
}


void RadialBlurMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->x->update(config.x);
		thread->window->y->update(config.y);
		thread->window->angle->update(config.angle);
		thread->window->steps->update(config.steps);
		thread->window->r->update(config.r);
		thread->window->g->update(config.g);
		thread->window->b->update(config.b);
		thread->window->a->update(config.a);
		thread->window->unlock_window();
	}
}


int RadialBlurMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%sradialblur.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.x = defaults->get("X", config.x);
	config.y = defaults->get("Y", config.y);
	config.angle = defaults->get("ANGLE", config.angle);
	config.steps = defaults->get("STEPS", config.steps);
	config.r = defaults->get("R", config.r);
	config.g = defaults->get("G", config.g);
	config.b = defaults->get("B", config.b);
	config.a = defaults->get("A", config.a);
	return 0;
}


int RadialBlurMain::save_defaults()
{
	defaults->update("X", config.x);
	defaults->update("Y", config.y);
	defaults->update("ANGLE", config.angle);
	defaults->update("STEPS", config.steps);
	defaults->update("R", config.r);
	defaults->update("G", config.g);
	defaults->update("B", config.b);
	defaults->update("A", config.a);
	defaults->save();
	return 0;
}



void RadialBlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("RADIALBLUR");

	output.tag.set_property("X", config.x);
	output.tag.set_property("Y", config.y);
	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("STEPS", config.steps);
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.terminate_string();
}

void RadialBlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("RADIALBLUR"))
			{
				config.x = input.tag.get_property("X", config.x);
				config.y = input.tag.get_property("Y", config.y);
				config.angle = input.tag.get_property("ANGLE", config.angle);
				config.steps = input.tag.get_property("STEPS", config.steps);
				config.r = input.tag.get_property("R", config.r);
				config.g = input.tag.get_property("G", config.g);
				config.b = input.tag.get_property("B", config.b);
				config.a = input.tag.get_property("A", config.a);
			}
		}
	}
}






RadialBlurPackage::RadialBlurPackage()
 : LoadPackage()
{
}


RadialBlurUnit::RadialBlurUnit(RadialBlurEngine *server, 
	RadialBlurMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}


#define BLEND_LAYER(COMPONENTS, TYPE, MAX, DO_YUV) \
{ \
	int chroma_offset = (DO_YUV ? ((MAX + 1) / 2) : 0); \
	TYPE **in_rows = (TYPE**)plugin->input->get_rows(); \
	TYPE **out_rows = (TYPE**)plugin->output->get_rows(); \
	int steps = plugin->config.steps; \
	double step = (double)plugin->config.angle / 360 * 2 * M_PI / steps; \
 \
	for(int i = pkg->y1, out_y = pkg->y1 - center_y; \
		i < pkg->y2; \
		i++, out_y++) \
	{ \
		TYPE *out_row = out_rows[i]; \
		TYPE *in_row = in_rows[i]; \
		int y_square = out_y * out_y; \
 \
		for(int j = 0, out_x = -center_x; j < w; j++, out_x++) \
		{ \
			double offset = 0; \
			int accum_r = 0; \
			int accum_g = 0; \
			int accum_b = 0; \
			int accum_a = 0; \
 \
/* Output coordinate to polar */ \
			double magnitude = sqrt(y_square + out_x * out_x); \
			double angle; \
			if(out_y < 0) \
				angle = atan((double)out_x / out_y) + M_PI; \
			else \
			if(out_y > 0) \
				angle = atan((double)out_x / out_y); \
			else \
			if(out_x > 0) \
				angle = M_PI / 2; \
			else \
				angle = M_PI * 1.5; \
 \
/* Overlay all steps on this pixel*/ \
			angle -= (double)plugin->config.angle / 360 * M_PI; \
			for(int k = 0; k < steps; k++, angle += step) \
			{ \
/* Polar to input coordinate */ \
				int in_x = (int)(magnitude * sin(angle)) + center_x; \
				int in_y = (int)(magnitude * cos(angle)) + center_y; \
 \
/* Accumulate input coordinate */ \
				if(in_x >= 0 && in_x < w && in_y >= 0 && in_y < h) \
				{ \
					accum_r += in_rows[in_y][in_x * COMPONENTS]; \
					if(DO_YUV) \
					{ \
						accum_g += (int)in_rows[in_y][in_x * COMPONENTS + 1]; \
						accum_b += (int)in_rows[in_y][in_x * COMPONENTS + 2]; \
					} \
					else \
					{ \
						accum_g += in_rows[in_y][in_x * COMPONENTS + 1]; \
						accum_b += in_rows[in_y][in_x * COMPONENTS + 2]; \
					} \
					if(COMPONENTS == 4) \
						accum_a += in_rows[in_y][in_x * COMPONENTS + 3]; \
				} \
				else \
				{ \
					accum_g += chroma_offset; \
					accum_b += chroma_offset; \
				} \
			} \
 \
/* Accumulation to output */ \
			if(do_r) \
			{ \
				*out_row++ = (accum_r * fraction) >> 16; \
				in_row++; \
			} \
			else \
			{ \
				*out_row++ = *in_row++; \
			} \
 \
			if(do_g) \
			{ \
				if(DO_YUV) \
					*out_row++ = ((accum_g * fraction) >> 16); \
				else \
					*out_row++ = (accum_g * fraction) >> 16; \
				in_row++; \
			} \
			else \
			{ \
				*out_row++ = *in_row++; \
			} \
 \
			if(do_b) \
			{ \
				if(DO_YUV) \
					*out_row++ = ((accum_b * fraction) >> 16); \
				else \
					*out_row++ = (accum_b * fraction) >> 16; \
				in_row++; \
			} \
			else \
			{ \
				*out_row++ = *in_row++; \
			} \
 \
			if(COMPONENTS == 4) \
			{ \
				if(do_a) \
				{ \
					*out_row++ = (accum_a * fraction) >> 16; \
					in_row++; \
				} \
				else \
				{ \
					*out_row++ = *in_row++; \
				} \
			} \
		} \
	} \
}

void RadialBlurUnit::process_package(LoadPackage *package)
{
	RadialBlurPackage *pkg = (RadialBlurPackage*)package;
	int h = plugin->output->get_h();
	int w = plugin->output->get_w();
	int do_r = plugin->config.r;
	int do_g = plugin->config.g;
	int do_b = plugin->config.b;
	int do_a = plugin->config.a;
	int fraction = 0x10000 / plugin->config.steps;
	int center_x = plugin->config.x * w / 100;
	int center_y = plugin->config.y * h / 100;

	switch(plugin->input->get_color_model())
	{
		case BC_RGB888:
			BLEND_LAYER(3, uint8_t, 0xff, 0)
			break;
		case BC_RGBA8888:
			BLEND_LAYER(4, uint8_t, 0xff, 0)
			break;
		case BC_RGB161616:
			BLEND_LAYER(3, uint16_t, 0xffff, 0)
			break;
		case BC_RGBA16161616:
			BLEND_LAYER(4, uint16_t, 0xffff, 0)
			break;
		case BC_YUV888:
			BLEND_LAYER(3, uint8_t, 0xff, 1)
			break;
		case BC_YUVA8888:
			BLEND_LAYER(4, uint8_t, 0xff, 1)
			break;
		case BC_YUV161616:
			BLEND_LAYER(3, uint16_t, 0xffff, 1)
			break;
		case BC_YUVA16161616:
			BLEND_LAYER(4, uint16_t, 0xffff, 1)
			break;
	}
}






RadialBlurEngine::RadialBlurEngine(RadialBlurMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
// : LoadServer(1, 1)
{
	this->plugin = plugin;
}

void RadialBlurEngine::init_packages()
{
	int package_h = (int)((float)plugin->output->get_h() / 
			total_packages + 1);
	int y1 = 0;
	for(int i = 0; i < total_packages; i++)
	{
		RadialBlurPackage *package = (RadialBlurPackage*)packages[i];
		package->y1 = y1;
		package->y2 = y1 + package_h;
		package->y1 = MIN(plugin->output->get_h(), package->y1);
		package->y2 = MIN(plugin->output->get_h(), package->y2);
		y1 = package->y2;
	}
}

LoadClient* RadialBlurEngine::new_client()
{
	return new RadialBlurUnit(this, plugin);
}

LoadPackage* RadialBlurEngine::new_package()
{
	return new RadialBlurPackage;
}





