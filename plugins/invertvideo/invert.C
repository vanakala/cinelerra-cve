
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "picon_png.h"
#include "plugincolors.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>



class InvertVideoEffect;


class InvertVideoConfig
{
public:
	InvertVideoConfig();

	void copy_from(InvertVideoConfig &src);
	int equivalent(InvertVideoConfig &src);
	void interpolate(InvertVideoConfig &prev, 
		InvertVideoConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	int r, g, b, a;
};

class InvertVideoEnable : public BC_CheckBox
{
public:
	InvertVideoEnable(InvertVideoEffect *plugin, int *output, int x, int y, char *text);
	int handle_event();
	InvertVideoEffect *plugin;
	int *output;
};

class InvertVideoWindow : public BC_Window
{
public:
	InvertVideoWindow(InvertVideoEffect *plugin, int x, int y);
	void create_objects();
	int close_event();
	InvertVideoEnable *r, *g, *b, *a;
	InvertVideoEffect *plugin;
};

PLUGIN_THREAD_HEADER(InvertVideoEffect, InvertVideoThread, InvertVideoWindow)

class InvertVideoEffect : public PluginVClient
{
public:
	InvertVideoEffect(PluginServer *server);
	~InvertVideoEffect();
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int show_gui();
	void raise_window();
	int set_string();
	int load_configuration();
	int handle_opengl();

	InvertVideoConfig config;
	InvertVideoThread *thread;
	BC_Hash *defaults;
};





REGISTER_PLUGIN(InvertVideoEffect)







InvertVideoConfig::InvertVideoConfig()
{
	r = 1;
	g = 1;
	b = 1;
	a = 1;
}

void InvertVideoConfig::copy_from(InvertVideoConfig &src)
{
	r = src.r;
	g = src.g;
	b = src.b;
	a = src.a;
}

int InvertVideoConfig::equivalent(InvertVideoConfig &src)
{
	return r == src.r && 
		g == src.g && 
		b == src.b && 
		a == src.a;
}

void InvertVideoConfig::interpolate(InvertVideoConfig &prev, 
	InvertVideoConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	r = prev.r;
	g = prev.g;
	b = prev.b;
	a = prev.a;
}




InvertVideoEnable::InvertVideoEnable(InvertVideoEffect *plugin, int *output, int x, int y, char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->plugin = plugin;
	this->output = output;
}
int InvertVideoEnable::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}





InvertVideoWindow::InvertVideoWindow(InvertVideoEffect *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	260, 
	130, 
	260, 
	130, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

void InvertVideoWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(r = new InvertVideoEnable(plugin, &plugin->config.r, x, y, _("Invert R")));
	y += 30;
	add_subwindow(g = new InvertVideoEnable(plugin, &plugin->config.g, x, y, _("Invert G")));
	y += 30;
	add_subwindow(b = new InvertVideoEnable(plugin, &plugin->config.b, x, y, _("Invert B")));
	y += 30;
	add_subwindow(a = new InvertVideoEnable(plugin, &plugin->config.a, x, y, _("Invert A")));

	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(InvertVideoWindow)





PLUGIN_THREAD_OBJECT(InvertVideoEffect, InvertVideoThread, InvertVideoWindow)






InvertVideoEffect::InvertVideoEffect(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}
InvertVideoEffect::~InvertVideoEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
}

char* InvertVideoEffect::plugin_title() { return N_("Invert Video"); }
int InvertVideoEffect::is_realtime() { return 1; }

NEW_PICON_MACRO(InvertVideoEffect)
SHOW_GUI_MACRO(InvertVideoEffect, InvertVideoThread)
RAISE_WINDOW_MACRO(InvertVideoEffect)
SET_STRING_MACRO(InvertVideoEffect)
LOAD_CONFIGURATION_MACRO(InvertVideoEffect, InvertVideoConfig)

void InvertVideoEffect::update_gui()
{
	if(thread)
	{
		thread->window->lock_window();
		load_configuration();
		thread->window->r->update(config.r);
		thread->window->g->update(config.g);
		thread->window->b->update(config.b);
		thread->window->a->update(config.a);
		thread->window->unlock_window();
	}
}

