
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
#include "keyframe.h"
#include "language.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <string.h>
#include <stdint.h>


class Overlay;
class OverlayWindow;


class OverlayConfig
{
public:
	OverlayConfig();



	static char* mode_to_text(int mode);
	int mode;

	static char* direction_to_text(int direction);
	int direction;
	enum
	{
		BOTTOM_FIRST,
		TOP_FIRST
	};

	static char* output_to_text(int output_layer);
	int output_layer;
	enum
	{
		TOP,
		BOTTOM
	};
};





class OverlayMode : public BC_PopupMenu
{
public:
	OverlayMode(Overlay *plugin,
		int x, 
		int y);
	void create_objects();
	int handle_event();
	Overlay *plugin;
};

class OverlayDirection : public BC_PopupMenu
{
public:
	OverlayDirection(Overlay *plugin,
		int x, 
		int y);
	void create_objects();
	int handle_event();
	Overlay *plugin;
};

class OverlayOutput : public BC_PopupMenu
{
public:
	OverlayOutput(Overlay *plugin,
		int x, 
		int y);
	void create_objects();
	int handle_event();
	Overlay *plugin;
};


class OverlayWindow : public BC_Window
{
public:
	OverlayWindow(Overlay *plugin, int x, int y);
	~OverlayWindow();

	void create_objects();
	int close_event();

	Overlay *plugin;
	OverlayMode *mode;
	OverlayDirection *direction;
	OverlayOutput *output;
};


PLUGIN_THREAD_HEADER(Overlay, OverlayThread, OverlayWindow)



class Overlay : public PluginVClient
{
public:
	Overlay(PluginServer *server);
	~Overlay();


	PLUGIN_CLASS_MEMBERS(OverlayConfig, OverlayThread);

