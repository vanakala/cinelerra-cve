
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

#include "bchash.h"
#include "filexml.h"
#include "freezeframe.h"
#include "language.h"
#include "picon_png.h"

#include <string.h>

REGISTER_PLUGIN

FreezeFrameMain::FreezeFrameMain(PluginServer *server)
 : PluginVClient(server)
{
	first_frame = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

FreezeFrameMain::~FreezeFrameMain()
{
	release_vframe(first_frame);
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS


VFrame *FreezeFrameMain::process_tmpframe(VFrame *frame)
{
	if(!first_frame)
	{
		ptstime plugin_start = get_start();
		first_frame = clone_vframe(frame);
		if(PTSEQU(frame->get_pts(), plugin_start))
			first_frame->copy_from(frame, 1);
		else
		{
			first_frame->set_pts(plugin_start);
			get_frame(first_frame);
		}
	}
	frame->copy_from(first_frame, 0);
	return frame;
}

void FreezeFrameMain::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	get_output()->enable_opengl();
	get_output()->init_screen();
	first_frame->to_texture();
	first_frame->bind_texture(0);
	first_frame->draw_texture();
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}
