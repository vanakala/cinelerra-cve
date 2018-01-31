
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

#include "dissolve.h"
#include "edl.inc"
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
	fade = source_pts / total_len_pts;

// Use hardware
	if(get_use_opengl())
	{
		run_opengl();
		return;
	}

// Use software
	if(!overlayer) overlayer = new OverlayFrame(get_project_smp() + 1);


// There is a problem when dissolving from a big picture to a small picture.
// In order to make it dissolve correctly, we have to manually decrese alpha of big picture.
	switch (outgoing->get_color_model())
	{
	case BC_RGBA8888:
	case BC_YUVA8888:
		{
			uint8_t** data_rows = (uint8_t **)outgoing->get_rows();
			int w = outgoing->get_w();
			int h = outgoing->get_h(); 
			for(int i = 0; i < h; i++) 
			{ 
				uint8_t* alpha_chan = data_rows[i] + 3; 
				for(int j = 0; j < w; j++) 
				{
					*alpha_chan = (uint8_t) (*alpha_chan * (1-fade));
					alpha_chan+=4;
				} 
			}
			break;
		}
	case BC_YUVA16161616:
		{
			uint16_t** data_rows = (uint16_t **)outgoing->get_rows();
			int w = outgoing->get_w();
			int h = outgoing->get_h(); 
			for(int i = 0; i < h; i++) 
			{ 
				uint16_t* alpha_chan = data_rows[i] + 3; // 3 since this is uint16_t
				for(int j = 0; j < w; j++) 
				{
					*alpha_chan = (uint16_t)(*alpha_chan * (1-fade));
					alpha_chan += 8;
				} 
			}
			break;
		}
	case BC_RGBA_FLOAT:
		{
			float** data_rows = (float **)outgoing->get_rows();
			int w = outgoing->get_w();
			int h = outgoing->get_h(); 
			for(int i = 0; i < h; i++) 
			{ 
				float* alpha_chan = data_rows[i] + 3; // 3 since this is floats 
				for(int j = 0; j < w; j++) 
				{
					*alpha_chan = *alpha_chan * (1-fade);
					alpha_chan += sizeof(float);
				} 
			}
			break;
		}
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
