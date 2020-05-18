// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "dissolve.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"

#include <string.h>

REGISTER_PLUGIN

DissolveMain::DissolveMain(PluginServer *server)
 : PluginVClient(server)
{
	overlayer = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

DissolveMain::~DissolveMain()
{
	delete overlayer;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void DissolveMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	int w = outgoing->get_w();
	int h = outgoing->get_h();

	if(total_len_pts < EPSILON)
		return;

	fade = source_pts / total_len_pts;

// Use hardware
	if(get_use_opengl())
	{
		run_opengl();
		return;
	}

// Use software
	if(!overlayer)
		overlayer = new OverlayFrame(get_project_smp() + 1);

// There is a problem when dissolving from a big picture to a small picture.
// In order to make it dissolve correctly, we have to manually decrese alpha of big picture.
	switch (outgoing->get_color_model())
	{
	case BC_RGBA8888:
	case BC_YUVA8888:
		for(int i = 0; i < h; i++)
		{
			uint8_t* alpha_chan = (uint8_t *)outgoing->get_row_ptr(i) + 3;

			for(int j = 0; j < w; j++)
			{
				*alpha_chan = (uint8_t)(*alpha_chan * (1 - fade));
				alpha_chan += 4;
			}
		}
		break;

	case BC_YUVA16161616:
	case BC_RGBA16161616:
		for(int i = 0; i < h; i++)
		{
			uint16_t* alpha_chan = (uint16_t*)outgoing->get_row_ptr(i) + 3;

			for(int j = 0; j < w; j++)
			{
				*alpha_chan = (uint16_t)(*alpha_chan * (1 - fade));
				alpha_chan += 8;
			}
		}
		break;

	case BC_AYUV16161616:
		for(int i = 0; i < h; i++)
		{
			uint16_t* alpha_chan = (uint16_t*)outgoing->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				*alpha_chan = (uint16_t)(*alpha_chan * (1-fade));
				alpha_chan += 4;
			}
		}
		break;

	case BC_RGBA_FLOAT:
		for(int i = 0; i < h; i++)
		{
			float* alpha_chan = (float*)outgoing->get_row_ptr(i) + 3;

			for(int j = 0; j < w; j++)
			{
				*alpha_chan = *alpha_chan * (1 - fade);
				alpha_chan += sizeof(float);
			}
		}
		break;
	default:
		break;
	}

	overlayer->overlay(outgoing, 
		incoming, 
		0, 
		0, 
		incoming->get_w(),
		incoming->get_h(),
		0,
		0,
		incoming->get_w(),
		incoming->get_h(),
		fade,
		TRANSFER_NORMAL,
		NEAREST_NEIGHBOR);
}

void DissolveMain::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
// Read images from RAM
	get_input()->to_texture();
	get_output()->to_texture();

// Create output pbuffer
	get_output()->enable_opengl();

	VFrame::init_screen(get_output()->get_w(), get_output()->get_h());

// Enable output texture
	get_output()->bind_texture(0);
// Draw output texture
	glDisable(GL_BLEND);
	glColor4f(1, 1, 1, 1);
	get_output()->draw_texture();

// Draw input texture
	get_input()->bind_texture(0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1, 1, 1, fade);
	get_input()->draw_texture();

	glDisable(GL_BLEND);
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}
