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

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)



#include <stdint.h>
#include <string.h>

class WhirlEffect;
class WhirlWindow;
class WhirlEngine;

#define MAXRADIUS 100
#define MAXPINCH 100






class WhirlConfig
{
public:
	WhirlConfig();
	
	void copy_from(WhirlConfig &src);
	int equivalent(WhirlConfig &src);
	void interpolate(WhirlConfig &prev, 
		WhirlConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);
	
	float angle;
	float pinch;
	float radius;
};


class WhirlAngle : public BC_FSlider
{
public:
	WhirlAngle(WhirlEffect *plugin, int x, int y);
	int handle_event();
	WhirlEffect *plugin;
};



class WhirlPinch : public BC_FSlider
{
public:
	WhirlPinch(WhirlEffect *plugin, int x, int y);
	int handle_event();
	WhirlEffect *plugin;
};



class WhirlRadius : public BC_FSlider
{
public:
	WhirlRadius(WhirlEffect *plugin, int x, int y);
	int handle_event();
	WhirlEffect *plugin;
};

class WhirlWindow : public BC_Window
{
public:
	WhirlWindow(WhirlEffect *plugin, int x, int y);
	void create_objects();
	int close_event();
	WhirlEffect *plugin;
	WhirlRadius *radius;
	WhirlPinch *pinch;
	WhirlAngle *angle;
};


PLUGIN_THREAD_HEADER(WhirlEffect, WhirlThread, WhirlWindow)


class WhirlPackage : public LoadPackage
{
public:
	WhirlPackage();
	int row1, row2;
};

class WhirlUnit : public LoadClient
{
public:
	WhirlUnit(WhirlEffect *plugin, WhirlEngine *server);
	void process_package(LoadPackage *package);
	WhirlEngine *server;
	WhirlEffect *plugin;
	
};


class WhirlEngine : public LoadServer
{
public:
	WhirlEngine(WhirlEffect *plugin, int cpus);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	WhirlEffect *plugin;
};



class WhirlEffect : public PluginVClient
{
public:
	WhirlEffect(PluginServer *server);
	~WhirlEffect();

	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int show_gui();
	void raise_window();
	void update_gui();
	int set_string();
	int load_configuration();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	WhirlEngine *engine;
	VFrame *temp_frame;
	VFrame *input, *output;
	WhirlConfig config;
	Defaults *defaults;
	WhirlThread *thread;
	int need_reconfigure;
};




PLUGIN_THREAD_OBJECT(WhirlEffect, WhirlThread, WhirlWindow)


REGISTER_PLUGIN(WhirlEffect)
















WhirlConfig::WhirlConfig()
{
	angle = 0.0;
	pinch = 0.0;
	radius = 0.0;
}

void WhirlConfig::copy_from(WhirlConfig &src)
{
	this->angle = src.angle;
	this->pinch = src.pinch;
	this->radius = src.radius;
}

int WhirlConfig::equivalent(WhirlConfig &src)
{
	return EQUIV(this->angle, src.angle) &&
		EQUIV(this->pinch, src.pinch) &&
		EQUIV(this->radius, src.radius);
}

void WhirlConfig::interpolate(WhirlConfig &prev, 
	WhirlConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->angle = prev.angle * prev_scale + next.angle * next_scale;
	this->pinch = prev.pinch * prev_scale + next.pinch * next_scale;
	this->radius = prev.radius * prev_scale + next.radius * next_scale;
}










WhirlWindow::WhirlWindow(WhirlEffect *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	220, 
	200, 
	220, 
	200, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}



void WhirlWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, _("Radius")));
	y += 20;
	add_subwindow(radius = new WhirlRadius(plugin, x, y));
	y += 40;
	add_subwindow(new BC_Title(x, y, _("Pinch")));
	y += 20;
	add_subwindow(pinch = new WhirlPinch(plugin, x, y));
	y += 40;
	add_subwindow(new BC_Title(x, y, _("Angle")));
	y += 20;
	add_subwindow(angle = new WhirlAngle(plugin, x, y));

	show_window();
	flush();
}

int WhirlWindow::close_event()
{
	set_done(1);
	return 1;
}













