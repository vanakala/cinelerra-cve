#include "../motion/affine.h"
#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "rotateframe.h"
#include "vframe.h"


#include <string.h>



#define SQR(x) ((x) * (x))
#define MAXANGLE 360


class RotateEffect;
class RotateWindow;


class RotateConfig
{
public:
	RotateConfig();

	int equivalent(RotateConfig &that);
	void copy_from(RotateConfig &that);
	void interpolate(RotateConfig &prev, 
		RotateConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	float angle;
	float pivot_x;
	float pivot_y;
	int draw_pivot;
//	int bilinear;
};

class RotateToggle : public BC_Radial
{
public:
	RotateToggle(RotateWindow *window, 
		RotateEffect *plugin, 
		int init_value, 
		int x, 
		int y, 
		int value, 
		char *string);
	int handle_event();

	RotateEffect *plugin;
    RotateWindow *window;
    int value;
};

class RotateDrawPivot : public BC_CheckBox
{
public:
	RotateDrawPivot(RotateWindow *window, 
		RotateEffect *plugin, 
		int x, 
		int y);
	int handle_event();
	RotateEffect *plugin;
    RotateWindow *window;
    int value;
};

class RotateInterpolate : public BC_CheckBox
{
public:
	RotateInterpolate(RotateEffect *plugin, int x, int y);
	int handle_event();
	RotateEffect *plugin;
};

class RotateFine : public BC_FPot
{
public:
	RotateFine(RotateWindow *window, 
		RotateEffect *plugin, 
		int x, 
		int y);
	int handle_event();

	RotateEffect *plugin;
    RotateWindow *window;
};

class RotateX : public BC_FPot
{
public:
	RotateX(RotateWindow *window, 
		RotateEffect *plugin, 
		int x, 
		int y);
	int handle_event();
	RotateEffect *plugin;
    RotateWindow *window;
};

class RotateY : public BC_FPot
{
public:
	RotateY(RotateWindow *window, 
		RotateEffect *plugin, 
		int x, 
		int y);
	int handle_event();
	RotateEffect *plugin;
    RotateWindow *window;
};


class RotateText : public BC_TextBox
{
public:
	RotateText(RotateWindow *window, 
		RotateEffect *plugin, 
		int x, 
		int y);
	int handle_event();

	RotateEffect *plugin;
    RotateWindow *window;
};

class RotateWindow : public BC_Window
{
public:
	RotateWindow(RotateEffect *plugin, int x, int y);

	int create_objects();
	int close_event();
	int update();
	int update_fine();
	int update_text();
	int update_toggles();

	RotateEffect *plugin;
	RotateToggle *toggle0;
	RotateToggle *toggle90;
	RotateToggle *toggle180;
	RotateToggle *toggle270;
	RotateDrawPivot *draw_pivot;
	RotateFine *fine;
	RotateText *text;
	RotateX *x;
	RotateY *y;
//	RotateInterpolate *bilinear;
};


PLUGIN_THREAD_HEADER(RotateEffect, RotateThread, RotateWindow)


class RotateEffect : public PluginVClient
{
public:
	RotateEffect(PluginServer *server);
	~RotateEffect();
	
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

