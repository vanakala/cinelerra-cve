#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "gradient.h"
#include "keyframe.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"





REGISTER_PLUGIN(GradientMain)






GradientConfig::GradientConfig()
{
	angle = 0;
	in_radius = 0;
	out_radius = 100;
	in_r = 0xff;
	in_g = 0xff;
	in_b = 0xff;
	in_a = 0xff;
	out_r = 0x0;
	out_g = 0x0;
	out_b = 0x0;
	out_a = 0x0;
}

int GradientConfig::equivalent(GradientConfig &that)
{
	return (EQUIV(angle, that.angle) &&
		EQUIV(in_radius, that.in_radius) &&
		EQUIV(out_radius, that.out_radius) &&
		in_r == that.in_r &&
		in_g == that.in_g &&
		in_b == that.in_b &&
		in_a == that.in_a &&
		out_r == that.out_r &&
		out_g == that.out_g &&
		out_b == that.out_b &&
		out_a == that.out_a);
}

void GradientConfig::copy_from(GradientConfig &that)
{
	angle = that.angle;
	in_radius = that.in_radius;
	out_radius = that.out_radius;
	in_r = that.in_r;
	in_g = that.in_g;
	in_b = that.in_b;
	in_a = that.in_a;
	out_r = that.out_r;
	out_g = that.out_g;
	out_b = that.out_b;
	out_a = that.out_a;
}

void GradientConfig::interpolate(GradientConfig &prev, 
	GradientConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);


	this->angle = (int)(prev.angle * prev_scale + next.angle * next_scale);
	this->in_radius = (int)(prev.in_radius * prev_scale + next.in_radius * next_scale);
	this->out_radius = (int)(prev.out_radius * prev_scale + next.out_radius * next_scale);
	in_r = (int)(prev.in_r * prev_scale + next.in_r * next_scale);
	in_g = (int)(prev.in_g * prev_scale + next.in_g * next_scale);
	in_b = (int)(prev.in_b * prev_scale + next.in_b * next_scale);
	in_a = (int)(prev.in_a * prev_scale + next.in_a * next_scale);
	out_r = (int)(prev.out_r * prev_scale + next.out_r * next_scale);
	out_g = (int)(prev.out_g * prev_scale + next.out_g * next_scale);
	out_b = (int)(prev.out_b * prev_scale + next.out_b * next_scale);
	out_a = (int)(prev.out_a * prev_scale + next.out_a * next_scale);
}

int GradientConfig::get_in_color()
{
	int result = (in_r << 16) | (in_g << 8) | (in_b);
	return result;
}

int GradientConfig::get_out_color()
{
	int result = (out_r << 16) | (out_g << 8) | (out_b);
	return result;
}









PLUGIN_THREAD_OBJECT(GradientMain, GradientThread, GradientWindow)


#define COLOR_W 100
#define COLOR_H 30

GradientWindow::GradientWindow(GradientMain *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x,
	y,
	350, 
	210, 
	350, 
	210, 
	0, 
	1)
{
	this->plugin = plugin; 
}

GradientWindow::~GradientWindow()
{
	delete in_color_thread;
	delete out_color_thread;
}

int GradientWindow::create_objects()
{
	int x = 10, y = 10, x2 = 130;

	add_subwindow(new BC_Title(x, y, "Angle:"));
	add_subwindow(angle = new GradientAngle(plugin, x2, y));
	y += 40;
	add_subwindow(new BC_Title(x, y, "Inner radius:"));
	add_subwindow(in_radius = new GradientInRadius(plugin, x2, y));
	y += 30;
	add_subwindow(new BC_Title(x, y, "Outer radius:"));
	add_subwindow(out_radius = new GradientOutRadius(plugin, x2, y));
	y += 35;
	add_subwindow(in_color = new GradientInColorButton(plugin, this, x, y));
	in_color_x = x2;
	in_color_y = y;
	y += 35;
	add_subwindow(out_color = new GradientOutColorButton(plugin, this, x, y));
	out_color_x = x2;
	out_color_y = y;
	in_color_thread = new GradientInColorThread(plugin, this);
	out_color_thread = new GradientOutColorThread(plugin, this);
	update_in_color();
	update_out_color();

	show_window();
	flush();
	return 0;
}

