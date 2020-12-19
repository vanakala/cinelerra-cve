// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bchash.h"
#include "bcmenuitem.h"
#include "bcpopupmenu.h"
#include "bctitle.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "overlay.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"

OverlayConfig::OverlayConfig()
{
	mode = TRANSFER_NORMAL;
	direction = 0;
}

OverlayWindow::OverlayWindow(Overlay *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	300, 
	85)
{
	BC_Title *title;

	x = 10;
	y = 20;

	add_subwindow(title = new BC_Title(x, y, _("Mode:")));
	add_subwindow(mode = new OverlayMode(plugin, 
		x + title->get_w() + 5, 
		y));
	mode->create_objects();
	y += mode->get_h() + 8;
	add_subwindow(direction = new OverlayDirection(plugin, x, y));

	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void OverlayWindow::update()
{
	mode->set_text(_(OverlayFrame::transfer_name(plugin->config.mode)));
}


OverlayMode::OverlayMode(Overlay *plugin,
	int x, 
	int y)
 : BC_PopupMenu(x,
	y,
	150,
	_(OverlayFrame::transfer_name(plugin->config.mode)),
	1)
{
	this->plugin = plugin;
}

void OverlayMode::create_objects()
{
	for(int i = 0; i < TRANSFER_TYPES; i++)
		add_item(new BC_MenuItem(_(OverlayFrame::transfer_name(i))));
}

int OverlayMode::handle_event()
{
	char *text = get_text();

	plugin->config.mode = OverlayFrame::transfer_mode(text);

	plugin->send_configure_change();
	return 1;
}


OverlayDirection::OverlayDirection(Overlay *plugin, int x, int y)
: BC_CheckBox(x, y, plugin->config.direction, _("Bottom-top"))
{
	this->plugin = plugin;
}

int OverlayDirection::handle_event()
{
	plugin->config.direction = get_value();
	plugin->send_configure_change();
	return 1;
}

PLUGIN_THREAD_METHODS

REGISTER_PLUGIN


Overlay::Overlay(PluginServer *server)
 : PluginVClient(server)
{
	overlayer = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

Overlay::~Overlay()
{
	delete overlayer;
	PLUGIN_DESTRUCTOR_MACRO
}

void Overlay::reset_plugin()
{
	if(overlayer)
	{
		delete overlayer;
		overlayer = 0;
	}
}

PLUGIN_CLASS_METHODS

void Overlay::process_tmpframes(VFrame **frame)
{
	VFrame *output;
	int cmodel = frame[0]->get_color_model();
	int total_buffers = get_total_buffers();

	switch(cmodel)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(cmodel);
		return;
	}

	if(load_configuration())
		update_gui();

	if(!overlayer)
		overlayer = new OverlayFrame(get_project_smp());

// Direct copy the first layer
	output = frame[0];

	if(total_buffers < 2)
		return;

	if(config.direction)
	{
		if(total_buffers == 2 && frame[1])
		{
			// do not modify frame[1]
			VFrame *tmp = clone_vframe(output);
			tmp->copy_from(frame[1], 1);
			overlayer->overlay(tmp,
				output,
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
			frame[0] = tmp;
			release_vframe(output);
		}
		else
		for(int i = total_buffers - 1; i > 0; i--)
		{
			if(!frame[i])
				continue;
			overlayer->overlay(output,
				frame[i],
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
	}
	else
	for(int i = 1; i < total_buffers; i++)
	{
		if(!frame[i])
			continue;
		overlayer->overlay(output,
			frame[i],
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
}

void Overlay::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	static const char *get_pixels_frag = 
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

	static const char *put_pixels_frag = 
		"	result_color.rgb += chroma_offset;\n"
		"	result_color.rgb = mix(dst_color.rgb, result_color.rgb, src_color.a);\n"
		"	result_color.a = max(src_color.a, dst_color.a);\n"
		"	gl_FragColor = result_color;\n"
		"}\n";

	static const char *blend_add_frag = 
		"	result_color.rgb = dst_color.rgb + src_color.rgb;\n";

	static const char *blend_max_frag = 
		"	result_color.r = max(abs(dst_color.r, src_color.r);\n"
		"	result_color.g = max(abs(dst_color.g, src_color.g);\n"
		"	result_color.b = max(abs(dst_color.b, src_color.b);\n";

	static const char *blend_subtract_frag = 
		"	result_color.rgb = dst_color.rgb - src_color.rgb;\n";


	static const char *blend_multiply_frag = 
		"	result_color.rgb = dst_color.rgb * src_color.rgb;\n";

	static const char *blend_divide_frag = 
		"	result_color.rgb = dst_color.rgb / src_color.rgb;\n"
		"	if(src_color.r == 0.0) result_color.r = 1.0;\n"
		"	if(src_color.g == 0.0) result_color.g = 1.0;\n"
		"	if(src_color.b == 0.0) result_color.b = 1.0;\n";

	VFrame *src = temp;
	VFrame *dst = get_output(output_layer);

	dst->enable_opengl();
	dst->init_screen();

	const char *shader_stack[] = { 0, 0, 0 };
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
		if(ColorModels::is_yuv(dst->get_color_model()))
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
	*/
#endif
}

int Overlay::load_configuration()
{
	KeyFrame *prev_keyframe;

	if(prev_keyframe = get_prev_keyframe(source_pts))
		read_data(prev_keyframe);
	return 1;
}

void Overlay::load_defaults()
{
	defaults = load_defaults_file("overlay.rc");

	config.mode = defaults->get("MODE", config.mode);
	config.direction = defaults->get("DIRECTION", config.direction);
}

void Overlay::save_defaults()
{
	defaults->update("MODE", config.mode);
	defaults->update("DIRECTION", config.direction);
	defaults->delete_key("OUTPUT_LAYER");
	defaults->save();
}

void Overlay::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("OVERLAY");
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("DIRECTION", config.direction);
	output.append_tag();
	output.tag.set_title("/OVERLAY");
	output.append_tag();
	keyframe->set_data(output.string);
}

void Overlay::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("OVERLAY"))
		{
			config.mode = input.tag.get_property("MODE", config.mode);
			config.direction = input.tag.get_property("DIRECTION", config.direction);
		}
	}
}
