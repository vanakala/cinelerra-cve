// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
	ptstime start_pts = get_start();
	ptstime input_pts;

	input_pts = end_pts - curpts + start_pts - frame->get_duration();

	if(input_pts >= start_pts && input_pts < end_pts)
	{
		frame->set_pts(input_pts);
		frame = get_frame(frame);
		frame->set_pts(curpts);
	}
	return frame;
}