WhirlAngle::WhirlAngle(WhirlEffect *plugin, int x, int y)
 : BC_FSlider(x, 
	   	y, 
		0,
		200,
		200, 
		(float)0, 
		(float)360,
		plugin->config.angle)
{
	this->plugin = plugin;
}
int WhirlAngle::handle_event()
{
	plugin->config.angle = get_value();
	plugin->send_configure_change();
	return 1;
}




WhirlPinch::WhirlPinch(WhirlEffect *plugin, int x, int y)
 : BC_FSlider(x, 
	   	y, 
		0,
		200,
		200, 
		(float)0, 
		(float)MAXPINCH, 
		plugin->config.pinch)
{
	this->plugin = plugin;
}
int WhirlPinch::handle_event()
{
	plugin->config.pinch = get_value();
	plugin->send_configure_change();
	return 1;
}




WhirlRadius::WhirlRadius(WhirlEffect *plugin, int x, int y)
 : BC_FSlider(x, 
	   	y, 
		0,
		200,
		200, 
		(float)0, 
		(float)MAXRADIUS, 
		plugin->config.radius)
{
	this->plugin = plugin;
}
int WhirlRadius::handle_event()
{
	plugin->config.radius = get_value();
	plugin->send_configure_change();
	return 1;
}











WhirlEffect::WhirlEffect(PluginServer *server)
 : PluginVClient(server)
{
	need_reconfigure = 1;
	engine = 0;
	temp_frame = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

WhirlEffect::~WhirlEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(engine) delete engine;
	if(temp_frame) delete temp_frame;
}





int WhirlEffect::is_realtime()
{
	return 1;
}

char* WhirlEffect::plugin_title()
{
	return _("Whirl");
}

NEW_PICON_MACRO(WhirlEffect)

SHOW_GUI_MACRO(WhirlEffect, WhirlThread)

RAISE_WINDOW_MACRO(WhirlEffect)

SET_STRING_MACRO(WhirlEffect)

void WhirlEffect::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->angle->update(config.angle);
		thread->window->pinch->update(config.pinch);
		thread->window->radius->update(config.radius);
		thread->window->unlock_window();
	}
}

LOAD_CONFIGURATION_MACRO(WhirlEffect, WhirlConfig)




int WhirlEffect::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%swhirl.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.angle = defaults->get("ANGLE", config.angle);
	config.pinch = defaults->get("PINCH", config.pinch);
	config.radius = defaults->get("RADIUS", config.radius);
}

int WhirlEffect::save_defaults()
{
	defaults->update("ANGLE", config.angle);
	defaults->update("PINCH", config.pinch);
	defaults->update("RADIUS", config.radius);
	defaults->save();
}

void WhirlEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);

	output.tag.set_title("WHIRL");
	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("PINCH", config.pinch);
	output.tag.set_property("RADIUS", config.radius);
	output.append_tag();
	output.terminate_string();
// data is now in *text
}

void WhirlEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("WHIRL"))
			{
				config.angle = input.tag.get_property("ANGLE", config.angle);
				config.pinch = input.tag.get_property("PINCH", config.pinch);
				config.radius = input.tag.get_property("RADIUS", config.radius);
			}
		}
	}
}

int WhirlEffect::process_realtime(VFrame *input, VFrame *output)
{
	need_reconfigure |= load_configuration();
	this->input = input;
	this->output = output;

	if(EQUIV(config.angle, 0) || EQUIV(config.radius, 0))
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
//printf("WhirlEffect::process_realtime 1\n");
		}

		if(!engine) engine = new WhirlEngine(this, PluginClient::smp + 1);
		
		engine->process_packages();
	}
}








WhirlPackage::WhirlPackage()
 : LoadPackage()
{
}



WhirlUnit::WhirlUnit(WhirlEffect *plugin, WhirlEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}



static int calc_undistorted_coords(double cen_x,
			double cen_y,
			double scale_x,
			double scale_y,
			double radius,
			double radius2,
			double radius3,
			double pinch,
			double wx,
			double wy,
			double &whirl,
			double &x,
			double &y)
{
	double dx, dy;
	double d, factor;
	double dist;
	double ang, sina, cosa;
	int inside;

/* Distances to center, scaled */

	dx = (wx - cen_x) * scale_x;
	dy = (wy - cen_y) * scale_y;

/* Distance^2 to center of *circle* (scaled ellipse) */

	d = dx * dx + dy * dy;

/*  If we are inside circle, then distort.
 *  Else, just return the same position
 */

	inside = (d < radius2);

	if(inside)
    {
    	dist = sqrt(d / radius3) / radius;

/* Pinch */

    	factor = pow(sin(M_PI / 2 * dist), -pinch);

    	dx *= factor;
    	dy *= factor;

/* Whirl */

    	factor = 1.0 - dist;

    	ang = whirl * factor * factor;

    	sina = sin(ang);
    	cosa = cos(ang);

    	x = (cosa * dx - sina * dy) / scale_x + cen_x;
    	y = (sina * dx + cosa * dy) / scale_y + cen_y;
    }

	return inside;
}



