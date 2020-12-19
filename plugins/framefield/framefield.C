// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "bchash.h"
#include "filexml.h"
#include "framefield.h"
#include "keyframe.h"
#include "picon_png.h"
#include "vframe.h"

#include <string.h>
#include <stdint.h>

REGISTER_PLUGIN

FrameFieldConfig::FrameFieldConfig()
{
	field_dominance = TOP_FIELD_FIRST;
}

int FrameFieldConfig::equivalent(FrameFieldConfig &src)
{
	return src.field_dominance == field_dominance;
}

FrameFieldWindow::FrameFieldWindow(FrameField *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	210, 
	160)
{
	x = y = 10;
	add_subwindow(top = new FrameFieldTop(plugin, this, x, y));
	y += top->get_h() + 5;
	add_subwindow(bottom = new FrameFieldBottom(plugin, this, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void FrameFieldWindow::update()
{
	top->update(plugin->config.field_dominance == TOP_FIELD_FIRST);
	bottom->update(plugin->config.field_dominance == BOTTOM_FIELD_FIRST);
}


FrameFieldTop::FrameFieldTop(FrameField *plugin, 
	FrameFieldWindow *gui, 
	int x, 
	int y)
 : BC_Radial(x, 
	y, 
	plugin->config.field_dominance == TOP_FIELD_FIRST,
	_("Top field first"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FrameFieldTop::handle_event()
{
	plugin->config.field_dominance = TOP_FIELD_FIRST;
	gui->bottom->update(0);
	plugin->send_configure_change();
	return 1;
}


FrameFieldBottom::FrameFieldBottom(FrameField *plugin, 
	FrameFieldWindow *gui, 
	int x, 
	int y)
 : BC_Radial(x, 
	y, 
	plugin->config.field_dominance == BOTTOM_FIELD_FIRST,
	_("Bottom field first"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FrameFieldBottom::handle_event()
{
	plugin->config.field_dominance = BOTTOM_FIELD_FIRST;
	gui->top->update(0);
	plugin->send_configure_change();
	return 1;
}

PLUGIN_THREAD_METHODS

FrameField::FrameField(PluginServer *server)
 : PluginVClient(server)
{
	field_number = 0;
	current_frame_pts = -1;
	current_frame_duration = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

FrameField::~FrameField()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

VFrame *FrameField::process_tmpframe(VFrame *src_frame)
{
	VFrame *frame;
	int color_model = src_frame->get_color_model();

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return src_frame;
	}

	if(load_configuration())
		update_gui();

// Read into temporary for software
	frame = clone_vframe(src_frame);
	frame->copy_pts(src_frame);

// Import source frame at half frame rate
	if(!(current_frame_pts <= source_pts && 
		source_pts < current_frame_pts + current_frame_duration - EPSILON))
	{
// Get frame
		current_frame_pts = src_frame->get_pts();
		current_frame_duration = src_frame->get_duration();
	}

	field_number = (source_pts < current_frame_pts + current_frame_duration / 2.2) ?
		0 : 1;

	if(field_number)
		frame->set_pts(current_frame_pts + current_frame_duration / 2);
	else
		frame->set_pts(current_frame_pts);
	frame->set_duration(current_frame_duration / 2);

	int row_size = src_frame->get_bytes_per_line();
	int start_row;

// Even field
	if(field_number == 0)
	{
		if(config.field_dominance == TOP_FIELD_FIRST) 
		{
			for(int i = 0; i < frame->get_h() - 1; i += 2)
			{
// Copy even lines of src to both lines of output
				memcpy(frame->get_row_ptr(i),
					src_frame->get_row_ptr(i),
					row_size);
			}

// Average empty rows
			average_rows(0, frame);
		}
		else
		{
			for(int i = 0; i < frame->get_h() - 1; i += 2)
			{
// Copy odd lines of current to both lines of output with shift up.
				memcpy(frame->get_row_ptr(i + 1),
					src_frame->get_row_ptr(i + 1),
					row_size);
			}

// Average empty rows
			average_rows(1, frame);
		}
	}
	else
// Odd field
	{
		if(config.field_dominance == TOP_FIELD_FIRST)
		{
			for(int i = 0; i < frame->get_h() - 1; i += 2)
			{
// Copy odd lines of src to both lines of output
				memcpy(frame->get_row_ptr(i + 1),
					src_frame->get_row_ptr(i + 1),
					row_size);
			}

// Average empty rows
			average_rows(1, frame);
		}
		else
		{
			for(int i = 0; i < frame->get_h() - 1; i += 2)
			{
// Copy even lines of src to both lines of output.
				memcpy(frame->get_row_ptr(i),
					src_frame->get_row_ptr(i),
					row_size);
			}

// Average empty rows
			average_rows(0, frame);
		}
	}
	release_vframe(src_frame);
	return frame;
}

void FrameField::average_rows(int offset, VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();
	int row_size = 4 * w;

	for(int i = offset; i < h - 3; i += 2)
	{
		uint16_t *row1 = (uint16_t*)frame->get_row_ptr(i);
		uint16_t *row2 = (uint16_t*)frame->get_row_ptr(i + 1);
		uint16_t *row3 = (uint16_t*)frame->get_row_ptr(i + 2);
		for(int j = 0; j < row_size; j++)
		{
			int sum = *row1++ + *row3++;
			*row2++ = (sum / 2);
		}
	}
}

int FrameField::load_configuration()
{
	KeyFrame *prev_keyframe;
	FrameFieldConfig old_config = config;

	prev_keyframe = get_prev_keyframe(source_pts);

	if(!prev_keyframe)
		return 0;

	read_data(prev_keyframe);

	return !old_config.equivalent(config);
}

void FrameField::load_defaults()
{
	defaults = load_defaults_file("framefield.rc");

	config.field_dominance = defaults->get("DOMINANCE", config.field_dominance);
}

void FrameField::save_defaults()
{
	defaults->update("DOMINANCE", config.field_dominance);
	defaults->save();
}

void FrameField::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("FRAME_FIELD");
	output.tag.set_property("DOMINANCE", config.field_dominance);
	output.append_tag();
	output.tag.set_title("/FRAME_FIELD");
	output.append_tag();
	keyframe->set_data(output.string);
}

void FrameField::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("FRAME_FIELD"))
		{
			config.field_dominance = input.tag.get_property("DOMINANCE", config.field_dominance);
		}
	}
}

void FrameField::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	static const char *field_frag = 
		"uniform sampler2D tex;\n"
		"uniform float double_line_h;\n"
		"uniform float y_offset;\n"
		"void main()\n"
		"{\n"
		"	vec2 coord = gl_TexCoord[0].st;\n"
// Number of double lines + fraction of current double line
		"	float half_y = (coord.y - y_offset) / double_line_h;\n"
// Lines comprising current double line
		"	float line1 = floor(half_y) * double_line_h + y_offset;\n"
		"	float line2 = line1 + double_line_h;\n"
// Distance from line1 to line2
		"	float frac = fract(half_y);\n"
		"	gl_FragColor =  mix(\n"
		"		texture2D(tex, vec2(coord.x, line1)), \n"
		"		texture2D(tex, vec2(coord.x, line2)), \n"
		"		frac);\n"
		"}\n";

	static const char *_601_to_rgb_frag = 
		"void main()\n"
		"{\n"
		"	gl_FragColor.rgb = gl_FragColor.rgb * vec3(1.1644, 1.1644, 1.1644) - vec3(0.0627, 0.0627, 0.0627);\n"
		"}\n";

	static const char *_601_to_yuv_frag = 
		"void main()\n"
		"{\n"
		"	gl_FragColor.r = gl_FragColor.r * 1.1644 - 0.0627;\n"
		"}\n";

	static const char *rgb_to_601_frag = 
		"void main()\n"
		"{\n"
		"	gl_FragColor.rgb = gl_FragColor.rgb * vec3(0.8588, 0.8588, 0.8588) + vec3(0.0627, 0.0627, 0.0627);\n"
		"}\n";

	static const char *yuv_to_601_frag = 
		"void main()\n"
		"{\n"
		"	gl_FragColor.r = gl_FragColor.r * 0.8588 + 0.0627;\n"
		"}\n";
	if(new_frame)
	{
		if(get_output()->get_opengl_state() != VFrame::SCREEN)
		{
// Copy new frame to temporary texture
			get_output()->to_texture();

// Draw it only to copy it to the temporary.
			get_output()->enable_opengl();
			VFrame::init_screen(get_output()->get_w(), get_output()->get_h());
			get_output()->bind_texture(0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			get_output()->draw_texture();
		}

		get_output()->enable_opengl();
		VFrame::init_screen(get_output()->get_w(), get_output()->get_h());
		glActiveTexture(GL_TEXTURE0);
		BC_Texture::new_texture(&src_texture,
			get_output()->get_w(), 
			get_output()->get_h(), 
			get_output()->get_color_model());
		src_texture->bind(0);
		glCopyTexSubImage2D(GL_TEXTURE_2D,
			0,
			0,
			0,
			0,
			0,
			get_output()->get_w(),
			get_output()->get_h());
	}
	else
	{
		get_output()->enable_opengl();
	}

	unsigned int frag = 0;
	float y_offset = 0.0;
	if(field_number == 0)
	{
		if(config.field_dominance == BOTTOM_FIELD_FIRST)
			y_offset = 1.0;
	}
	else
	{
		if(config.field_dominance == TOP_FIELD_FIRST)
			y_offset = 1.0;
	}

	VFrame::init_screen(get_output()->get_w(), get_output()->get_h());
	glActiveTexture(GL_TEXTURE0);
	BC_Texture::new_texture(&src_texture,
		get_output()->get_w(), 
		get_output()->get_h(), 
		get_output()->get_color_model());

	const char *shaders[3] = { 0, 0, 0 };
	shaders[0] = field_frag;

	frag = VFrame::make_shader(0, shaders[0], shaders[1], shaders[2], 0);

	if(frag)
	{
		glUseProgram(frag);
		glUniform1i(glGetUniformLocation(frag, "tex"), 0);
		glUniform1f(glGetUniformLocation(frag, "double_line_h"), 
			2.0 / src_texture->get_texture_h());
		glUniform1f(glGetUniformLocation(frag, "y_offset"), 
			y_offset / src_texture->get_texture_h());
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	get_output()->draw_texture();

	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);

// Reset for other operations
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	*/
#endif
}


