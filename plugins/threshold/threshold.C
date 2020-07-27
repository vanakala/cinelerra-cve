// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "clip.h"
#include "bchash.h"
#include "bcresources.h"
#include "colorspaces.h"
#include "filexml.h"
#include "histogramengine.h"
#include "language.h"
#include "picon_png.h"
#include "tmpframecache.h"
#include "threshold.h"
#include "thresholdwindow.h"
#include "vframe.h"

#include <string.h>
#include <string>

using std::string;

ThresholdConfig::ThresholdConfig()
{
	min = 0.0;
	max = 1.0;
	plot = 1;
	low_color.set (0x0,  0x0,  0x0,  0xffff);
	mid_color.set (0xffff, 0xffff, 0xffff, 0xffff);
	high_color.set(0x0,  0x0,  0x0,  0xffff);
}

int ThresholdConfig::equivalent(ThresholdConfig &that)
{
	return EQUIV(min, that.min) &&
		EQUIV(max, that.max) &&
		plot == that.plot &&
		low_color == that.low_color &&
		mid_color == that.mid_color &&
		high_color == that.high_color;
}

void ThresholdConfig::copy_from(ThresholdConfig &that)
{
	min = that.min;
	max = that.max;
	plot = that.plot;
	low_color = that.low_color;
	mid_color = that.mid_color;
	high_color = that.high_color;
}

// General purpose scale function.
template<typename T>
T interpolate(const T & prev, const double & prev_scale, const T & next, const double & next_scale)
{
	return static_cast<T>(prev * prev_scale + next * next_scale);
}

void ThresholdConfig::interpolate(ThresholdConfig &prev,
	ThresholdConfig &next,
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	min = ::interpolate(prev.min, prev_scale, next.min, next_scale);
	max = ::interpolate(prev.max, prev_scale, next.max, next_scale);
	plot = prev.plot;

	low_color =  ::interpolate(prev.low_color,  prev_scale, next.low_color,  next_scale);
	mid_color =  ::interpolate(prev.mid_color,  prev_scale, next.mid_color,  next_scale);
	high_color = ::interpolate(prev.high_color, prev_scale, next.high_color, next_scale);
}

void ThresholdConfig::boundaries()
{
	CLAMP(min, HISTOGRAM_MIN, max);
	CLAMP(max, min, HISTOGRAM_MAX);
}


REGISTER_PLUGIN


ThresholdMain::ThresholdMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	threshold_engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

ThresholdMain::~ThresholdMain()
{
	delete engine;
	delete threshold_engine;
	PLUGIN_DESTRUCTOR_MACRO
}

void ThresholdMain::reset_plugin()
{
	if(engine)
	{
		delete engine;
		engine = 0;
		delete threshold_engine;
		threshold_engine = 0;
	}
}

PLUGIN_CLASS_METHODS


VFrame *ThresholdMain::process_tmpframe(VFrame *frame)
{
	int cmodel = frame->get_color_model();

	switch(cmodel)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(cmodel);
		return frame;
	}

	load_configuration();

	if(config.plot)
		calculate_histogram(frame);
	if(!threshold_engine)
		threshold_engine = new ThresholdEngine(this);
	threshold_engine->process_packages(frame);
	update_gui();
	return frame;
}

void ThresholdMain::load_defaults()
{
	defaults = load_defaults_file("threshold.rc");
	config.min = defaults->get("MIN", config.min);
	config.max = defaults->get("MAX", config.max);
	config.plot = defaults->get("PLOT", config.plot);

	config.low_color.r = adjusted_default(defaults, "LOW_COLOR_R",
		"LOW_COLOR_RE", config.low_color.r);
	config.low_color.g = adjusted_default(defaults, "LOW_COLOR_G",
		"LOW_COLOR_GR", config.low_color.g);
	config.low_color.b = adjusted_default(defaults, "LOW_COLOR_B",
		"LOW_COLOR_BL", config.low_color.b);
	config.low_color.a = adjusted_default(defaults, "LOW_COLOR_A",
		"LOW_COLOR_AL", config.low_color.a);

	config.mid_color.r = adjusted_default(defaults, "MID_COLOR_R",
		"MID_COLOR_RE", config.mid_color.r);
	config.mid_color.g = adjusted_default(defaults, "MID_COLOR_G",
		"MID_COLOR_GR", config.mid_color.g);
	config.mid_color.b = adjusted_default(defaults, "MID_COLOR_B",
		"MID_COLOR_BL", config.mid_color.b);
	config.mid_color.a = adjusted_default(defaults, "MID_COLOR_A",
		"MID_COLOR_AL", config.mid_color.a);

	config.high_color.r = adjusted_default(defaults, "HIGH_COLOR_R",
		"HIGH_COLOR_RE", config.high_color.r);
	config.high_color.g = adjusted_default(defaults, "HIGH_COLOR_G",
		"HIGH_COLOR_GR", config.high_color.g);
	config.high_color.b = adjusted_default(defaults, "HIGH_COLOR_B",
		"HIGH_COLOR_BL", config.high_color.b);
	config.high_color.a = adjusted_default(defaults, "HIGH_COLOR_A",
		"HIGH_COLOR_AL", config.high_color.a);
	config.boundaries();
}

