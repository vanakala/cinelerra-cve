
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

#include "bcsignals.h"
#include "bcresources.h"
#include "clip.h"
#include "datatype.h"
#include "plugin.h"
#include "pluginserver.h"
#include "renderengine.h"
#include "tmpframecache.h"
#include "vattachmentpoint.h"
#include "vdevicex11.h"
#include "videodevice.h"
#include "vframe.h"

VAttachmentPoint::VAttachmentPoint(RenderEngine *renderengine, Plugin *plugin)
: AttachmentPoint(renderengine, plugin, TRACK_VIDEO)
{
	buffer_vector = 0;
}

VAttachmentPoint::~VAttachmentPoint()
{
	delete_buffer_vector();
}

void VAttachmentPoint::delete_buffer_vector()
{
	if(buffer_vector)
	{
		for(int i = 0; i < virtual_plugins.total; i++)
			BC_Resources::tmpframes.release_frame(buffer_vector[i]);
		delete [] buffer_vector;
	}
	buffer_vector = 0;
}

void VAttachmentPoint::new_buffer_vector(int width, int height, int colormodel)
{
	if(buffer_vector &&
		(width != buffer_vector[0]->get_w() ||
		height != buffer_vector[0]->get_h() ||
		colormodel != buffer_vector[0]->get_color_model()))
	{
		delete_buffer_vector();
	}

	if(!buffer_vector)
	{
		buffer_vector = new VFrame*[virtual_plugins.total];
		for(int i = 0; i < virtual_plugins.total; i++)
		{
			buffer_vector[i] = BC_Resources::tmpframes.get_tmpframe(
				width,
				height,
				colormodel);
		}
	}
}

void VAttachmentPoint::render(VFrame **output,
	int buffer_number)
{
	VFrame *frame;
	if(!plugin_server || !plugin->on) return;

	if(plugin_server->multichannel)
	{
// Allocate buffer vector for subsequent render calls
		new_buffer_vector((*output)->get_w(),
			(*output)->get_h(),
			(*output)->get_color_model());

		frame = *output;
		for(int i = 0; i < virtual_plugins.total; i++)
			buffer_vector[i]->copy_pts(frame);

		*output = buffer_vector[buffer_number];
		buffer_vector[buffer_number] = frame;
// Process plugin
		plugin_servers.values[0]->process_buffer(buffer_vector,
			plugin->get_length());

		frame = buffer_vector[buffer_number];
		buffer_vector[buffer_number] = *output;
		*output = frame;
	}
	else
// process single track
	{
		VFrame *output_temp[1];

		output_temp[0] = *output;
		*output = BC_Resources::tmpframes.get_tmpframe(
			output_temp[0]->get_w(),
			output_temp[0]->get_h(),
			output_temp[0]->get_color_model());
		(*output)->copy_pts(output_temp[0]);
		plugin_servers.values[buffer_number]->process_buffer(output_temp,
			plugin->get_length());
		BC_Resources::tmpframes.release_frame(*output);
		*output = output_temp[0];
	}
}
