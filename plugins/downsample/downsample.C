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


class DownSampleMain;
class DownSampleServer;





class DownSampleConfig
{
public:
	DownSampleConfig();

	int equivalent(DownSampleConfig &that);
	void copy_from(DownSampleConfig &that);
	void interpolate(DownSampleConfig &prev, 
		DownSampleConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	int horizontal;
	int vertical;
	int r;
	int g;
	int b;
	int a;
};


class DownSampleToggle : public BC_CheckBox
{
public:
	DownSampleToggle(DownSampleMain *plugin, 
		int x, 
		int y, 
		int *output, 
		char *string);
	int handle_event();
	DownSampleMain *plugin;
	int *output;
};

class DownSampleSize : public BC_ISlider
{
public:
	DownSampleSize(DownSampleMain *plugin, 
		int x, 
		int y, 
		int *output);
	int handle_event();
	DownSampleMain *plugin;
	int *output;
};

class DownSampleWindow : public BC_Window
{
public:
	DownSampleWindow(DownSampleMain *plugin, int x, int y);
	~DownSampleWindow();
	
	int create_objects();
	int close_event();

	DownSampleToggle *r, *g, *b, *a;
	DownSampleSize *h, *v;
	DownSampleMain *plugin;
};



PLUGIN_THREAD_HEADER(DownSampleMain, DownSampleThread, DownSampleWindow)


class DownSampleMain : public PluginVClient
{
public:
	DownSampleMain(PluginServer *server);
	~DownSampleMain();

	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	PLUGIN_CLASS_MEMBERS(DownSampleConfig, DownSampleThread)

	VFrame *input, *output;
	DownSampleServer *engine;
};

class DownSamplePackage : public LoadPackage
{
public:
	DownSamplePackage();
	int y1, y2;
};

class DownSampleUnit : public LoadClient
{
public:
	DownSampleUnit(DownSampleServer *server, DownSampleMain *plugin);
	void process_package(LoadPackage *package);
	DownSampleServer *server;
	DownSampleMain *plugin;
};

class DownSampleServer : public LoadServer
{
public:
	DownSampleServer(DownSampleMain *plugin, 
		int total_clients, 
		int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	DownSampleMain *plugin;
};

















REGISTER_PLUGIN(DownSampleMain)



DownSampleConfig::DownSampleConfig()
{
	horizontal = 2;
	vertical = 2;
	r = 1;
	g = 1;
	b = 1;
	a = 1;
}

int DownSampleConfig::equivalent(DownSampleConfig &that)
{
	return 
		horizontal == that.horizontal &&
		vertical == that.vertical &&
		r == that.r &&
		g == that.g &&
		b == that.b &&
		a == that.a;
}

void DownSampleConfig::copy_from(DownSampleConfig &that)
{
	horizontal = that.horizontal;
	vertical = that.vertical;
	r = that.r;
	g = that.g;
	b = that.b;
	a = that.a;
}

void DownSampleConfig::interpolate(DownSampleConfig &prev, 
	DownSampleConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->horizontal = (int)(prev.horizontal * prev_scale + next.horizontal * next_scale);
	this->vertical = (int)(prev.vertical * prev_scale + next.vertical * next_scale);
	r = prev.r;
	g = prev.g;
	b = prev.b;
	a = prev.a;
}









PLUGIN_THREAD_OBJECT(DownSampleMain, DownSampleThread, DownSampleWindow)



DownSampleWindow::DownSampleWindow(DownSampleMain *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x,
	y,
	230, 
	250, 
	230, 
	250, 
	0, 
	1)
{
	this->plugin = plugin; 
}

DownSampleWindow::~DownSampleWindow()
{
}

int DownSampleWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, "Horizontal"));
	y += 30;
	add_subwindow(h = new DownSampleSize(plugin, x, y, &plugin->config.horizontal));
	y += 30;
	add_subwindow(new BC_Title(x, y, "Vertical"));
	y += 30;
	add_subwindow(v = new DownSampleSize(plugin, x, y, &plugin->config.vertical));
	y += 30;
	add_subwindow(r = new DownSampleToggle(plugin, x, y, &plugin->config.r, "Red"));
	y += 30;
	add_subwindow(g = new DownSampleToggle(plugin, x, y, &plugin->config.g, "Green"));
	y += 30;
	add_subwindow(b = new DownSampleToggle(plugin, x, y, &plugin->config.b, "Blue"));
	y += 30;
	add_subwindow(a = new DownSampleToggle(plugin, x, y, &plugin->config.a, "Alpha"));
	y += 30;

	show_window();
	flush();
	return 0;
}