int ThresholdMain::adjusted_default(BC_Hash *defaults,
	const char *oldkey, const char *newkey, int default_value)
{
	int val = defaults->get(oldkey, 0);

	if(val < 256 && val > 0)
		val = (val << 8) | val;
	else
		val = defaults->get(newkey, default_value);
	return val;
}

int ThresholdMain::adjusted_property(FileXML *file,
	const char *oldkey, const char *newkey, int default_value)
{
	int val = file->tag.get_property(oldkey, 0);

	if(val < 256 && val > 0)
		val = (val << 8) | val;
	else
		val = file->tag.get_property(newkey, default_value);
	return val;
}

void ThresholdMain::save_defaults()
{
	defaults->update("MIN", config.min);
	defaults->update("MAX", config.max);
	defaults->update("PLOT", config.plot);

	defaults->delete_key("LOW_COLOR_R");
	defaults->delete_key("LOW_COLOR_G");
	defaults->delete_key("LOW_COLOR_B");
	defaults->delete_key("LOW_COLOR_A");

	defaults->delete_key("MID_COLOR_R");
	defaults->delete_key("MID_COLOR_G");
	defaults->delete_key("MID_COLOR_B");
	defaults->delete_key("MID_COLOR_A");

	defaults->delete_key("HIGH_COLOR_R");
	defaults->delete_key("HIGH_COLOR_G");
	defaults->delete_key("HIGH_COLOR_B");
	defaults->delete_key("HIGH_COLOR_A");

	defaults->update("LOW_COLOR_RE", config.low_color.r);
	defaults->update("LOW_COLOR_GR", config.low_color.g);
	defaults->update("LOW_COLOR_BL", config.low_color.b);
	defaults->update("LOW_COLOR_AL", config.low_color.a);

	defaults->update("MID_COLOR_RE", config.mid_color.r);
	defaults->update("MID_COLOR_GR", config.mid_color.g);
	defaults->update("MID_COLOR_BL", config.mid_color.b);
	defaults->update("MID_COLOR_AL", config.mid_color.a);

	defaults->update("HIGH_COLOR_RE", config.high_color.r);
	defaults->update("HIGH_COLOR_GR", config.high_color.g);
	defaults->update("HIGH_COLOR_BL", config.high_color.b);
	defaults->update("HIGH_COLOR_AL", config.high_color.a);
}

void ThresholdMain::save_data(KeyFrame *keyframe)
{
	FileXML file;

	file.tag.set_title("THRESHOLD");
	file.tag.set_property("MIN", config.min);
	file.tag.set_property("MAX", config.max);
	file.tag.set_property("PLOT", config.plot);

	file.tag.set_property("LOW_COLOR_RE", config.low_color.r);
	file.tag.set_property("LOW_COLOR_GR", config.low_color.g);
	file.tag.set_property("LOW_COLOR_BL", config.low_color.b);
	file.tag.set_property("LOW_COLOR_AL", config.low_color.a);

	file.tag.set_property("MID_COLOR_RE", config.mid_color.r);
	file.tag.set_property("MID_COLOR_GR", config.mid_color.g);
	file.tag.set_property("MID_COLOR_BL", config.mid_color.b);
	file.tag.set_property("MID_COLOR_AL", config.mid_color.a);

	file.tag.set_property("HIGH_COLOR_RE", config.high_color.r);
	file.tag.set_property("HIGH_COLOR_GR", config.high_color.g);
	file.tag.set_property("HIGH_COLOR_BL", config.high_color.b);
	file.tag.set_property("HIGH_COLOR_AL", config.high_color.a);

	file.append_tag();
	file.tag.set_title("/THRESHOLD");
	file.append_tag();
	keyframe->set_data(file.string);
}