int GradientWindow::close_event()
{
// Set result to 1 to indicate a plugin side close
	set_done(1);
	return 1;
}

void GradientWindow::update_in_color()
{
//printf("GradientWindow::update_in_color 1 %08x\n", plugin->config.get_in_color());
	set_color(plugin->config.get_in_color());
	draw_box(in_color_x, in_color_y, COLOR_W, COLOR_H);
	flash(in_color_x, in_color_y, COLOR_W, COLOR_H);
}

void GradientWindow::update_out_color()
{
//printf("GradientWindow::update_out_color 1 %08x\n", plugin->config.get_in_color());
	set_color(plugin->config.get_out_color());
	draw_box(out_color_x, out_color_y, COLOR_W, COLOR_H);
	flash(out_color_x, out_color_y, COLOR_W, COLOR_H);
}









GradientAngle::GradientAngle(GradientMain *plugin, int x, int y)
 : BC_FPot(x,
 	y,
	plugin->config.angle,
	-180,
	180)
{
	this->plugin = plugin;
}

int GradientAngle::handle_event()
{
	plugin->config.angle = get_value();
	plugin->send_configure_change();
	return 1;
}


GradientInRadius::GradientInRadius(GradientMain *plugin, int x, int y)
 : BC_FSlider(x,
 	y,
	0,
	200,
	200,
	(float)0,
	(float)100,
	(float)plugin->config.in_radius)
{
	this->plugin = plugin;
}

int GradientInRadius::handle_event()
{
	plugin->config.in_radius = get_value();
	plugin->send_configure_change();
	return 1;
}


GradientOutRadius::GradientOutRadius(GradientMain *plugin, int x, int y)
 : BC_FSlider(x,
 	y,
	0,
	200,
	200,
	(float)0,
	(float)100,
	(float)plugin->config.out_radius)
{
	this->plugin = plugin;
}

int GradientOutRadius::handle_event()
{
	plugin->config.out_radius = get_value();
	plugin->send_configure_change();
	return 1;
}

GradientInColorButton::GradientInColorButton(GradientMain *plugin, GradientWindow *window, int x, int y)
 : BC_GenericButton(x, y, "Inner color:")
{
	this->plugin = plugin;
	this->window = window;
}

int GradientInColorButton::handle_event()
{
	window->in_color_thread->start_window(
		plugin->config.get_in_color(),
		plugin->config.in_a);
	return 1;
}


GradientOutColorButton::GradientOutColorButton(GradientMain *plugin, GradientWindow *window, int x, int y)
 : BC_GenericButton(x, y, "Outer color:")
{
	this->plugin = plugin;
	this->window = window;
}

int GradientOutColorButton::handle_event()
{
	window->out_color_thread->start_window(
		plugin->config.get_out_color(),
		plugin->config.out_a);
	return 1;
}



GradientInColorThread::GradientInColorThread(GradientMain *plugin, 
	GradientWindow *window)
 : ColorThread(1, "Inner color")
{
	this->plugin = plugin;
	this->window = window;
}

int GradientInColorThread::handle_event(int output)
{
	plugin->config.in_r = (output & 0xff0000) >> 16;
	plugin->config.in_g = (output & 0xff00) >> 8;
	plugin->config.in_b = (output & 0xff);
	plugin->config.in_a = alpha;
	window->update_in_color();
	window->flush();
	plugin->send_configure_change();
// printf("GradientInColorThread::handle_event 1 %d %d %d %d %d %d %d %d\n",
// plugin->config.in_r,
// plugin->config.in_g,
// plugin->config.in_b,
// plugin->config.in_a,
// plugin->config.out_r,
// plugin->config.out_g,
// plugin->config.out_b,
// plugin->config.out_a);

	return 1;
}



