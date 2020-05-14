// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "reverseaudio.h"
#include "picon_png.h"

REGISTER_PLUGIN


ReverseAudio::ReverseAudio(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

ReverseAudio::~ReverseAudio()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

AFrame *ReverseAudio::process_tmpframe(AFrame *aframe)
{
	ptstime current_pts = aframe->get_pts();
	ptstime rq_pts = get_end() - (aframe->get_pts() - get_start()) -
		aframe->get_duration();

	aframe->set_fill_request(rq_pts, aframe->get_duration());
	aframe = get_frame(aframe);

	double *buffer = aframe->buffer;

	for(int start = 0, end = aframe->get_length() - 1;
		end > start; start++, end--)
	{
		double temp = buffer[start];
		buffer[start] = buffer[end];
		buffer[end] = temp;
	}
	aframe->set_pts(current_pts);
	return aframe;
}

