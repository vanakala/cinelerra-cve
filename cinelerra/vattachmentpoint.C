#include "bcsignals.h"
#include "clip.h"
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "plugin.h"
#include "pluginserver.h"
#include "renderengine.h"
#include "transportque.h"
#include "vattachmentpoint.h"
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
			delete buffer_vector[i];
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
			buffer_vector[i] = new VFrame(0,
				width,
				height,
				colormodel,
				-1);
		}
	}
}

int VAttachmentPoint::get_buffer_size()
{
	return 1;
}

void VAttachmentPoint::render(VFrame *output, 
	int buffer_number,
	long start_position,
	double frame_rate)
{
	if(!plugin_server || !plugin->on) return;

	if(plugin_server->multichannel)
	{
// Test against previous parameters for reuse of previous data
		if(is_processed &&
			this->start_position == start_position &&
			EQUIV(this->frame_rate, frame_rate))
		{
			output->copy_from(buffer_vector[buffer_number]);
			return;
		}

		is_processed = 1;
		this->start_position = start_position;
		this->frame_rate = frame_rate;

// Allocate buffer vector
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
				output_temp[i] = buffer_vector[i];
		}

// Process plugin
		plugin_servers.values[0]->process_buffer(output_temp,
			start_position,
			frame_rate,
			(int64_t)Units::round(plugin->length * 
				frame_rate / 
				renderengine->edl->session->frame_rate),
			renderengine->command->get_direction());

		delete [] output_temp;
	}
	else
// process single track
	{
//printf("VAttachmentPoint::render 1\n");
		VFrame *output_temp[1];
		output_temp[0] = output;
//printf("VAttachmentPoint::render 1 %s\n", plugin_server->title);
		plugin_servers.values[buffer_number]->process_buffer(output_temp,
			start_position,
			frame_rate,
			(int64_t)Units::round(plugin->length * 
				frame_rate / 
				renderengine->edl->session->frame_rate),
			renderengine->command->get_direction());
//printf("VAttachmentPoint::render 100\n");
	}
}