GradientOutColorThread::GradientOutColorThread(GradientMain *plugin, 
	GradientWindow *window)
 : ColorThread(1, "Outer color")
{
	this->plugin = plugin;
	this->window = window;
}

int GradientOutColorThread::handle_event(int output)
{
	plugin->config.out_r = (output & 0xff0000) >> 16;
	plugin->config.out_g = (output & 0xff00) >> 8;
	plugin->config.out_b = (output & 0xff);
	plugin->config.out_a = alpha;
	window->update_out_color();
	window->flush();
	plugin->send_configure_change();
// printf("GradientOutColorThread::handle_event 1 %d %d %d %d %d %d %d %d\n",
// plugin->config.in_r,
// plugin->config.in_g,
// plugin->config.in_b,
// plugin->config.in_a,
// plugin->config.out_r,
// plugin->config.out_g,
// plugin->config.out_b,
// plugin->config.out_a);
	return 1;
}












GradientMain::GradientMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	need_reconfigure = 1;
	gradient = 0;
	engine = 0;
	overlayer = 0;
}

GradientMain::~GradientMain()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(gradient) delete gradient;
	if(engine) delete engine;
	if(overlayer) delete overlayer;
}

char* GradientMain::plugin_title() { return "Gradient"; }
int GradientMain::is_realtime() { return 1; }


NEW_PICON_MACRO(GradientMain)

SHOW_GUI_MACRO(GradientMain, GradientThread)

SET_STRING_MACRO(GradientMain)

RAISE_WINDOW_MACRO(GradientMain)

LOAD_CONFIGURATION_MACRO(GradientMain, GradientConfig)

int GradientMain::is_synthesis()
{
	return 1;
}


int GradientMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	this->input = input_ptr;
	this->output = output_ptr;
	need_reconfigure |= load_configuration();

// printf("GradientMain::process_realtime 1 %d %d %d %d %d %d %d %d\n",
// config.in_r,
// config.in_g,
// config.in_b,
// config.in_a,
// config.out_r,
// config.out_g,
// config.out_b,
// config.out_a);



// Generate new gradient
	if(need_reconfigure)
	{
		need_reconfigure = 0;

//printf("GradientMain::process_realtime 1\n");
		if(!gradient) gradient = new VFrame(0, 
			input->get_w(),
			input->get_h(),
			input->get_color_model());

		if(!engine) engine = new GradientServer(this,
			get_project_smp() + 1,
			get_project_smp() + 1);
		engine->process_packages();
	}

//printf("GradientMain::process_realtime 2\n");
// Overlay new gradient
	if(output->get_rows()[0] != input->get_rows()[0])
	{
		output->copy_from(input);
	}

//printf("GradientMain::process_realtime 2\n");
	if(!overlayer) overlayer = new OverlayFrame(get_project_smp() + 1);
	overlayer->overlay(output, 
		gradient,
		0, 
		0, 
		input->get_w(), 
		input->get_h(),
		0, 
		0, 
		input->get_w(), 
		input->get_h(), 
		1.0, 
		TRANSFER_NORMAL,
		NEAREST_NEIGHBOR);
//printf("GradientMain::process_realtime 2\n");

	return 0;
}


void GradientMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->angle->update(config.angle);
		thread->window->in_radius->update(config.in_radius);
		thread->window->out_radius->update(config.out_radius);
		thread->window->update_in_color();
		thread->window->update_out_color();
		thread->window->unlock_window();
	}
}


int GradientMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%sgradient.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

// printf("GradientMain::load_defaults %d %d %d %d\n",
// config.out_r,
// config.out_g,
// config.out_b,
// config.out_a);
	config.angle = defaults->get("ANGLE", config.angle);
	config.in_radius = defaults->get("IN_RADIUS", config.in_radius);
	config.out_radius = defaults->get("OUT_RADIUS", config.out_radius);
	config.in_r = defaults->get("IN_R", config.in_r);
	config.in_g = defaults->get("IN_G", config.in_g);
	config.in_b = defaults->get("IN_B", config.in_b);
	config.in_a = defaults->get("IN_A", config.in_a);
	config.out_r = defaults->get("OUT_R", config.out_r);
	config.out_g = defaults->get("OUT_G", config.out_g);
	config.out_b = defaults->get("OUT_B", config.out_b);
	config.out_a = defaults->get("OUT_A", config.out_a);
	return 0;
}


