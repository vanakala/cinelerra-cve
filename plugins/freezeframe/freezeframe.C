// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
			first_frame = get_frame(first_frame);
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
