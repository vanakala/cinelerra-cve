#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>



// Algorithm by Torsten Martinsen
// Ported to Cinelerra by Heroine Virtual Ltd.




class OilEffect;



class OilConfig
{
public:
	OilConfig();
	void copy_from(OilConfig &src);
	int equivalent(OilConfig &src);
	void interpolate(OilConfig &prev, 
		OilConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);
	float radius;
	int use_intensity;
};

class OilRadius : public BC_FSlider
{
public:
	OilRadius(OilEffect *plugin, int x, int y);
	int handle_event();
	OilEffect *plugin;
};


class OilIntensity : public BC_CheckBox
{
public:
	OilIntensity(OilEffect *plugin, int x, int y);
	int handle_event();
	OilEffect *plugin;
};

class OilWindow : public BC_Window
{
public:
	OilWindow(OilEffect *plugin, int x, int y);
	~OilWindow();
	void create_objects();
	int close_event();
	OilEffect *plugin;
	OilRadius *radius;
	OilIntensity *intensity;
};

PLUGIN_THREAD_HEADER(OilEffect, OilThread, OilWindow)





class OilServer : public LoadServer
{
public:
	OilServer(OilEffect *plugin, int cpus);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	OilEffect *plugin;
};

class OilPackage : public LoadPackage
{
public:
	OilPackage();
	int row1, row2;
};

class OilUnit : public LoadClient
{
public:
	OilUnit(OilEffect *plugin, OilServer *server);
	void process_package(LoadPackage *package);
	OilEffect *plugin;
};









class OilEffect : public PluginVClient
{
public:
	OilEffect(PluginServer *server);
	~OilEffect();

	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int load_configuration();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int show_gui();
	int set_string();
	void raise_window();
	void update_gui();

	OilConfig config;
	VFrame *temp_frame;
	VFrame *input, *output;
	Defaults *defaults;
	OilThread *thread;
	OilServer *engine;
	int need_reconfigure;
};













OilConfig::OilConfig()
{
	radius = 5;
	use_intensity = 0;
}

void OilConfig::copy_from(OilConfig &src)
{
	this->radius = src.radius;
	this->use_intensity = src.use_intensity;
}

int OilConfig::equivalent(OilConfig &src)
{
	return (EQUIV(this->radius, src.radius) &&
		this->use_intensity == src.use_intensity);
}

void OilConfig::interpolate(OilConfig &prev, 
		OilConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->radius = prev.radius * prev_scale + next.radius * next_scale;
	this->use_intensity = prev.use_intensity;
// printf("OilConfig::interpolate prev_frame=%ld current_frame=%ld next_frame=%ld prev.radius=%f this->radius=%f next.radius=%f\n", 
// 	prev_frame, current_frame, next_frame, prev.radius, this->radius, next.radius);
}












OilRadius::OilRadius(OilEffect *plugin, int x, int y)
 : BC_FSlider(x, 
	   	y, 
		0,
		200,
		200, 
		(float)0, 
		(float)30,
		plugin->config.radius)
{
	this->plugin = plugin;
}
int OilRadius::handle_event()
{
	plugin->config.radius = get_value();
	plugin->send_configure_change();
	return 1;
}







OilIntensity::OilIntensity(OilEffect *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.use_intensity, "Use intensity")
{
	this->plugin = plugin;
}
int OilIntensity::handle_event()
{
	plugin->config.use_intensity = get_value();
	plugin->send_configure_change();
	return 1;
}







OilWindow::OilWindow(OilEffect *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	300, 
	160, 
	300, 
	160, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

OilWindow::~OilWindow()
{
}

void OilWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, "Radius:"));
	add_subwindow(radius = new OilRadius(plugin, x + 70, y));
	y += 40;
	add_subwindow(intensity = new OilIntensity(plugin, x, y));
	
	show_window();
	flush();
}

int OilWindow::close_event()
{
	set_done(1);
	return 1;
}



PLUGIN_THREAD_OBJECT(OilEffect, OilThread, OilWindow)




REGISTER_PLUGIN(OilEffect)