int GradientMain::save_defaults()
{
	defaults->update("ANGLE", config.angle);
	defaults->update("IN_RADIUS", config.in_radius);
	defaults->update("OUT_RADIUS", config.out_radius);
	defaults->update("IN_R", config.in_r);
	defaults->update("IN_G", config.in_g);
	defaults->update("IN_B", config.in_b);
	defaults->update("IN_A", config.in_a);
	defaults->update("OUT_R", config.out_r);
	defaults->update("OUT_G", config.out_g);
	defaults->update("OUT_B", config.out_b);
	defaults->update("OUT_A", config.out_a);
	defaults->save();
	return 0;
}



void GradientMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("GRADIENT");

	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("IN_RADIUS", config.in_radius);
	output.tag.set_property("OUT_RADIUS", config.out_radius);
	output.tag.set_property("IN_R", config.in_r);
	output.tag.set_property("IN_G", config.in_g);
	output.tag.set_property("IN_B", config.in_b);
	output.tag.set_property("IN_A", config.in_a);
	output.tag.set_property("OUT_R", config.out_r);
	output.tag.set_property("OUT_G", config.out_g);
	output.tag.set_property("OUT_B", config.out_b);
	output.tag.set_property("OUT_A", config.out_a);
	output.append_tag();
	output.terminate_string();
}

void GradientMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("GRADIENT"))
			{
				config.angle = input.tag.get_property("ANGLE", config.angle);
				config.in_radius = input.tag.get_property("IN_RADIUS", config.in_radius);
				config.out_radius = input.tag.get_property("OUT_RADIUS", config.out_radius);
				config.in_r = input.tag.get_property("IN_R", config.in_r);
				config.in_g = input.tag.get_property("IN_G", config.in_g);
				config.in_b = input.tag.get_property("IN_B", config.in_b);
				config.in_a = input.tag.get_property("IN_A", config.in_a);
				config.out_r = input.tag.get_property("OUT_R", config.out_r);
				config.out_g = input.tag.get_property("OUT_G", config.out_g);
				config.out_b = input.tag.get_property("OUT_B", config.out_b);
				config.out_a = input.tag.get_property("OUT_A", config.out_a);
			}
		}
	}
}






GradientPackage::GradientPackage()
 : LoadPackage()
{
}




GradientUnit::GradientUnit(GradientServer *server, GradientMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}


#define SQR(x) ((x) * (x))