	RotateConfig config;
	AffineEngine *engine;
	RotateThread *thread;
	Defaults *defaults;
	int need_reconfigure;
};







REGISTER_PLUGIN(RotateEffect)


















RotateConfig::RotateConfig()
{
	angle = 0;
	pivot_x = 50;
	pivot_y = 50;
	draw_pivot = 0;
}

int RotateConfig::equivalent(RotateConfig &that)
{
	return EQUIV(angle, that.angle) &&
		EQUIV(pivot_x, that.pivot_y) &&
		EQUIV(pivot_y, that.pivot_y) &&
		draw_pivot == that.draw_pivot;
}

void RotateConfig::copy_from(RotateConfig &that)
{
	angle = that.angle;
	pivot_x = that.pivot_x;
	pivot_y = that.pivot_y;
	draw_pivot = that.draw_pivot;
//	bilinear = that.bilinear;
}

void RotateConfig::interpolate(RotateConfig &prev, 
		RotateConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->angle = prev.angle * prev_scale + next.angle * next_scale;
	this->pivot_x = prev.pivot_x * prev_scale + next.pivot_x * next_scale;
	this->pivot_y = prev.pivot_y * prev_scale + next.pivot_y * next_scale;
	draw_pivot = prev.draw_pivot;
//	bilinear = prev.bilinear;
}











RotateToggle::RotateToggle(RotateWindow *window, 
	RotateEffect *plugin, 
	int init_value, 
	int x, 
	int y, 
	int value, 
	char *string)
 : BC_Radial(x, y, init_value, string)
{
	this->value = value;
	this->plugin = plugin;
    this->window = window;
}

int RotateToggle::handle_event()
{
	plugin->config.angle = (float)value;
    window->update();
	plugin->send_configure_change();
	return 1;
}







RotateDrawPivot::RotateDrawPivot(RotateWindow *window, 
	RotateEffect *plugin, 
	int x, 
	int y)
 : BC_CheckBox(x, y, plugin->config.draw_pivot, _("Draw pivot"))
{
	this->plugin = plugin;
    this->window = window;
}

int RotateDrawPivot::handle_event()
{
	plugin->config.draw_pivot = get_value();
	plugin->send_configure_change();
	return 1;
}





// RotateInterpolate::RotateInterpolate(RotateEffect *plugin, int x, int y)
//  : BC_CheckBox(x, y, plugin->config.bilinear, _("Interpolate"))
// {
// 	this->plugin = plugin;
// }
// int RotateInterpolate::handle_event()
// {
// 	plugin->config.bilinear = get_value();
// 	plugin->send_configure_change();
// 	return 1;
// }
// 



RotateFine::RotateFine(RotateWindow *window, RotateEffect *plugin, int x, int y)
 : BC_FPot(x, 
 	y, 
	(float)plugin->config.angle, 
	(float)-360, 
	(float)360)
{
	this->window = window;
	this->plugin = plugin;
	set_precision(0.01);
	set_use_caption(0);
}

int RotateFine::handle_event()
{
	plugin->config.angle = get_value();
	window->update_toggles();
	window->update_text();
	plugin->send_configure_change();
	return 1;
}



RotateText::RotateText(RotateWindow *window, 
	RotateEffect *plugin, 
	int x, 
	int y)
 : BC_TextBox(x, 
 	y, 
	100,
	1,
	(float)plugin->config.angle)
{
	this->window = window;
	this->plugin = plugin;
	set_precision(4);
}

int RotateText::handle_event()
{
	plugin->config.angle = atof(get_text());
	window->update_toggles();
	window->update_fine();
	plugin->send_configure_change();
	return 1;
}



RotateX::RotateX(RotateWindow *window, RotateEffect *plugin, int x, int y)
 : BC_FPot(x, 
 	y, 
	(float)plugin->config.pivot_x, 
	(float)0, 
	(float)100)
{
	this->window = window;
	this->plugin = plugin;
	set_precision(0.01);
	set_use_caption(1);
}

int RotateX::handle_event()
{
	plugin->config.pivot_x = get_value();
	plugin->send_configure_change();
	return 1;
}

RotateY::RotateY(RotateWindow *window, RotateEffect *plugin, int x, int y)
 : BC_FPot(x, 
 	y, 
	(float)plugin->config.pivot_y, 
	(float)0, 
	(float)100)
{
	this->window = window;
	this->plugin = plugin;
	set_precision(0.01);
	set_use_caption(1);
}

int RotateY::handle_event()
{
	plugin->config.pivot_y = get_value();
	plugin->send_configure_change();
	return 1;
}








RotateWindow::RotateWindow(RotateEffect *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
	x,
	y,
	250, 
	230, 
	250, 
	230, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

#define RADIUS 30

int RotateWindow::create_objects()
{
	int x = 10, y = 10;
	BC_Title *title;



	add_tool(new BC_Title(x, y, _("Rotate")));
	x += 50;
	y += 20;
	add_tool(toggle0 = new RotateToggle(this, 
		plugin, 
		plugin->config.angle == 0, 
		x, 
		y, 
		0, 
		"0"));
    x += RADIUS;
    y += RADIUS;
	add_tool(toggle90 = new RotateToggle(this, 
		plugin, 
		plugin->config.angle == 90, 
		x, 
		y, 
		90, 
		"90"));
    x -= RADIUS;
    y += RADIUS;
	add_tool(toggle180 = new RotateToggle(this, 
		plugin, 
		plugin->config.angle == 180, 
		x, 
		y, 
		180, 
		"180"));
    x -= RADIUS;
    y -= RADIUS;
	add_tool(toggle270 = new RotateToggle(this, 
		plugin, 
		plugin->config.angle == 270, 
		x, 
		y, 
		270, 
		"270"));
//	add_subwindow(bilinear = new RotateInterpolate(plugin, 10, y + 60));
	x += 120;
	y -= 50;
	add_tool(fine = new RotateFine(this, plugin, x, y));
	y += fine->get_h() + 10;
	add_tool(text = new RotateText(this, plugin, x, y));
	y += 30;
	add_tool(new BC_Title(x, y, _("Degrees")));
	




	y += text->get_h() + 10;
	add_subwindow(title = new BC_Title(x, y, _("Pivot (x,y):")));
	y += title->get_h() + 10;
	add_subwindow(this->x = new RotateX(this, plugin, x, y));
	x += this->x->get_w() + 10;
	add_subwindow(this->y = new RotateY(this, plugin, x, y));

	x = 10;
	y += this->y->get_h() + 10;
	add_subwindow(draw_pivot = new RotateDrawPivot(this, plugin, x, y));

	show_window();
	flush();



	return 0;
}

WINDOW_CLOSE_EVENT(RotateWindow)

int RotateWindow::update()
{
	update_fine();
	update_toggles();
	update_text();
//	bilinear->update(plugin->config.bilinear);
	return 0;
}

int RotateWindow::update_fine()
{
	fine->update(plugin->config.angle);
	x->update(plugin->config.pivot_x);
	y->update(plugin->config.pivot_y);
	return 0;
}

int RotateWindow::update_text()
{
	text->update(plugin->config.angle);
	return 0;
}

int RotateWindow::update_toggles()
{
	toggle0->update(EQUIV(plugin->config.angle, 0));
	toggle90->update(EQUIV(plugin->config.angle, 90));
	toggle180->update(EQUIV(plugin->config.angle, 180));
	toggle270->update(EQUIV(plugin->config.angle, 270));
	draw_pivot->update(plugin->config.draw_pivot);
	return 0;
}















PLUGIN_THREAD_OBJECT(RotateEffect, RotateThread, RotateWindow)


















RotateEffect::RotateEffect(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	need_reconfigure = 1;
	PLUGIN_CONSTRUCTOR_MACRO
}

RotateEffect::~RotateEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(engine) delete engine;
}



char* RotateEffect::plugin_title() { return N_("Rotate"); }
int RotateEffect::is_realtime() { return 1; }

NEW_PICON_MACRO(RotateEffect)

SET_STRING_MACRO(RotateEffect)

SHOW_GUI_MACRO(RotateEffect, RotateThread)

RAISE_WINDOW_MACRO(RotateEffect)


void RotateEffect::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->update();
		thread->window->unlock_window();
	}
}

