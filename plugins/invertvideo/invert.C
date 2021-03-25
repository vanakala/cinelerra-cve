// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bchash.h"
#include "bctoggle.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "invert.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>


REGISTER_PLUGIN

const char *InvertVideoWindow::inv_chn_rgba[] =
{
    N_("Invert red"),
    N_("Invert green"),
    N_("Invert blue"),
    N_("Invert alpha")
};

const char *InvertVideoWindow::inv_chn_ayuv[] =
{
    N_("Invert alpha"),
    N_("Invert Y"),
    N_("Invert U"),
    N_("Invert V")
};


InvertVideoConfig::InvertVideoConfig()
{
	chan0 = 1;
	chan1 = 1;
	chan2 = 1;
	chan3 = 1;
}

void InvertVideoConfig::copy_from(InvertVideoConfig &src)
{
	chan0 = src.chan0;
	chan1 = src.chan1;
	chan2 = src.chan2;
	chan3 = src.chan3;
}

int InvertVideoConfig::equivalent(InvertVideoConfig &src)
{
	return chan0 == src.chan0 &&
		chan1 == src.chan1 &&
		chan2 == src.chan2 &&
		chan3 == src.chan3;
}

void InvertVideoConfig::interpolate(InvertVideoConfig &prev, 
	InvertVideoConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	chan0 = prev.chan0;
	chan1 = prev.chan1;
	chan2 = prev.chan2;
	chan3 = prev.chan3;
}


InvertVideoEnable::InvertVideoEnable(InvertVideoEffect *plugin,
	int *output, int x, int y, const char *text)
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
 : PluginWindow(plugin,
	x, 
	y, 
	260, 
	170)
{
	BC_WindowBase *win;
	int title_h;
	const char **chname;
	int cmodel = plugin->get_project_color_model();

	x = y = 10;

	if(cmodel == BC_AYUV16161616)
		chname = inv_chn_ayuv;
	else
		chname = inv_chn_rgba;

	add_subwindow(win = print_title(x, y, "%s: %s", _(plugin->plugin_title()),
		ColorModels::name(cmodel)));
	title_h = win->get_h() + 8;
	y += title_h;

	add_subwindow(chan0 = new InvertVideoEnable(plugin,
		&plugin->config.chan0, x, y, _(chname[0])));
	y += chan0->get_h() + 8;
	add_subwindow(chan1 = new InvertVideoEnable(plugin,
		&plugin->config.chan1, x, y, _(chname[1])));
	y += chan1->get_h() + 8;
	add_subwindow(chan2 = new InvertVideoEnable(plugin,
		&plugin->config.chan2, x, y, _(chname[2])));
	y += chan2->get_h() + 8;
	add_subwindow(chan3 = new InvertVideoEnable(plugin,
		&plugin->config.chan3, x, y, _(chname[3])));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void InvertVideoWindow::update()
{
	chan0->update(plugin->config.chan0);
	chan1->update(plugin->config.chan1);
	chan2->update(plugin->config.chan2);
	chan3->update(plugin->config.chan3);
}

PLUGIN_THREAD_METHODS


InvertVideoEffect::InvertVideoEffect(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

InvertVideoEffect::~InvertVideoEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void InvertVideoEffect::load_defaults()
{
	defaults = load_defaults_file("invertvideo.rc");
	// Compatibility
	config.chan0 = defaults->get("R", config.chan0);
	config.chan1 = defaults->get("G", config.chan1);
	config.chan2 = defaults->get("B", config.chan2);
	config.chan3 = defaults->get("A", config.chan3);

	config.chan0 = defaults->get("CHAN0", config.chan0);
	config.chan1 = defaults->get("CHAN1", config.chan1);
	config.chan2 = defaults->get("CHAN2", config.chan2);
	config.chan3 = defaults->get("CHAN3", config.chan3);
}

void InvertVideoEffect::save_defaults()
{
	defaults->delete_key("R");
	defaults->delete_key("G");
	defaults->delete_key("B");
	defaults->delete_key("A");
	defaults->update("CHAN0", config.chan0);
	defaults->update("CHAN1", config.chan1);
	defaults->update("CHAN2", config.chan2);
	defaults->update("CHAN3", config.chan3);
	defaults->save();
}

void InvertVideoEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("INVERTVIDEO");
	output.tag.set_property("CHAN0", config.chan0);
	output.tag.set_property("CHAN1", config.chan1);
	output.tag.set_property("CHAN2", config.chan2);
	output.tag.set_property("CHAN3", config.chan3);
	output.append_tag();
	output.tag.set_title("/INVERTVIDEO");
	output.append_tag();
	keyframe->set_data(output.string);
}

void InvertVideoEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("INVERTVIDEO"))
		{
			config.chan0 = input.tag.get_property("CHAN0", config.chan0);
			config.chan1 = input.tag.get_property("CHAN1", config.chan1);
			config.chan2 = input.tag.get_property("CHAN2", config.chan2);
			config.chan3 = input.tag.get_property("CHAN3", config.chan3);
		}
	}
}

VFrame  *InvertVideoEffect::process_tmpframe(VFrame *frame)
{
	int cmodel = frame->get_color_model();

	if(load_configuration())
		update_gui();

	if(config.chan0 || config.chan1 || config.chan2 || config.chan3)
	{
		int w = frame->get_w();

		switch(frame->get_color_model())
		{
		case BC_RGBA16161616:
		case BC_AYUV16161616:
			for(int i = 0; i < frame->get_h(); i++)
			{
				uint16_t *in_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					if(config.chan0)
						in_row[0] = 0xffff - in_row[0];
					if(config.chan1)
						in_row[1] = 0xffff - in_row[1];
					if(config.chan2)
						in_row[2] = 0xffff - in_row[2];
					if(config.chan3)
						in_row[3] = 0xffff - in_row[3];
					in_row += 4;
				}
			}
			if(cmodel == BC_RGBA16161616 && config.chan3 ||
					cmodel == BC_AYUV16161616 && config.chan0)
				frame->set_transparent();
			break;
		default:
			unsupported(cmodel);
			break;
		}
	}
	return frame;
}

void InvertVideoEffect::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	static const char *invert_frag = 
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
	*/
#endif
}
