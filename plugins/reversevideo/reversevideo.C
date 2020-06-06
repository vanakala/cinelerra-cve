
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

#include "picon_png.h"
#include "reversevideo.h"

REGISTER_PLUGIN

ReverseVideo::ReverseVideo(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

ReverseVideo::~ReverseVideo()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

VFrame *ReverseVideo::process_tmpframe(VFrame *frame)
{
	ptstime curpts = frame->get_pts();
	ptstime end_pts = get_end();
	ptstime input_pts;

	input_pts = end_pts - curpts + get_start();

	if(input_pts >= 0 && input_pts < end_pts)
	{
		frame->set_pts(input_pts);
		get_frame(frame);
		frame->set_pts(curpts);
	}
	return frame;
}

