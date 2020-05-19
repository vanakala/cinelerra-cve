// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "crossfade.h"
#include "picon_png.h"

REGISTER_PLUGIN


CrossfadeMain::CrossfadeMain(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

CrossfadeMain::~CrossfadeMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void CrossfadeMain::process_realtime(AFrame *out, AFrame *in)
{
	if(total_len_pts < EPSILON)
		return;

	double intercept = source_pts / total_len_pts;
	double slope = (double)1 / round(total_len_pts * out->get_samplerate());
	double *incoming = in->buffer;
	double *outgoing = out->buffer;
	int size = out->get_length();

	for(int i = 0; i < size; i++)
	{
		incoming[i] = outgoing[i] * ((double)1 - (slope * i + intercept)) + 
			incoming[i] * (slope * i + intercept);
	}
}