LOAD_CONFIGURATION_MACRO(RotateEffect, RotateConfig)



int RotateEffect::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%srotate.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.angle = defaults->get("ANGLE", (float)config.angle);
	config.pivot_x = defaults->get("PIVOT_X", (float)config.pivot_x);
	config.pivot_y = defaults->get("PIVOT_Y", (float)config.pivot_y);
	config.draw_pivot = defaults->get("DRAW_PIVOT", (int)config.draw_pivot);
//	config.bilinear = defaults->get("INTERPOLATE", (int)config.bilinear);
	return 0;
}

int RotateEffect::save_defaults()
{
	defaults->update("ANGLE", (float)config.angle);
	defaults->update("PIVOT_X", (float)config.pivot_x);
	defaults->update("PIVOT_Y", (float)config.pivot_y);
	defaults->update("DRAW_PIVOT", (int)config.draw_pivot);
//	defaults->update("INTERPOLATE", (int)config.bilinear);
	defaults->save();
	return 0;
}

void RotateEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("ROTATE");
	output.tag.set_property("ANGLE", (float)config.angle);
	output.tag.set_property("PIVOT_X", (float)config.pivot_x);
	output.tag.set_property("PIVOT_Y", (float)config.pivot_y);
	output.tag.set_property("DRAW_PIVOT", (int)config.draw_pivot);