int DownSampleWindow::close_event()
{
// Set result to 1 to indicate a plugin side close
	set_done(1);
	return 1;
}










DownSampleToggle::DownSampleToggle(DownSampleMain *plugin, 
	int x, 
	int y, 
	int *output, 
	char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int DownSampleToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}







DownSampleSize::DownSampleSize(DownSampleMain *plugin, 
	int x, 
	int y, 
	int *output)
 : BC_ISlider(x, y, 0, 200, 200, 1, 100, *output)
{
	this->plugin = plugin;
	this->output = output;
}
int DownSampleSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}










DownSampleMain::DownSampleMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
}

DownSampleMain::~DownSampleMain()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(engine) delete engine;
}

char* DownSampleMain::plugin_title() { return "Downsample"; }
int DownSampleMain::is_realtime() { return 1; }


NEW_PICON_MACRO(DownSampleMain)

SHOW_GUI_MACRO(DownSampleMain, DownSampleThread)

SET_STRING_MACRO(DownSampleMain)

RAISE_WINDOW_MACRO(DownSampleMain)

LOAD_CONFIGURATION_MACRO(DownSampleMain, DownSampleConfig)


int DownSampleMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	this->input = input_ptr;
	this->output = output_ptr;
//printf("DownSampleMain::process_realtime 1\n");
	load_configuration();
//printf("DownSampleMain::process_realtime 1\n");

// Copy to destination
	if(output->get_rows()[0] != input->get_rows()[0])
	{
		output->copy_from(input);
	}
//printf("DownSampleMain::process_realtime 1\n");

// Process in destination
//printf("DownSampleMain::process_realtime 1\n");
	if(!engine) engine = new DownSampleServer(this,
		get_project_smp() + 1,
		get_project_smp() + 1);
	engine->process_packages();
//printf("DownSampleMain::process_realtime 2\n");

	return 0;
}


void DownSampleMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->h->update(config.horizontal);
		thread->window->v->update(config.vertical);
		thread->window->r->update(config.r);
		thread->window->g->update(config.g);
		thread->window->b->update(config.b);
		thread->window->a->update(config.a);
		thread->window->unlock_window();
	}
}


int DownSampleMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%sdownsample.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.horizontal = defaults->get("HORIZONTAL", config.horizontal);
	config.vertical = defaults->get("VERTICAL", config.vertical);
	config.r = defaults->get("R", config.r);
	config.g = defaults->get("G", config.g);
	config.b = defaults->get("B", config.b);
	config.a = defaults->get("A", config.a);
	return 0;
}


int DownSampleMain::save_defaults()
{
	defaults->update("HORIZONTAL", config.horizontal);
	defaults->update("VERTICAL", config.vertical);
	defaults->update("R", config.r);
	defaults->update("G", config.g);
	defaults->update("B", config.b);
	defaults->update("A", config.a);
	defaults->save();
	return 0;
}



void DownSampleMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("DOWNSAMPLE");

	output.tag.set_property("HORIZONTAL", config.horizontal);
	output.tag.set_property("VERTICAL", config.vertical);
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.terminate_string();
}

void DownSampleMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("DOWNSAMPLE"))
			{
				config.horizontal = input.tag.get_property("HORIZONTAL", config.horizontal);
				config.vertical = input.tag.get_property("VERTICAL", config.vertical);
				config.r = input.tag.get_property("R", config.r);
				config.g = input.tag.get_property("G", config.g);
				config.b = input.tag.get_property("B", config.b);
				config.a = input.tag.get_property("A", config.a);
			}
		}
	}
}






DownSamplePackage::DownSamplePackage()
 : LoadPackage()
{
}




DownSampleUnit::DownSampleUnit(DownSampleServer *server, 
	DownSampleMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}

#define SQR(x) ((x) * (x))