#define GET_PIXEL(components, x, y, input_rows) \
	input_rows[CLIP(y, 0, (h - 1))] + components * CLIP(x, 0, (w - 1))






static float bilinear(double x, double y, double *values)
{
	double m0, m1;
	x = fmod(x, 1.0);
	y = fmod(y, 1.0);

	if(x < 0.0) x += 1.0;
	if(y < 0.0) y += 1.0;

	m0 = (double)values[0] + x * ((double)values[1] - values[0]);
	m1 = (double)values[2] + x * ((double)values[3] - values[2]);
	return m0 + y * (m1 - m0);
}





#define WHIRL_MACRO(type, max, components) \
{ \
	type **input_rows = (type**)plugin->input->get_rows(); \
	double values[components]; \
	for(int row = pkg->row1; row <= (pkg->row2 + pkg->row1) / 2; row++) \
	{ \
		type *top_row = (type*)plugin->output->get_rows()[row]; \
		type *bot_row = (type*)plugin->output->get_rows()[h - row - 1]; \
		type *top_p = top_row; \
		type *bot_p = bot_row + components * w - components; \
		 \
		for(int col = 0; col < w; col++) \
		{ \
			if(calc_undistorted_coords(cen_x, \
				cen_y, \
				scale_x, \
				scale_y, \
				radius, \
				radius2, \
				radius3, \
				pinch, \
				col, \
				row, \
				whirl, \
				cx, \
				cy)) \
			{ \
/* Inside distortion area */ \
/* Do top */ \
				if(cx >= 0.0) \
					ix = (int)cx; \
				else \
					ix = -((int)-cx + 1); \
 \
				if(cy >= 0.0) \
					iy = (int)cy; \
				else \
					iy = -((int)-cy + 1); \
 \
				type *pixel1 = GET_PIXEL(components, ix,     iy,     input_rows); \
				type *pixel2 = GET_PIXEL(components, ix + 1, iy,     input_rows); \
				type *pixel3 = GET_PIXEL(components, ix,     iy + 1, input_rows); \
				type *pixel4 = GET_PIXEL(components, ix + 1, iy + 1, input_rows); \
 \
				values[0] = pixel1[0]; \
				values[1] = pixel2[0]; \
				values[2] = pixel3[0]; \
				values[3] = pixel4[0]; \
				top_p[0] = (type)bilinear(cx, cy, values); \
 \
				values[0] = pixel1[1]; \
				values[1] = pixel2[1]; \
				values[2] = pixel3[1]; \
				values[3] = pixel4[1]; \
				top_p[1] = (type)bilinear(cx, cy, values); \
 \
				values[0] = pixel1[2]; \
				values[1] = pixel2[2]; \
				values[2] = pixel3[2]; \
				values[3] = pixel4[2]; \
				top_p[2] = (type)bilinear(cx, cy, values); \
 \
				if(components == 4) \
				{ \
					values[0] = pixel1[3]; \
					values[1] = pixel2[3]; \
					values[2] = pixel3[3]; \
					values[3] = pixel4[3]; \
					top_p[3] = (type)bilinear(cx, cy, values); \
				} \
 \
				top_p += components; \
 \
/* Do bottom */ \
	    		cx = cen_x + (cen_x - cx); \
	    		cy = cen_y + (cen_y - cy); \
 \
	    		if(cx >= 0.0) \
					ix = (int)cx; \
	    		else \
					ix = -((int)-cx + 1); \
 \
	    		if(cy >= 0.0) \
					iy = (int)cy; \
	    		else \
					iy = -((int)-cy + 1); \
 \
				pixel1 = GET_PIXEL(components, ix,     iy,     input_rows); \
				pixel2 = GET_PIXEL(components, ix + 1, iy,     input_rows); \
				pixel3 = GET_PIXEL(components, ix,     iy + 1, input_rows); \
				pixel4 = GET_PIXEL(components, ix + 1, iy + 1, input_rows); \
 \
 \
 \
				values[0] = pixel1[0]; \
				values[1] = pixel2[0]; \
				values[2] = pixel3[0]; \
				values[3] = pixel4[0]; \
				bot_p[0] = (type)bilinear(cx, cy, values); \
 \
				values[0] = pixel1[1]; \
				values[1] = pixel2[1]; \
				values[2] = pixel3[1]; \
				values[3] = pixel4[1]; \
				bot_p[1] = (type)bilinear(cx, cy, values); \
 \
				values[0] = pixel1[2]; \
				values[1] = pixel2[2]; \
				values[2] = pixel3[2]; \
				values[3] = pixel4[2]; \
				bot_p[2] = (type)bilinear(cx, cy, values); \
 \
				if(components == 4) \
				{ \
					values[0] = pixel1[3]; \
					values[1] = pixel2[3]; \
					values[2] = pixel3[3]; \
					values[3] = pixel4[3]; \
					bot_p[3] = (type)bilinear(cx, cy, values); \
				} \
 \
				bot_p -= components; \
 \
 \
			} \
			else \
			{ \
/* Outside distortion area */ \
/* Do top */ \
				top_p[0] = input_rows[row][components * col + 0]; \
				top_p[1] = input_rows[row][components * col + 1]; \
				top_p[2] = input_rows[row][components * col + 2]; \
				if(components == 4) top_p[3] = input_rows[row][components * col + 3]; \
 \
 \
				top_p += components; \
 \
/* Do bottom */ \
				int bot_offset = w * components - col * components - components; \
				int bot_row = h - 1 - row; \
				bot_p[0] = input_rows[bot_row][bot_offset + 0]; \
				bot_p[1] = input_rows[bot_row][bot_offset + 1]; \
				bot_p[2] = input_rows[bot_row][bot_offset + 2]; \
				if(components == 4) bot_p[3] = input_rows[bot_row][bot_offset + 3]; \
				bot_p -= components; \
			} \
		} \
	} \
}