void ThresholdMain::read_data(KeyFrame *keyframe)
{
	FileXML file;

	file.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!file.read_tag())
	{
		config.min = file.tag.get_property("MIN", config.min);
		config.max = file.tag.get_property("MAX", config.max);
		config.plot = file.tag.get_property("PLOT", config.plot);

		config.low_color.r = adjusted_property(&file, "LOW_COLOR_R",
			"LOW_COLOR_RE", config.low_color.r);
		config.low_color.g = adjusted_property(&file, "LOW_COLOR_G",
			"LOW_COLOR_GR", config.low_color.g);
		config.low_color.b = adjusted_property(&file, "LOW_COLOR_B",
			"LOW_COLOR_BL", config.low_color.b);
		config.low_color.a = adjusted_property(&file, "LOW_COLOR_A",
			"LOW_COLOR_AL", config.low_color.a);

		config.mid_color.r = adjusted_property(&file, "MID_COLOR_R",
			"MID_COLOR_RE", config.mid_color.r);
		config.mid_color.g = adjusted_property(&file, "MID_COLOR_G",
			"MID_COLOR_GR",config.mid_color.g);
		config.mid_color.b = adjusted_property(&file, "MID_COLOR_B",
			"MID_COLOR_BL", config.mid_color.b);
		config.mid_color.a = adjusted_property(&file, "MID_COLOR_A",
			"MID_COLOR_AL", config.mid_color.a);

		config.high_color.r = adjusted_property(&file, "HIGH_COLOR_R",
			"HIGH_COLOR_RE", config.high_color.r);
		config.high_color.g = adjusted_property(&file, "HIGH_COLOR_G",
			"HIGH_COLOR_GR", config.high_color.g);
		config.high_color.b = adjusted_property(&file, "HIGH_COLOR_B",
			"HIGH_COLOR_BL", config.high_color.b);
		config.high_color.a = adjusted_property(&file, "HIGH_COLOR_A",
			"HIGH_COLOR_AL", config.high_color.a);
	}
	config.boundaries();
}

void ThresholdMain::calculate_histogram(VFrame *frame)
{
	if(!engine)
		engine = new HistogramEngine(get_project_smp(),
			get_project_smp());

	engine->process_packages(frame);
}