#define DOWNSAMPLE(type, components, max) \
{ \
	uint64_t r; \
	uint64_t g; \
	uint64_t b; \
	uint64_t a; \
	int do_r = plugin->config.r; \
	int do_g = plugin->config.g; \
	int do_b = plugin->config.b; \
	int do_a = plugin->config.a; \
	int vertical = MAX(plugin->config.vertical, 1); \
 \
	for(int i = pkg->y1; i < pkg->y2; i += vertical) \
	{ \
/*printf("DOWNSAMPLE 2 %d\n", i);*/ \
		type **row1 = (type**)plugin->output->get_rows() + i; \
		int horizontal = MAX(plugin->config.horizontal, 1); \
 \
 		if(pkg->y2 - i < vertical) \
		{ \
			vertical = pkg->y2 - i; \
		} \
 \
		int64_t scale = horizontal * vertical; \
		for(int j = 0; j < w; j += horizontal) \
		{ \
			if(w - j < horizontal) \
			{ \
				horizontal = w - j; \
				scale = horizontal * vertical; \
			} \
 \
/* Read in values */ \
			r = 0; \
			g = 0; \
			b = 0; \
			if(components == 4) a = 0; \
 \
			for(int k = 0; k < vertical; k++) \
			{ \
				type *row2 = row1[k] + j * components; \
				for(int l = 0; l < horizontal; l++) \
				{ \
					if(do_r) r += *row2++; else row2++; \
					if(do_g) g += *row2++; else row2++;  \
					if(do_b) b += *row2++; else row2++;  \
					if(components == 4) if(do_a) a += *row2++; else row2++;  \
				} \
			} \
 \
/* Write average */ \
			r /= scale; \
			g /= scale; \
			b /= scale; \
			if(components == 4) a /= scale; \
			for(int k = 0; k < vertical; k++) \
			{ \
				type *row2 = row1[k] + j * components; \
				for(int l = 0; l < horizontal; l++) \
				{ \
					if(do_r) *row2++ = r; else row2++; \
					if(do_g) *row2++ = g; else row2++; \
					if(do_b) *row2++ = b; else row2++; \
					if(components == 4) if(do_a) *row2++ = a; else row2++; \
				} \
			} \
		} \
/*printf("DOWNSAMPLE 3 %d\n", i);*/ \
	} \
}

void DownSampleUnit::process_package(LoadPackage *package)
{
	DownSamplePackage *pkg = (DownSamplePackage*)package;
	int h = plugin->output->get_h();
	int w = plugin->output->get_w();


	switch(plugin->input->get_color_model())
	{
		case BC_RGB888:
			DOWNSAMPLE(uint8_t, 3, 0xff)
			break;
		case BC_RGBA8888:
			DOWNSAMPLE(uint8_t, 4, 0xff)
			break;
		case BC_RGB161616:
			DOWNSAMPLE(uint16_t, 3, 0xffff)
			break;
		case BC_RGBA16161616:
			DOWNSAMPLE(uint16_t, 4, 0xffff)
			break;
		case BC_YUV888:
			DOWNSAMPLE(uint8_t, 3, 0xff)
			break;
		case BC_YUVA8888:
			DOWNSAMPLE(uint8_t, 4, 0xff)
			break;
		case BC_YUV161616:
			DOWNSAMPLE(uint16_t, 3, 0xffff)
			break;
		case BC_YUVA16161616:
			DOWNSAMPLE(uint16_t, 4, 0xffff)
			break;
	}
}






DownSampleServer::DownSampleServer(DownSampleMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void DownSampleServer::init_packages()
{
	int package_h = (int)((float)plugin->output->get_h() / 
			total_packages + 1);
	int y1 = 0;
	for(int i = 0; i < total_packages; i++)
	{
		DownSamplePackage *package = (DownSamplePackage*)packages[i];
		package->y1 = y1;
		package->y2 = y1 + 
			(int)((float)package_h / plugin->config.vertical + 1) *
			plugin->config.vertical;
		package->y1 = MIN(plugin->output->get_h(), package->y1);
		package->y2 = MIN(plugin->output->get_h(), package->y2);
		y1 = package->y2;
//printf("DownSampleServer::init_packages 1 %d %d %d\n",plugin->config.vertical, package->y1 , package->y2);
	}
}

LoadClient* DownSampleServer::new_client()
{
	return new DownSampleUnit(this, plugin);
}

LoadPackage* DownSampleServer::new_package()
{
	return new DownSamplePackage;
}





