#include "plugin.h"
#include "pluginserver.h"
#include "vattachmentpoint.h"


VAttachmentPoint::VAttachmentPoint(RenderEngine *renderengine, Plugin *plugin)
: AttachmentPoint(renderengine, plugin)
{
	buffer_in = 0;
	buffer_out = 0;
}

VAttachmentPoint::~VAttachmentPoint()
{
}

void VAttachmentPoint::delete_buffer_vectors()
{
	if(buffer_in) delete [] buffer_in;
	if(buffer_out) delete [] buffer_out;
	buffer_in = 0;
	buffer_out = 0;
}

void VAttachmentPoint::new_buffer_vectors()
{
	buffer_in = new VFrame*[total_input_buffers];
	buffer_out = new VFrame*[total_input_buffers];
}

int VAttachmentPoint::get_buffer_size()
{
	return 1;
}

void VAttachmentPoint::render(VFrame *video_in, 
	VFrame *video_out, 
	long current_position)
{
//printf("VAttachmentPoint::render 1\n");
// Arm buffer vectors
	buffer_in[current_buffer] = video_in;
	buffer_out[current_buffer] = video_out;



//printf("VAttachmentPoint::render 2\n");
// Run base class
	AttachmentPoint::render(current_position, 1);
//printf("VAttachmentPoint::render 3\n");
}

void VAttachmentPoint::dispatch_plugin_server(int buffer_number, 
	long current_position, 
	long fragment_size)
{
// Current position must be relative to plugin position since the keyframes
// are relative.

//printf("VAttachmentPoint::dispatch_plugin_server 1 %d %d\n", current_position, plugin->startproject);
	if(plugin_server->multichannel)
		plugin_servers.values[0]->process_realtime(
			buffer_in, 
			buffer_out, 
			current_position /* - plugin->startproject */,
			plugin->length);
	else
		plugin_servers.values[buffer_number]->process_realtime(
			&buffer_in[buffer_number], 
			&buffer_out[buffer_number],
			current_position /* - plugin->startproject */,
			plugin->length);
//printf("VAttachmentPoint::dispatch_plugin_server 2\n");
}