//	output.tag.set_property("INTERPOLATE", (int)config.bilinear);
	output.append_tag();
	output.terminate_string();
// data is now in *text
}

void RotateEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("ROTATE"))
			{
				config.angle = input.tag.get_property("ANGLE", (float)config.angle);
				config.pivot_x = input.tag.get_property("PIVOT_X", (float)config.pivot_x);
				config.pivot_y = input.tag.get_property("PIVOT_Y", (float)config.pivot_y);
				config.draw_pivot = input.tag.get_property("DRAW_PIVOT", (int)config.draw_pivot);
//				config.bilinear = input.tag.get_property("INTERPOLATE", (int)config.bilinear);
			}
		}
	}
}

int RotateEffect::process_realtime(VFrame *input, VFrame *output)
{
	load_configuration();
	int w = output->get_w();
	int h = output->get_h();

//printf("RotateEffect::process_realtime 1 %d %f\n", config.bilinear, config.angle);

	VFrame *temp_frame = PluginVClient::new_temp(input->get_w(),
		input->get_h(),
		input->get_color_model());

	if(!engine) engine = new AffineEngine(PluginClient::smp + 1, 
		PluginClient::smp + 1);

	temp_frame->copy_from(input);

// engine->set_viewport(50, 
// 50, 
// 100, 
// 100);
// engine->set_pivot(100, 100);

	engine->set_pivot((int)(config.pivot_x * input->get_w() / 100), 
		(int)(config.pivot_y * input->get_h() / 100));
	output->clear_frame();
	engine->rotate(output, 
		temp_frame, 
		config.angle);


// Draw center
#define CENTER_H 20
#define CENTER_W 20
#define DRAW_CENTER(components, type, max) \
{ \
	type **rows = (type**)output->get_rows(); \
	int center_x = (int)(config.pivot_x * w / 100); \
	int center_y = (int)(config.pivot_y * h / 100); \
	if(center_x >= 0 && center_x < w || \
		center_y >= 0 && center_y < h) \
	{ \
		type *hrow = rows[center_y] + components * (center_x - CENTER_W / 2); \
		for(int i = center_x - CENTER_W / 2; i <= center_x + CENTER_W / 2; i++) \
		{ \
			if(i >= 0 && i < w) \
			{ \
				hrow[0] = max - hrow[0]; \
				hrow[1] = max - hrow[1]; \
				hrow[2] = max - hrow[2]; \
				hrow += components; \
			} \
		} \
 \
		for(int i = center_y - CENTER_W / 2; i <= center_y + CENTER_W / 2; i++) \
		{ \
			if(i >= 0 && i < h) \
			{ \
				type *vrow = rows[i] + center_x * components; \
				vrow[0] = max - vrow[0]; \
				vrow[1] = max - vrow[1]; \
				vrow[2] = max - vrow[2]; \
			} \
		} \
	} \
}

	if(config.draw_pivot)
	{
		switch(output->get_color_model())
		{
			case BC_RGB_FLOAT:
				DRAW_CENTER(3, float, 1.0)
				break;
			case BC_RGBA_FLOAT:
				DRAW_CENTER(4, float, 1.0)
				break;
			case BC_RGB888:
				DRAW_CENTER(3, unsigned char, 0xff)
				break;
			case BC_RGBA8888:
				DRAW_CENTER(4, unsigned char, 0xff)
				break;
			case BC_YUV888:
				DRAW_CENTER(3, unsigned char, 0xff)
				break;
			case BC_YUVA8888:
				DRAW_CENTER(4, unsigned char, 0xff)
				break;
		}
	}

// Conserve memory by deleting large frames
	if(input->get_w() > PLUGIN_MAX_W &&
		input->get_h() > PLUGIN_MAX_H)
	{
		delete engine;
		engine = 0;
	}
	return 0;
}



