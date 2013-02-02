
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
#include "clip.h"
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "plugin.h"
#include "pluginserver.h"
#include "renderengine.h"
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
	if(!this) printf("VAttachmentPoint::delete_buffer_vector NULL\n");
	if(buffer_vector)
	{
		for(int i = 0; i < virtual_plugins.total; i++)
			delete buffer_vector[i];
		delete [] buffer_vector;
	}
	buffer_vector = 0;
}

void VAttachmentPoint::new_buffer_vector(int width, int height, int colormodel)
{
	if(!this) printf("VAttachmentPoint::new_buffer_vector NULL\n");
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
			buffer_vector[i] = new VFrame(0,
				width,
				height,
				colormodel,
				-1);
		}
	}
}

void VAttachmentPoint::render(VFrame *output, 
	int buffer_number,
	int debug_render,
	int use_opengl)
{
	if(!plugin_server || !plugin->on) return;
	if(debug_render)
		printf("    VAttachmentPoint::render \"%s\" multi=%d opengl=%d\n", 
			plugin_server->title,
			plugin_server->multichannel,
			use_opengl);

	if(plugin_server->multichannel)
	{
// Test against previous parameters for reuse of previous data
		if(is_processed &&
			PTSEQU(this->start_postime, output->get_pts()) &&
			EQUIV(this->duration, output->get_duration()))
		{
// Need to copy PBuffer if OpenGL, regardless of use_opengl
			if(buffer_vector[buffer_number]->get_opengl_state() == VFrame::RAM)
			{
				output->copy_from(buffer_vector[buffer_number]);
				output->set_opengl_state(VFrame::RAM);
			}
			else
			if(renderengine && renderengine->video)
			{
// Need to copy PBuffer to texture
				VDeviceX11 *x11_device = (VDeviceX11*)renderengine->video->get_output_base();
				x11_device->copy_frame(output, buffer_vector[buffer_number]);
			}
			return;
		}

		is_processed = 1;
		this->start_postime = output->get_pts();
		this->duration = output->get_duration();

// Allocate buffer vector for subsequent render calls
		new_buffer_vector(output->get_w(), 
			output->get_h(), 
			output->get_color_model());

// Create temporary vector with output argument substituted in
		VFrame **output_temp = new VFrame*[virtual_plugins.total];
		for(int i = 0; i < virtual_plugins.total; i++)
		{
			if(i == buffer_number)
				output_temp[i] = output;
			else
			{
				output_temp[i] = buffer_vector[i];
				output_temp[i]->copy_pts(output);
			}
		}

// Process plugin
		if(renderengine)
			plugin_servers.values[0]->set_use_opengl(use_opengl,
				renderengine->video);
		plugin_servers.values[0]->process_buffer(output_temp,
			plugin->length());

		delete [] output_temp;
	}
	else
// process single track
	{
		VFrame *output_temp[1];
		output_temp[0] = output;
		if(renderengine)
			plugin_servers.values[buffer_number]->set_use_opengl(use_opengl,
				renderengine->video);
		plugin_servers.values[buffer_number]->process_buffer(output_temp,
			plugin->length());
	}
}