void WhirlUnit::process_package(LoadPackage *package)
{
	WhirlPackage *pkg = (WhirlPackage*)package;
	int w = plugin->input->get_w();
	int h = plugin->input->get_h();
	double whirl = plugin->config.angle * M_PI / 180;
	double pinch = plugin->config.pinch / MAXPINCH;
  	double cx, cy;
    int ix, iy;
	double cen_x = (double)(w - 1) / 2.0;
	double cen_y = (double)(h - 1) / 2.0;
	double radius = MAX(w, h);
	double radius3 = plugin->config.radius / MAXRADIUS;
  	double radius2 = radius * radius * radius3;
	double scale_x;
	double scale_y;


//printf("WhirlUnit::process_package 1 %f %f %f\n", 
//	plugin->config.angle, plugin->config.pinch, plugin->config.radius);
	if(w < h)
	{
    	scale_x = (double)h / w;
    	scale_y = 1.0;
	}
	else
	if(w > h)
	{
    	scale_x = 1.0;
    	scale_y = (double)w / h;
	}
	else
	{
    	scale_x = 1.0;
    	scale_y = 1.0;
	}



	switch(plugin->input->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			WHIRL_MACRO(unsigned char, 0xff, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			WHIRL_MACRO(unsigned char, 0xff, 4);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			WHIRL_MACRO(uint16_t, 0xffff, 3);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			WHIRL_MACRO(uint16_t, 0xffff, 4);
			break;
		
	}
}







WhirlEngine::WhirlEngine(WhirlEffect *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}
void WhirlEngine::init_packages()
{
	int increment = plugin->input->get_h() / LoadServer::total_packages;
	int y = 0;
	for(int i = 0; i < LoadServer::total_packages; i++)
	{
		WhirlPackage *pkg = (WhirlPackage*)packages[i];
		pkg->row1 = y;
		pkg->row2 = y + increment;
		if(i == LoadServer::total_packages - 1) pkg->row2 = plugin->input->get_h();
		y += increment;
	}
	
}

LoadClient* WhirlEngine::new_client()
{
	return new WhirlUnit(plugin, this);
}

LoadPackage* WhirlEngine::new_package()
{
	return new WhirlPackage;
}