void ThresholdMain::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	static const char *rgb_shader = 
		"uniform sampler2D tex;\n"
		"uniform float min;\n"
		"uniform float max;\n"
		"uniform vec4 low_color;\n"
		"uniform vec4 mid_color;\n"
		"uniform vec4 high_color;\n"
		"void main()\n"
		"{\n"
		"	vec4 pixel = texture2D(tex, gl_TexCoord[0].st);\n"
		"	float v = dot(pixel.rgb, vec3(0.299, 0.587, 0.114));\n"
		"	if(v < min)\n"
		"		pixel = low_color;\n"
		"	else if(v < max)\n"
		"		pixel = mid_color;\n"
		"	else\n"
		"		pixel = high_color;\n"
		"	gl_FragColor = pixel;\n"
		"}\n";

	static const char *yuv_shader = 
		"uniform sampler2D tex;\n"
		"uniform float min;\n"
		"uniform float max;\n"
		"uniform vec4 low_color;\n"
		"uniform vec4 mid_color;\n"
		"uniform vec4 high_color;\n"
		"void main()\n"
		"{\n"
		"	vec4 pixel = texture2D(tex, gl_TexCoord[0].st);\n"
		"	if(pixel.r < min)\n"
		"		pixel = low_color;\n"
		"	else if(pixel.r < max)\n"
		"		pixel = mid_color;\n"
		"	else\n"
		"		pixel = high_color;\n"
		"	gl_FragColor = pixel;\n"
		"}\n";
	get_output()->to_texture();
	get_output()->enable_opengl();

	unsigned int shader = 0;
	int color_model = get_output()->get_color_model();
	bool is_yuv = ColorModels::is_yuv(color_model);
	bool has_alpha = ColorModels::has_alpha(color_model);
	if(is_yuv)
		shader = VFrame::make_shader(0, yuv_shader, 0);
	else
		shader = VFrame::make_shader(0, rgb_shader, 0);

	if(shader > 0)
	{
		glUseProgram(shader);
		glUniform1i(glGetUniformLocation(shader, "tex"), 0);
		glUniform1f(glGetUniformLocation(shader, "min"), config.min);
		glUniform1f(glGetUniformLocation(shader, "max"), config.max);

		if (is_yuv)
		{
			float y_low,  u_low,  v_low;
			float y_mid,  u_mid,  v_mid;
			float y_high, u_high, v_high;

			ColorSpaces::rgb_to_yuv_f((float)config.low_color.r / 0xff,
					(float)config.low_color.g / 0xff,
					(float)config.low_color.b / 0xff,
					y_low,
					u_low,
					v_low);
			u_low += 0.5;
			v_low += 0.5;
			ColorSpaces::rgb_to_yuv_f((float)config.mid_color.r / 0xff,
					(float)config.mid_color.g / 0xff,
					(float)config.mid_color.b / 0xff,
					y_mid,
					u_mid,
					v_mid);
			u_mid += 0.5;
			v_mid += 0.5;
			ColorSpaces::rgb_to_yuv_f((float)config.high_color.r / 0xff,
					(float)config.high_color.g / 0xff,
					(float)config.high_color.b / 0xff,
					y_high,
					u_high,
					v_high);
			u_high += 0.5;
			v_high += 0.5;

			glUniform4f(glGetUniformLocation(shader, "low_color"),
				y_low, u_low, v_low,
				has_alpha ? (float)config.low_color.a / 0xff : 1.0);
			glUniform4f(glGetUniformLocation(shader, "mid_color"),
				y_mid, u_mid, v_mid,
				has_alpha ? (float)config.mid_color.a / 0xff : 1.0);
			glUniform4f(glGetUniformLocation(shader, "high_color"),
				y_high, u_high, v_high,
				has_alpha ? (float)config.high_color.a / 0xff : 1.0);
		} else {
			glUniform4f(glGetUniformLocation(shader, "low_color"),
				(float)config.low_color.r / 0xff,
				(float)config.low_color.g / 0xff,
				(float)config.low_color.b / 0xff,
				has_alpha ? (float)config.low_color.a / 0xff : 1.0);
			glUniform4f(glGetUniformLocation(shader, "mid_color"),
				(float)config.mid_color.r / 0xff,
				(float)config.mid_color.g / 0xff,
				(float)config.mid_color.b / 0xff,
				has_alpha ? (float)config.mid_color.a / 0xff : 1.0);
			glUniform4f(glGetUniformLocation(shader, "high_color"),
				(float)config.high_color.r / 0xff,
				(float)config.high_color.g / 0xff,
				(float)config.high_color.b / 0xff,
				has_alpha ? (float)config.high_color.a / 0xff : 1.0);
		}
	}

	get_output()->init_screen();
	get_output()->bind_texture(0);
	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}


ThresholdPackage::ThresholdPackage()
 : LoadPackage()
{
	start = end = 0;
}


ThresholdUnit::ThresholdUnit(ThresholdEngine *server)
 : LoadClient(server)
{
	this->server = server;
}