	int process_buffer(VFrame **frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	int is_multichannel();
	int is_synthesis();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int handle_opengl();

	OverlayFrame *overlayer;
	VFrame *temp;
	int current_layer;
	int output_layer;
// Inclusive layer numbers
	int input_layer1;
	int input_layer2;
};












OverlayConfig::OverlayConfig()
{
	mode = TRANSFER_NORMAL;
	direction = OverlayConfig::BOTTOM_FIRST;
	output_layer = OverlayConfig::TOP;
}

char* OverlayConfig::mode_to_text(int mode)
{
	switch(mode)
	{
		case TRANSFER_NORMAL:
			return "Normal";
			break;

		case TRANSFER_REPLACE:
			return "Replace";
			break;

		case TRANSFER_ADDITION:
			return "Addition";
			break;

		case TRANSFER_SUBTRACT:
			return "Subtract";
			break;

		case TRANSFER_MULTIPLY:
			return "Multiply";
			break;

		case TRANSFER_DIVIDE:
			return "Divide";
			break;

		case TRANSFER_MAX:
			return "Max";
			break;

		default:
			return "Normal";
			break;
	}
	return "";
}

char* OverlayConfig::direction_to_text(int direction)
{
	switch(direction)
	{
		case OverlayConfig::BOTTOM_FIRST: return _("Bottom first");
		case OverlayConfig::TOP_FIRST:    return _("Top first");
	}
	return "";
}

char* OverlayConfig::output_to_text(int output_layer)
{
	switch(output_layer)
	{
		case OverlayConfig::TOP:    return _("Top");
		case OverlayConfig::BOTTOM: return _("Bottom");
	}
	return "";
}









OverlayWindow::OverlayWindow(Overlay *plugin, int x, int y)
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

OverlayWindow::~OverlayWindow()
{
}

void OverlayWindow::create_objects()
{
	int x = 10, y = 10;

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Mode:")));
	add_subwindow(mode = new OverlayMode(plugin, 
		x + title->get_w() + 5, 
		y));
	mode->create_objects();

	y += 30;
	add_subwindow(title = new BC_Title(x, y, _("Layer order:")));
	add_subwindow(direction = new OverlayDirection(plugin, 
		x + title->get_w() + 5, 
		y));
	direction->create_objects();

	y += 30;
	add_subwindow(title = new BC_Title(x, y, _("Output layer:")));
	add_subwindow(output = new OverlayOutput(plugin, 
		x + title->get_w() + 5, 
		y));
	output->create_objects();

	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(OverlayWindow)





OverlayMode::OverlayMode(Overlay *plugin,
	int x, 
	int y)
 : BC_PopupMenu(x,
 	y,
	150,
	OverlayConfig::mode_to_text(plugin->config.mode),
	1)
{
	this->plugin = plugin;
}

void OverlayMode::create_objects()
{
	for(int i = 0; i < TRANSFER_TYPES; i++)
		add_item(new BC_MenuItem(OverlayConfig::mode_to_text(i)));
}

int OverlayMode::handle_event()
{
	char *text = get_text();

	for(int i = 0; i < TRANSFER_TYPES; i++)
	{
		if(!strcmp(text, OverlayConfig::mode_to_text(i)))
		{
			plugin->config.mode = i;
			break;
		}
	}

	plugin->send_configure_change();
	return 1;
}


OverlayDirection::OverlayDirection(Overlay *plugin,
	int x, 
	int y)
 : BC_PopupMenu(x,
 	y,
	150,
	OverlayConfig::direction_to_text(plugin->config.direction),
	1)
{
	this->plugin = plugin;
}

void OverlayDirection::create_objects()
{
	add_item(new BC_MenuItem(
		OverlayConfig::direction_to_text(
			OverlayConfig::TOP_FIRST)));
	add_item(new BC_MenuItem(
		OverlayConfig::direction_to_text(
			OverlayConfig::BOTTOM_FIRST)));
}

int OverlayDirection::handle_event()
{
	char *text = get_text();

	if(!strcmp(text, 
		OverlayConfig::direction_to_text(
			OverlayConfig::TOP_FIRST)))
		plugin->config.direction = OverlayConfig::TOP_FIRST;
	else
	if(!strcmp(text, 
		OverlayConfig::direction_to_text(
			OverlayConfig::BOTTOM_FIRST)))
		plugin->config.direction = OverlayConfig::BOTTOM_FIRST;

	plugin->send_configure_change();
	return 1;
}


OverlayOutput::OverlayOutput(Overlay *plugin,
	int x, 
	int y)
 : BC_PopupMenu(x,
 	y,
	100,
	OverlayConfig::output_to_text(plugin->config.output_layer),
	1)
{
	this->plugin = plugin;
}

void OverlayOutput::create_objects()
{
	add_item(new BC_MenuItem(
		OverlayConfig::output_to_text(
			OverlayConfig::TOP)));
	add_item(new BC_MenuItem(
		OverlayConfig::output_to_text(
			OverlayConfig::BOTTOM)));
}

int OverlayOutput::handle_event()
{
	char *text = get_text();

	if(!strcmp(text, 
		OverlayConfig::output_to_text(
			OverlayConfig::TOP)))
		plugin->config.output_layer = OverlayConfig::TOP;
	else
	if(!strcmp(text, 
		OverlayConfig::output_to_text(
			OverlayConfig::BOTTOM)))
		plugin->config.output_layer = OverlayConfig::BOTTOM;

	plugin->send_configure_change();
	return 1;
}









PLUGIN_THREAD_OBJECT(Overlay, OverlayThread, OverlayWindow)










REGISTER_PLUGIN(Overlay)






Overlay::Overlay(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	overlayer = 0;
	temp = 0;
}


Overlay::~Overlay()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(overlayer) delete overlayer;
	if(temp) delete temp;
}



int Overlay::process_buffer(VFrame **frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	if(!temp) temp = new VFrame(0,
		frame[0]->get_w(),
		frame[0]->get_h(),
		frame[0]->get_color_model(),
		-1);

	if(!overlayer)
		overlayer = new OverlayFrame(get_project_smp() + 1);

	int step;
	VFrame *output;

	if(config.direction == OverlayConfig::BOTTOM_FIRST)
	{
		input_layer1 = get_total_buffers() - 1;
		input_layer2 = -1;
		step = -1;
	}
	else
	{
		input_layer1 = 0;
		input_layer2 = get_total_buffers();
		step = 1;
	}

	if(config.output_layer == OverlayConfig::TOP)
	{
		output_layer = 0;
	}
	else
	{
		output_layer = get_total_buffers() - 1;
	}



// Direct copy the first layer
	output = frame[output_layer];
	read_frame(output, 
		input_layer1, 
		start_position,
		frame_rate,
		get_use_opengl());

	if(get_total_buffers() == 1) return 0;



	current_layer = input_layer1;
	if(get_use_opengl()) 
		run_opengl();

	for(int i = input_layer1 + step; i != input_layer2; i += step)
	{
		read_frame(temp, 
			i, 
			start_position,
			frame_rate,
			get_use_opengl());

		if(get_use_opengl()) 
		{
			current_layer = i;
			run_opengl();
		}
		else
// Call the opengl handler once for each layer
			overlayer->overlay(output,
				temp,
				0,
				0,
				output->get_w(),
				output->get_h(),
				0,
				0,
				output->get_w(),
				output->get_h(),
				1,
				config.mode,
				NEAREST_NEIGHBOR);
	}


	return 0;
}

int Overlay::handle_opengl()
{
#ifdef HAVE_GL
	static char *get_pixels_frag = 
		"uniform sampler2D src_tex;\n"
		"uniform sampler2D dst_tex;\n"
		"uniform vec2 dst_tex_dimensions;\n"
		"uniform vec3 chroma_offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 result_color;\n"
		"	vec4 dst_color = texture2D(dst_tex, gl_FragCoord.xy / dst_tex_dimensions);\n"
		"	vec4 src_color = texture2D(src_tex, gl_TexCoord[0].st);\n"
		"	src_color.rgb -= chroma_offset;\n"
		"	dst_color.rgb -= chroma_offset;\n";

	static char *put_pixels_frag = 
		"	result_color.rgb += chroma_offset;\n"
		"	result_color.rgb = mix(dst_color.rgb, result_color.rgb, src_color.a);\n"
		"	result_color.a = max(src_color.a, dst_color.a);\n"
		"	gl_FragColor = result_color;\n"
		"}\n";

	static char *blend_add_frag = 
		"	result_color.rgb = dst_color.rgb + src_color.rgb;\n";

	static char *blend_max_frag = 
		"	result_color.r = max(abs(dst_color.r, src_color.r);\n"
		"	result_color.g = max(abs(dst_color.g, src_color.g);\n"
		"	result_color.b = max(abs(dst_color.b, src_color.b);\n";

	static char *blend_subtract_frag = 
		"	result_color.rgb = dst_color.rgb - src_color.rgb;\n";


	static char *blend_multiply_frag = 
		"	result_color.rgb = dst_color.rgb * src_color.rgb;\n";

	static char *blend_divide_frag = 
		"	result_color.rgb = dst_color.rgb / src_color.rgb;\n"
		"	if(src_color.r == 0.0) result_color.r = 1.0;\n"
		"	if(src_color.g == 0.0) result_color.g = 1.0;\n"
		"	if(src_color.b == 0.0) result_color.b = 1.0;\n";


	VFrame *src = temp;
	VFrame *dst = get_output(output_layer);

	dst->enable_opengl();
	dst->init_screen();

	char *shader_stack[] = { 0, 0, 0 };
	int current_shader = 0;




// Direct copy layer
	if(config.mode == TRANSFER_REPLACE)
	{
		src->to_texture();
		src->bind_texture(0);
		dst->enable_opengl();
		dst->init_screen();

// Multiply alpha
		glDisable(GL_BLEND);
		src->draw_texture();
	}
	else
	if(config.mode == TRANSFER_NORMAL)
	{
		dst->enable_opengl();
		dst->init_screen();

// Move destination to screen
		if(dst->get_opengl_state() != VFrame::SCREEN)
		{
			dst->to_texture();
			dst->bind_texture(0);
			dst->draw_texture();
		}

		src->to_texture();
		src->bind_texture(0);
		dst->enable_opengl();
		dst->init_screen();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		src->draw_texture();
	}
	else
	{
// Read destination back to texture
		dst->to_texture();

		src->enable_opengl();
		src->init_screen();
		src->to_texture();

		dst->enable_opengl();
		dst->init_screen();
		src->bind_texture(0);
		dst->bind_texture(1);


		shader_stack[current_shader++] = get_pixels_frag;

		switch(config.mode)
		{
			case TRANSFER_ADDITION:
				shader_stack[current_shader++] = blend_add_frag;
				break;
			case TRANSFER_SUBTRACT:
				shader_stack[current_shader++] = blend_subtract_frag;
				break;
			case TRANSFER_MULTIPLY:
				shader_stack[current_shader++] = blend_multiply_frag;
				break;
			case TRANSFER_DIVIDE:
				shader_stack[current_shader++] = blend_divide_frag;
				break;
			case TRANSFER_MAX:
				shader_stack[current_shader++] = blend_max_frag;
				break;
		}

		shader_stack[current_shader++] = put_pixels_frag;

		unsigned int shader_id = 0;
		shader_id = VFrame::make_shader(0,
			shader_stack[0],
			shader_stack[1],
			shader_stack[2],
			0);

		glUseProgram(shader_id);
		glUniform1i(glGetUniformLocation(shader_id, "src_tex"), 0);
		glUniform1i(glGetUniformLocation(shader_id, "dst_tex"), 1);
		if(cmodel_is_yuv(dst->get_color_model()))
			glUniform3f(glGetUniformLocation(shader_id, "chroma_offset"), 0.0, 0.5, 0.5);
		else
			glUniform3f(glGetUniformLocation(shader_id, "chroma_offset"), 0.0, 0.0, 0.0);
		glUniform2f(glGetUniformLocation(shader_id, "dst_tex_dimensions"), 
			(float)dst->get_texture_w(), 
			(float)dst->get_texture_h());

		glDisable(GL_BLEND);
		src->draw_texture();
		glUseProgram(0);
	}

	glDisable(GL_BLEND);
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);