OilEffect::OilEffect(PluginServer *server)
 : PluginVClient(server)
{
	temp_frame = 0;
	need_reconfigure = 1;
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

OilEffect::~OilEffect()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(temp_frame) delete temp_frame;
	if(engine) delete engine;
}


int OilEffect::is_realtime()
{
	return 1;
}

char* OilEffect::plugin_title()
{
	return "Oil painting";
}

NEW_PICON_MACRO(OilEffect)

SHOW_GUI_MACRO(OilEffect, OilThread)

RAISE_WINDOW_MACRO(OilEffect)

SET_STRING_MACRO(OilEffect)

void OilEffect::update_gui()
{
	if(thread)
	{
		thread->window->lock_window();
		load_configuration();
//printf("OilEffect::update_gui 1 %ld %f\n", get_source_position(), config.radius);

		thread->window->radius->update(config.radius);
		thread->window->intensity->update(config.use_intensity);
		thread->window->unlock_window();
	}
}


LOAD_CONFIGURATION_MACRO(OilEffect, OilConfig)

int OilEffect::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%soilpainting.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.radius = defaults->get("RADIUS", config.radius);
	config.use_intensity = defaults->get("USE_INTENSITY", config.use_intensity);
	return 0;
}

int OilEffect::save_defaults()
{
	defaults->update("RADIUS", config.radius);
	defaults->update("USE_INTENSITY", config.use_intensity);
	defaults->save();
	return 0;
}

void OilEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("OIL_PAINTING");
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("USE_INTENSITY", config.use_intensity);
	output.append_tag();
	output.terminate_string();
}

void OilEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("OIL_PAINTING"))
		{
			config.radius = input.tag.get_property("RADIUS", config.radius);
			config.use_intensity = input.tag.get_property("USE_INTENSITY", config.use_intensity);
		}
	}
}


int OilEffect::process_realtime(VFrame *input, VFrame *output)
{
	need_reconfigure |= load_configuration();



//printf("OilEffect::process_realtime %f %d\n", config.radius, config.use_intensity);
	this->input = input;
	this->output = output;

	if(EQUIV(config.radius, 0))
	{
		if(input->get_rows()[0] != output->get_rows()[0])
			output->copy_from(input);
	}
	else
	{
		if(input->get_rows()[0] == output->get_rows()[0])
		{
			if(!temp_frame) temp_frame = new VFrame(0,
				input->get_w(),
				input->get_h(),
				input->get_color_model());
			temp_frame->copy_from(input);
			this->input = temp_frame;
		}
		
		
		if(!engine)
		{
			engine = new OilServer(this, (PluginClient::smp + 1));
		}
		
		engine->process_packages();
	}
	
	
	
	return 0;
}






OilPackage::OilPackage()
 : LoadPackage()
{
}






OilUnit::OilUnit(OilEffect *plugin, OilServer *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}


#define INTENSITY(p) ((unsigned int)(((p)[0]) * 77+ \
									((p)[1] * 150) + \
									((p)[2] * 29)) >> 8)