int InvertVideoEffect::load_defaults()
{
	char directory[BCTEXTLEN];
	sprintf(directory, "%sinvertvideo.rc", BCASTDIR);
	defaults = new BC_Hash(directory);
	defaults->load();
	config.r = defaults->get("R", config.r);
	config.g = defaults->get("G", config.g);
	config.b = defaults->get("B", config.b);
	config.a = defaults->get("A", config.a);
	return 0;
}

int InvertVideoEffect::save_defaults()
{
	defaults->update("R", config.r);
	defaults->update("G", config.g);
	defaults->update("B", config.b);
	defaults->update("A", config.a);
	defaults->save();
	return 0;
}

void InvertVideoEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("INVERTVIDEO");
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.tag.set_title("/INVERTVIDEO");
	output.append_tag();
	output.terminate_string();
}

void InvertVideoEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));
	while(!input.read_tag())
	{
		if(input.tag.title_is("INVERTVIDEO"))
		{
			config.r = input.tag.get_property("R", config.r);
			config.g = input.tag.get_property("G", config.g);
			config.b = input.tag.get_property("B", config.b);
			config.a = input.tag.get_property("A", config.a);
		}
	}
}


#define INVERT_MACRO(type, components, max) \
{ \
	for(int i = 0; i < frame->get_h(); i++) \
	{ \
		type *in_row = (type*)frame->get_rows()[i]; \
		type *out_row = (type*)frame->get_rows()[i]; \
 \
		for(int j = 0; j < w; j++) \
		{ \
			if(config.r) out_row[0] = max - in_row[0]; \
			if(config.g) out_row[1] = max - in_row[1]; \
			if(config.b) out_row[2] = max - in_row[2]; \
			if(components == 4) \
				if(config.a) out_row[3] = max - in_row[3]; \
 \
			in_row += components; \
			out_row += components; \
		} \
	} \
}

int InvertVideoEffect::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	read_frame(frame, 
		0, 
		start_position, 
		frame_rate,
		get_use_opengl());


	if(config.r || config.g || config.b || config.a)
	{
		if(get_use_opengl())
		{
			run_opengl();
			return 0;
		}
		int w = frame->get_w();

		switch(frame->get_color_model())
		{
			case BC_RGB_FLOAT:
				INVERT_MACRO(float, 3, 1.0)
				break;
			case BC_RGB888:
			case BC_YUV888:
				INVERT_MACRO(unsigned char, 3, 0xff)
				break;
			case BC_RGBA_FLOAT:
				INVERT_MACRO(float, 4, 1.0)
				break;
			case BC_RGBA8888:
			case BC_YUVA8888:
				INVERT_MACRO(unsigned char, 4, 0xff)
				break;
			case BC_RGB161616:
			case BC_YUV161616:
				INVERT_MACRO(uint16_t, 3, 0xffff)
				break;
			case BC_RGBA16161616:
			case BC_YUVA16161616:
				INVERT_MACRO(uint16_t, 4, 0xffff)
				break;
		}
	}

	return 0;
}

int InvertVideoEffect::handle_opengl()
{
#ifdef HAVE_GL
	static char *invert_frag = 
		"uniform sampler2D tex;\n"
		"uniform bool do_r;\n"
		"uniform bool do_g;\n"
		"uniform bool do_b;\n"
		"uniform bool do_a;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
		"	if(do_r) gl_FragColor.r = 1.0 - gl_FragColor.r;\n"
		"	if(do_g) gl_FragColor.g = 1.0 - gl_FragColor.g;\n"
		"	if(do_b) gl_FragColor.b = 1.0 - gl_FragColor.b;\n"
		"	if(do_a) gl_FragColor.a = 1.0 - gl_FragColor.a;\n"
		"}\n";

	get_output()->to_texture();
	get_output()->enable_opengl();

	unsigned int frag_shader = 0;
	frag_shader = VFrame::make_shader(0,
		invert_frag,
		0);
	glUseProgram(frag_shader);
	glUniform1i(glGetUniformLocation(frag_shader, "tex"), 0);
	glUniform1i(glGetUniformLocation(frag_shader, "do_r"), config.r);
	glUniform1i(glGetUniformLocation(frag_shader, "do_g"), config.g);
	glUniform1i(glGetUniformLocation(frag_shader, "do_b"), config.b);
	glUniform1i(glGetUniformLocation(frag_shader, "do_a"), config.a);


	VFrame::init_screen(get_output()->get_w(), get_output()->get_h());
	get_output()->bind_texture(0);
	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
}