	dst->set_opengl_state(VFrame::SCREEN);
#endif
}


char* Overlay::plugin_title() { return N_("Overlay"); }
int Overlay::is_realtime() { return 1; }
int Overlay::is_multichannel() { return 1; }
int Overlay::is_synthesis() { return 1; }


NEW_PICON_MACRO(Overlay) 

SHOW_GUI_MACRO(Overlay, OverlayThread)

RAISE_WINDOW_MACRO(Overlay)

SET_STRING_MACRO(Overlay);

int Overlay::load_configuration()
{
	KeyFrame *prev_keyframe;
	prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(prev_keyframe);
	return 0;
}

int Overlay::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%soverlay.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.mode = defaults->get("MODE", config.mode);
	config.direction = defaults->get("DIRECTION", config.direction);
	config.output_layer = defaults->get("OUTPUT_LAYER", config.output_layer);
	return 0;
}

int Overlay::save_defaults()
{
	defaults->update("MODE", config.mode);
	defaults->update("DIRECTION", config.direction);
	defaults->update("OUTPUT_LAYER", config.output_layer);
	defaults->save();
	return 0;
}

void Overlay::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("OVERLAY");
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("DIRECTION", config.direction);
	output.tag.set_property("OUTPUT_LAYER", config.output_layer);
	output.append_tag();
	output.tag.set_title("/OVERLAY");
	output.append_tag();
	output.terminate_string();
}

void Overlay::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("OVERLAY"))
		{
			config.mode = input.tag.get_property("MODE", config.mode);
			config.direction = input.tag.get_property("DIRECTION", config.direction);
			config.output_layer = input.tag.get_property("OUTPUT_LAYER", config.output_layer);
		}
	}
}

void Overlay::update_gui()
{
	if(thread)
	{
		thread->window->lock_window("Overlay::update_gui");
		thread->window->mode->set_text(OverlayConfig::mode_to_text(config.mode));
		thread->window->direction->set_text(OverlayConfig::direction_to_text(config.direction));
		thread->window->output->set_text(OverlayConfig::output_to_text(config.output_layer));
		thread->window->unlock_window();
	}
}





