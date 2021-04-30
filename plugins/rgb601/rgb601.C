// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "clip.h"
#include "colormodels.inc"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "rgb601.h"
#include "rgb601window.h"

#include <stdio.h>
#include <string.h>


REGISTER_PLUGIN


RGB601Config::RGB601Config()
{
	direction = 0;
}

RGB601Main::RGB601Main(PluginServer *server)
 : PluginVClient(server)
{
	forward_table = 0;
	reverse_table = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

RGB601Main::~RGB601Main()
{
	delete [] forward_table;
	delete [] reverse_table;
	PLUGIN_DESTRUCTOR_MACRO
}

void RGB601Main::reset_plugin()
{
	if(forward_table)
	{
		delete [] forward_table;
		forward_table = 0;
	}

	if(reverse_table)
	{
		delete [] reverse_table;
		reverse_table = 0;
	}
}

PLUGIN_CLASS_METHODS

void RGB601Main::load_defaults()
{
	defaults = load_defaults_file("rgb601.rc");

	config.direction = defaults->get("DIRECTION", config.direction);
}

void RGB601Main::save_defaults()
{
	defaults->update("DIRECTION", config.direction);
	defaults->save();
}

int RGB601Main::load_configuration()
{
	KeyFrame *prev_keyframe;

// Must also switch between interpolation between keyframes and using first keyframe
	if(prev_keyframe = get_prev_keyframe(source_pts))
		read_data(prev_keyframe);
	return 1;
}

void RGB601Main::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("RGB601");
	output.tag.set_property("DIRECTION", config.direction);
	output.append_tag();
	output.tag.set_title("/RGB601");
	output.append_tag();
	keyframe->set_data(output.string);
}

void RGB601Main::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;
	float new_threshold;

	while(!input.read_tag())
	{
		if(input.tag.title_is("RGB601"))
		{
			config.direction = input.tag.get_property("DIRECTION", config.direction);
		}
	}
}

void RGB601Main::create_table(VFrame *input_ptr)
{
	int forward_output;
	int reverse_output;

	if(config.direction == RGB601_FORW && !forward_table)
		forward_table = new int[0x10000];
	if(config.direction == RGB601_RVRS && !reverse_table)
		reverse_table = new int[0x10000];

	if(config.direction == RGB601_FORW)
	{
		for(int i = 0; i < 0x10000; i++)
		{
			forward_output = (int)((double)0.8588 * i + 0x10000 * 0.0627 + 0.5);
			forward_table[i] = CLIP(forward_output, 0,  0xffff);
		}
	}

	if(config.direction == RGB601_RVRS)
	{
		for(int i = 0; i < 0x10000; i++)
		{
			reverse_output = (int)((double)1.1644 * i - 0x10000 * 0.0627 + 0.5);
			reverse_table[i] = CLIP(reverse_output, 0, 0xffff);
		}
	}
}

void RGB601Main::process(VFrame *input_ptr)
{
	int w = input_ptr->get_w();
	int h = input_ptr->get_h();
	int cmodel = input_ptr->get_color_model();

	if(config.direction == RGB601_FORW)
	{
		switch(cmodel)
		{
		case BC_RGBA16161616:
			for(int i = 0; i < h; i++)
			{
				uint16_t *in_row = (uint16_t*)input_ptr->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					in_row[j * 4] = forward_table[in_row[j * 4]];
					in_row[j * 4 + 1] = forward_table[in_row[j * 4 + 1]];
					in_row[j * 4 + 2] = forward_table[in_row[j * 4 + 2]];
				}
			}
			break;
		case BC_AYUV16161616:
			for(int i = 0; i < h; i++)
			{
				uint16_t *in_row = (uint16_t*)input_ptr->get_row_ptr(i);

				for(int j = 0; j < w; j++)
					in_row[j * 4 + 1] = forward_table[in_row[j * 4 + 1]];
			}
			break;
		}
	}
	else
	if(config.direction == RGB601_RVRS)
	{
		switch(cmodel)
		{
		case BC_RGBA16161616:
			for(int i = 0; i < h; i++)
			{
				uint16_t *in_row = (uint16_t*)input_ptr->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					in_row[j * 4] = reverse_table[in_row[j * 4]];
					in_row[j * 4 + 1] = reverse_table[in_row[j * 4 + 1]];
					in_row[j * 4 + 2] = reverse_table[in_row[j * 4 + 2]];
				}
			}
			break;
		case BC_AYUV16161616:
			for(int i = 0; i < h; i++)
			{
				uint16_t *in_row = (uint16_t*)input_ptr->get_row_ptr(i);

				for(int j = 0; j < w; j++)
					in_row[j * 4 + 1] = reverse_table[in_row[j * 4 + 1]];
			}
			break;
		}
	}
}

VFrame *RGB601Main::process_tmpframe(VFrame *frame)
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

	if(load_configuration())
		update_gui();

	if(config.direction != RGB601_NONE)
	{
		create_table(frame);
		process(frame);
	}
	return frame;
}

void RGB601Main::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	static const char *yuv_fwd_frag = 
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
		"	gl_FragColor.r = gl_FragColor.r * 0.8588 + 0.0627;\n"
		"}\n";
	static const char *yuv_rev_frag = 
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
		"	gl_FragColor.r = gl_FragColor.r * 1.1644 - 0.0627;\n"
		"}\n";
	static const char *rgb_fwd_frag = 
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
		"	gl_FragColor.rgb = gl_FragColor.rgb * vec3(0.8588, 0.8588, 0.8588) + vec3(0.0627, 0.0627, 0.0627);\n"
		"}\n";
	static const char *rgb_rev_frag = 
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
		"	gl_FragColor.rgb = gl_FragColor.rgb * vec3(1.1644, 1.1644, 1.1644) - vec3(0.0627, 0.0627, 0.0627);\n"
		"}\n";

	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->bind_texture(0);

	unsigned int frag_shader = 0;
	switch(get_output()->get_color_model())
	{
	case BC_YUV888:
	case BC_YUVA8888:
		frag_shader = VFrame::make_shader(0,
			config.direction == 1 ? yuv_fwd_frag : yuv_rev_frag,
			0);
		break;

	default:
		frag_shader = VFrame::make_shader(0,
			config.direction == 1 ? rgb_fwd_frag : rgb_rev_frag,
			0);
		break;
	}

	if(frag_shader)
	{
		glUseProgram(frag_shader);
		glUniform1i(glGetUniformLocation(frag_shader, "tex"), 0);
	}
	VFrame::init_screen(get_output()->get_w(), get_output()->get_h());
	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}