#define OIL_MACRO(type, max, components) \
{ \
	type *src, *dest; \
	type Val[components]; \
	type Cnt[components], Cnt2; \
	type Hist[components][max + 1], Hist2[max + 1]; \
	type **src_rows = (type**)plugin->input->get_rows(); \
 \
	for(int y1 = pkg->row1; y1 < pkg->row2; y1++) \
	{ \
		dest = (type*)plugin->output->get_rows()[y1]; \
 \
		if(!plugin->config.use_intensity) \
		{ \
			for(int x1 = 0; x1 < w; x1++) \
			{ \
				bzero(Cnt, sizeof(Cnt)); \
				bzero(Val, sizeof(Val)); \
				bzero(Hist, sizeof(Hist)); \
 \
				int x3 = CLIP((x1 - n), 0, w - 1); \
				int y3 = CLIP((y1 - n), 0, h - 1); \
				int x4 = CLIP((x1 + n + 1), 0, w - 1); \
				int y4 = CLIP((y1 + n + 1), 0, h - 1); \
 \
				for(int y2 = y3; y2 < y4; y2++) \
				{ \
					src = src_rows[y2]; \
					for(int x2 = x3; x2 < x4; x2++) \
					{ \
						int c; \
						if((c = ++Hist[0][src[x2 * components + 0]]) > Cnt[0]) \
						{ \
							Val[0] = src[x2 * components + 0]; \
							Cnt[0] = c; \
						} \
 \
						if((c = ++Hist[1][src[x2 * components + 1]]) > Cnt[1]) \
						{ \
							Val[1] = src[x2 * components + 1]; \
							Cnt[1] = c; \
						} \
 \
						if((c = ++Hist[2][src[x2 * components + 2]]) > Cnt[2]) \
						{ \
							Val[2] = src[x2 * components + 2]; \
							Cnt[2] = c; \
						} \
 \
						if(components == 4) \
						{ \
							if((c = ++Hist[3][src[x2 * components + 3]]) > Cnt[3]) \
							{ \
								Val[3] = src[x2 * components + 3]; \
								Cnt[3] = c; \
							} \
						} \
					} \
				} \
 \
				dest[x1 * components + 0] = Val[0]; \
				dest[x1 * components + 1] = Val[1]; \
				dest[x1 * components + 2] = Val[2]; \
				if(components == 4) dest[x1 * components + 3] = Val[3]; \
			} \
		} \
		else \
		{ \
			for(int x1 = 0; x1 < w; x1++) \
			{ \
				Cnt2 = 0; \
				bzero(Val, sizeof(Val)); \
				bzero(Hist2, sizeof(Hist2)); \
 \
				int x3 = CLIP((x1 - n), 0, w - 1); \
	    		int y3 = CLIP((y1 - n), 0, h - 1); \
	    		int x4 = CLIP((x1 + n + 1), 0, w - 1); \
	    		int y4 = CLIP((y1 + n + 1), 0, h - 1); \
 \
				for(int y2 = y3; y2 < y4; y2++) \
				{ \
					src = src_rows[y2]; \
					for(int x2 = x3; x2 < x4; x2++) \
					{ \
						int c; \
						if((c = ++Hist2[INTENSITY(src + x2 * components)]) > Cnt2) \
						{ \
							Val[0] = src[x2 * components + 0]; \
							Val[1] = src[x2 * components + 1]; \
							Val[2] = src[x2 * components + 2]; \
							if(components == 3) Val[3] = src[x2 * components + 3]; \
							Cnt2 = c; \
						} \
					} \
				} \
 \
				dest[x1 * components + 0] = Val[0]; \
				dest[x1 * components + 1] = Val[1]; \
				dest[x1 * components + 2] = Val[2]; \
				if(components == 3) dest[x1 * components + 3] = Val[3]; \
			} \
		} \
	} \
}




void OilUnit::process_package(LoadPackage *package)
{
	OilPackage *pkg = (OilPackage*)package;
	int w = plugin->input->get_w();
	int h = plugin->input->get_h();
	int n = (int)(plugin->config.radius / 2);

	switch(plugin->input->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			OIL_MACRO(unsigned char, 0xff, 3)
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			OIL_MACRO(uint16_t, 0xffff, 3)
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			OIL_MACRO(unsigned char, 0xff, 4)
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			OIL_MACRO(uint16_t, 0xffff, 4)
			break;
	}




}






OilServer::OilServer(OilEffect *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void OilServer::init_packages()
{
	int increment = plugin->input->get_h() / LoadServer::total_packages + 1;
	int y = 0;
	for(int i = 0; i < LoadServer::total_packages; i++)
	{
		OilPackage *pkg = (OilPackage*)packages[i];
		pkg->row1 = y;
		pkg->row2 = y + increment;
		y += increment;
		if(pkg->row2 > plugin->input->get_h())
		{
			y = pkg->row2 = plugin->input->get_h();
		}
	}
}


LoadClient* OilServer::new_client()
{
	return new OilUnit(plugin, this);
}

LoadPackage* OilServer::new_package()
{
	return new OilPackage;
}