#define CREATE_GRADIENT(type, components, max) \
{ \
/* Synthesize linear gradient for lookups */ \
 \
	for(int i = 0; i < gradient_size; i++) \
	{ \
		float opacity; \
		float transparency; \
		if(i < in_radius) \
			opacity = 0.0; \
		else \
		if(i < out_radius) \
			opacity = (float)(i - in_radius) / (out_radius - in_radius); \
		else \
			opacity = 1.0; \
		transparency = 1.0 - opacity; \
		r_table[i] = (int)(out1 * opacity + in1 * transparency); \
		g_table[i] = (int)(out2 * opacity + in2 * transparency); \
		b_table[i] = (int)(out3 * opacity + in3 * transparency); \
		a_table[i] = (int)(out4 * opacity + in4 * transparency); \
	} \
 \
	for(int i = pkg->y1; i < pkg->y2; i++) \
	{ \
		type *out_row = (type*)plugin->gradient->get_rows()[i]; \
 \
		for(int j = 0; j < w; j++) \
		{ \
/* Convert to polar coords */ \
			int x = j - half_w; \
			int y = -(i - half_h); \
			double magnitude = sqrt(SQR(x) + SQR(y)); \
			double angle; \
			if(x != 0) \
			{ \
				angle = atan((float)y / x); \
				if(x > 0 && y < 0) angle = M_PI / 2 - angle; \
				if(x > 0 && y > 0) angle = M_PI / 2 - angle; \
				if(x < 0 && y > 0) angle = -M_PI / 2 - angle; \
				if(x < 0 && y < 0) angle = -M_PI / 2 - angle; \
				if(y == 0 && x > 0) angle = M_PI / 2; \
				if(y == 0 && x < 0) angle = -M_PI / 2; \
			} \
			else \
			if(y > 0) \
				angle = 0; \
			else \
				angle = M_PI; \
 \
 \
 \
/* Rotate by effect angle */ \
			angle -= effect_angle; \
 \
/* Convert to cartesian coords */ \
			int input_y = (int)(gradient_size / 2 - magnitude * cos(angle)); \
 \
/* Get gradient value from these coords */ \
 \
 			if(input_y < 0) \
			{ \
				out_row[0] = out1; \
				out_row[1] = out2; \
				out_row[2] = out3; \
				if(components == 4) out_row[3] = out4; \
			} \
			else \
			if(input_y >= gradient_size) \
			{ \
				out_row[0] = in1; \
				out_row[1] = in2; \
				out_row[2] = in3; \
				if(components == 4) out_row[3] = in4; \
			} \
			else \
			{ \
				out_row[0] = r_table[input_y]; \
				out_row[1] = g_table[input_y]; \
				out_row[2] = b_table[input_y]; \
				if(components == 4) out_row[3] = a_table[input_y]; \
			} \
 \
 			out_row += components; \
		} \
	} \
}