void ThresholdUnit::process_package(LoadPackage *package)
{
	const ThresholdPackage *pkg = (ThresholdPackage*)package;
	const ThresholdConfig *config = &server->plugin->config;
	VFrame *data = server->data;
	const int min = round(config->min * 0xffff);
	const int max = round(config->max * 0xffff);
	const int w = data->get_w();
	const int h = data->get_h();

	const uint16_t r_low = config->low_color.r;
	const uint16_t g_low = config->low_color.g;
	const uint16_t b_low = config->low_color.b;
	const uint16_t a_low = config->low_color.a;

	const uint16_t r_mid = config->mid_color.r;
	const uint16_t g_mid = config->mid_color.g;
	const uint16_t b_mid = config->mid_color.b;
	const uint16_t a_mid = config->mid_color.a;

	const uint16_t r_high = config->high_color.r;
	const uint16_t g_high = config->high_color.g;
	const uint16_t b_high = config->high_color.b;
	const uint16_t a_high = config->high_color.a;

	uint16_t y_low,  u_low,  v_low;
	uint16_t y_mid,  u_mid,  v_mid;
	uint16_t y_high, u_high, v_high;

	switch(server->data->get_color_model())
	{
	case BC_RGBA16161616:
		for(int i = pkg->start; i < pkg->end; i++)
		{
			uint16_t *in_row = (uint16_t*)data->get_row_ptr(i);
			uint16_t *out_row = in_row;

			for(int j = 0; j < w; j++)
			{
				int r = in_row[0];
				int g = in_row[1];
				int b = in_row[2];
				int y = ColorSpaces::rgb_to_y_16(r, g, b);

				if(y < min)
				{
					*out_row++ = r_low;
					*out_row++ = g_low;
					*out_row++ = b_low;
					*out_row++ = a_low;
				}
				else if(y < max)
				{
					*out_row++ = r_mid;
					*out_row++ = g_mid;
					*out_row++ = b_mid;
					*out_row++ = a_mid;
				}
				else
				{
					*out_row++ = r_high;
					*out_row++ = g_high;
					*out_row++ = b_high;
					*out_row++ = a_high;
				}
				in_row += 4;
			}
		}
		break;

	case BC_AYUV16161616:
		ColorSpaces::rgb_to_yuv_16(r_low, g_low, b_low,
			y_low, u_low, v_low);
		ColorSpaces::rgb_to_yuv_16(r_mid, g_mid, b_mid,
			y_mid, u_mid, v_mid);
		ColorSpaces::rgb_to_yuv_16(r_high, g_high, b_high,
			y_high, u_high, v_high);

		for(int i = pkg->start; i < pkg->end; i++)
		{
			uint16_t *in_row = (uint16_t*)server->data->get_row_ptr(i);
			uint16_t *out_row = in_row;

			for(int j = 0; j < w; j++)
			{
				int y = in_row[1];

				if(y < min)
				{
					*out_row++ = a_low;
					*out_row++ = y_low;
					*out_row++ = u_low;
					*out_row++ = v_low;
				}
				else if (y < max)
				{
					*out_row++ = a_mid;
					*out_row++ = y_mid;
					*out_row++ = u_mid;
					*out_row++ = v_mid;
				}
				else
				{
					*out_row++ = a_high;
					*out_row++ = y_high;
					*out_row++ = u_high;
					*out_row++ = v_high;
				}
				in_row += 4;
			}
		}
		break;
	}
}


ThresholdEngine::ThresholdEngine(ThresholdMain *plugin)
 : LoadServer(plugin->get_project_smp(),
	plugin->get_project_smp())
{
	this->plugin = plugin;
}

void ThresholdEngine::process_packages(VFrame *data)
{
	this->data = data;
	LoadServer::process_packages();
}

void ThresholdEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		ThresholdPackage *package = (ThresholdPackage*)get_package(i);
		package->start = data->get_h() * i / get_total_packages();
		package->end = data->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* ThresholdEngine::new_client()
{
	return (LoadClient*)new ThresholdUnit(this);
}

LoadPackage* ThresholdEngine::new_package()
{
	return (LoadPackage*)new HistogramPackage;
}


RGBA::RGBA()
{
	r = g = b = a = 0;
}

RGBA::RGBA(int r, int g, int b, int a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

void RGBA::set(int r, int g, int b, int a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

int RGBA::getRGB() const
{
	return ((r << 8) & 0xff0000) | g & 0xff00 | b >> 8;
}

bool operator==(const RGBA & a, const RGBA & b)
{
	return  a.r == b.r &&
		a.g == b.g &&
		a.b == b.b &&
		a.a == b.a;
}

template<>
RGBA interpolate(const RGBA & prev_color, const double & prev_scale, const RGBA &next_color, const double & next_scale)
{
	return RGBA(interpolate(prev_color.r, prev_scale, next_color.r, next_scale),
		interpolate(prev_color.g, prev_scale, next_color.g, next_scale),
		interpolate(prev_color.b, prev_scale, next_color.b, next_scale),
		interpolate(prev_color.a, prev_scale, next_color.a, next_scale));
}
