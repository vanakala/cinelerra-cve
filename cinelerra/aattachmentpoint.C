
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

#include "aattachmentpoint.h"
#include "bcsignals.h"
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "playbackconfig.h"
#include "plugin.h"
#include "pluginserver.h"
#include "renderengine.h"

AAttachmentPoint::AAttachmentPoint(RenderEngine *renderengine, Plugin *plugin)
: AttachmentPoint(renderengine, plugin, TRACK_AUDIO)
{
	buffer_vector = 0;
	buffer_allocation = 0;
}

AAttachmentPoint::~AAttachmentPoint()
{
	delete_buffer_vector();
}

void AAttachmentPoint::delete_buffer_vector()
{
	if(buffer_vector)
	{
		for(int i = 0; i < virtual_plugins.total; i++)
			delete buffer_vector[i];
		delete [] buffer_vector;
	}
	buffer_vector = 0;
	buffer_allocation = 0;
}

void AAttachmentPoint::new_buffer_vector(int total, int size)
{
	if(buffer_vector && size > buffer_allocation)
		delete_buffer_vector();

	if(!buffer_vector)
	{
		buffer_allocation = size;
		buffer_vector = new AFrame*[virtual_plugins.total];
		for(int i = 0; i < virtual_plugins.total; i++)
		{
			buffer_vector[i] = new AFrame(buffer_allocation);
		}
	}
}

void AAttachmentPoint::render(AFrame *output, int buffer_number)
{
	if(!plugin_server || !plugin->on) return;

	if(plugin_server->multichannel)
	{
// Test against previous parameters for reuse of previous data
		if(is_processed &&
			PTSEQU(start_postime, output->pts) && 
			len == output->source_length && 
			sample_rate == output->samplerate)
		{
			output->copy(buffer_vector[buffer_number]);
			return;
		}

// Update status
		start_postime = output->pts;
		len = output->source_length;
		sample_rate = output->samplerate;
		is_processed = 1;

// Allocate buffer vector
		new_buffer_vector(virtual_plugins.total, len);

// Create temporary buffer vector with output argument substituted in
		AFrame *output_temp[virtual_plugins.total];
		for(int i = 0; i < virtual_plugins.total; i++)
		{
			if(i == buffer_number)
				output_temp[i] = output;
			else
			{
				output_temp[i] = buffer_vector[i];
				output_temp[i]->copy_pts(output);
				output_temp[i]->channel = i;
			}
		}

// Process plugin
		plugin_servers.values[0]->process_buffer(output_temp,
			plugin->length());

	}
	else
	{
// Process plugin
		AFrame *output_temp[1];
		output_temp[0] = output;
		plugin_servers.values[buffer_number]->process_buffer(output_temp,
			plugin->length());
	}
}

