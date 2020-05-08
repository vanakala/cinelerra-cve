
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
}