void GradientUnit::process_package(LoadPackage *package)
{
	GradientPackage *pkg = (GradientPackage*)package;
	int in1, in2, in3, in4;
	int out1, out2, out3, out4;
	int h = plugin->input->get_h();
	int w = plugin->input->get_w();
	int half_w = w / 2;
	int half_h = h / 2;
	int gradient_size = (int)(sqrt(SQR(w) + SQR(h)) + 1);
	int in_radius = (int)(plugin->config.in_radius / 100 * gradient_size);
	int out_radius = (int)(plugin->config.out_radius / 100 * gradient_size);
	double effect_angle = plugin->config.angle / 360 * 2 * M_PI;
	int r_table[gradient_size];
	int g_table[gradient_size];
	int b_table[gradient_size];
	int a_table[gradient_size];

	if(in_radius > out_radius)
	{
		in_radius ^= out_radius ^= in_radius ^= out_radius;
	}


	switch(plugin->input->get_color_model())
	{
		case BC_RGB888:
			in1 = plugin->config.in_r;
			in2 = plugin->config.in_g;
			in3 = plugin->config.in_b;
			in4 = plugin->config.in_a;
			out1 = plugin->config.out_r;
			out2 = plugin->config.out_g;
			out3 = plugin->config.out_b;
			out4 = plugin->config.out_a;
			CREATE_GRADIENT(unsigned char, 3, 0xff)
			break;
		case BC_RGBA8888:
			in1 = plugin->config.in_r;
			in2 = plugin->config.in_g;
			in3 = plugin->config.in_b;
			in4 = plugin->config.in_a;
			out1 = plugin->config.out_r;
			out2 = plugin->config.out_g;
			out3 = plugin->config.out_b;
			out4 = plugin->config.out_a;
			CREATE_GRADIENT(unsigned char, 4, 0xff)
			break;
		case BC_RGB161616:
			in1 = (plugin->config.in_r << 8) | plugin->config.in_r;
			in2 = (plugin->config.in_g << 8) | plugin->config.in_g;
			in3 = (plugin->config.in_b << 8) | plugin->config.in_b;
			in4 = (plugin->config.in_a << 8) | plugin->config.in_a;
			out1 = (plugin->config.out_r << 8) | plugin->config.out_r;
			out2 = (plugin->config.out_g << 8) | plugin->config.out_g;
			out3 = (plugin->config.out_b << 8) | plugin->config.out_b;
			out4 = (plugin->config.out_a << 8) | plugin->config.out_a;
			CREATE_GRADIENT(uint16_t, 3, 0xffff)
			break;
		case BC_RGBA16161616:
			in1 = (plugin->config.in_r << 8) | plugin->config.in_r;
			in2 = (plugin->config.in_g << 8) | plugin->config.in_g;
			in3 = (plugin->config.in_b << 8) | plugin->config.in_b;
			in4 = (plugin->config.in_a << 8) | plugin->config.in_a;
			out1 = (plugin->config.out_r << 8) | plugin->config.out_r;
			out2 = (plugin->config.out_g << 8) | plugin->config.out_g;
			out3 = (plugin->config.out_b << 8) | plugin->config.out_b;
			out4 = (plugin->config.out_a << 8) | plugin->config.out_a;
			CREATE_GRADIENT(uint16_t, 4, 0xffff)
			break;
		case BC_YUV888:
			yuv.rgb_to_yuv_8(plugin->config.in_r,
				plugin->config.in_g,
				plugin->config.in_b,
				in1,
				in2,
				in3);
			in4 = plugin->config.in_a;
			yuv.rgb_to_yuv_8(plugin->config.out_r,
				plugin->config.out_g,
				plugin->config.out_b,
				out1,
				out2,
				out3);
			out4 = plugin->config.out_a;
			CREATE_GRADIENT(unsigned char, 3, 0xff)
			break;
		case BC_YUVA8888:
			yuv.rgb_to_yuv_8(plugin->config.in_r,
				plugin->config.in_g,
				plugin->config.in_b,
				in1,
				in2,
				in3);
			in4 = plugin->config.in_a;
			yuv.rgb_to_yuv_8(plugin->config.out_r,
				plugin->config.out_g,
				plugin->config.out_b,
				out1,
				out2,
				out3);
			out4 = plugin->config.out_a;
			CREATE_GRADIENT(unsigned char, 4, 0xff)
			break;
		case BC_YUV161616:
			yuv.rgb_to_yuv_16(
				plugin->config.in_r,
				plugin->config.in_g,
				plugin->config.in_b,
				in1,
				in2,
				in3);
			in4 = plugin->config.in_a;
			yuv.rgb_to_yuv_16(
				(plugin->config.out_r << 8) | plugin->config.out_r,
				(plugin->config.out_g << 8) | plugin->config.out_g,
				(plugin->config.out_b << 8) | plugin->config.out_b,
				out1,
				out2,
				out3);
			out4 = (plugin->config.out_a << 8) | plugin->config.out_a;
			CREATE_GRADIENT(uint16_t, 3, 0xffff)
			break;
		case BC_YUVA16161616:
			yuv.rgb_to_yuv_16(
				(plugin->config.in_r << 8) | plugin->config.in_r,
				(plugin->config.in_g << 8) | plugin->config.in_g,
				(plugin->config.in_b << 8) | plugin->config.in_b,
				in1,
				in2,
				in3);
			in4 = (plugin->config.in_a << 8) | plugin->config.in_a;
			yuv.rgb_to_yuv_16(
				(plugin->config.out_r << 8) | plugin->config.out_r,
				(plugin->config.out_g << 8) | plugin->config.out_g,
				(plugin->config.out_b << 8) | plugin->config.out_b,
				out1,
				out2,
				out3);
			out4 = (plugin->config.out_a << 8) | plugin->config.out_a;
			CREATE_GRADIENT(uint16_t, 4, 0xffff)
			break;
	}
}






GradientServer::GradientServer(GradientMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void GradientServer::init_packages()
{
	for(int i = 0; i < total_packages; i++)
	{
		GradientPackage *package = (GradientPackage*)packages[i];
		package->y1 = plugin->input->get_h() * 
			i / 
			total_packages;
		package->y2 = package->y1 +
			plugin->input->get_h() /
			total_packages;
		if(i >= total_packages - 1) package->y2 = plugin->input->get_h();
	}
}

LoadClient* GradientServer::new_client()
{
	return new GradientUnit(this, plugin);
}

LoadPackage* GradientServer::new_package()
{
	return new GradientPackage;
}





