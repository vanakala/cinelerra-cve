// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "overlayaudio.h"
#include "picon_png.h"

REGISTER_PLUGIN

OverlayAudio::OverlayAudio(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

OverlayAudio::~OverlayAudio()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void OverlayAudio::process_tmpframes(AFrame **aframes)
{
	int num = get_total_buffers();
	double scale = 1.0 / num;
// Direct copy the output track
	int size = aframes[0]->get_source_length();

	double *output_buffer = aframes[0]->buffer;

	for(int i = 1; i < num; i++)
	{
		double *input_buffer = aframes[i]->buffer;

		for(int j = 0; j < size; j++)
		{
			output_buffer[j] += input_buffer[j];
		}
	}

	for(int j = 0; j < size; j++)
		output_buffer[j] *= scale;
}
