#include "aattachmentpoint.h"
#include "edl.h"
#include "edlsession.h"
#include "plugin.h"
#include "pluginserver.h"
#include "renderengine.h"

AAttachmentPoint::AAttachmentPoint(RenderEngine *renderengine, Plugin *plugin)
: AttachmentPoint(renderengine, plugin)
{
	buffer_in = 0;
	buffer_out = 0;
}

AAttachmentPoint::~AAttachmentPoint()
{
}

void AAttachmentPoint::delete_buffer_vectors()
{
	if(buffer_in) delete [] buffer_in;
	if(buffer_out) delete [] buffer_out;
	buffer_in = 0;
	buffer_out = 0;
}

void AAttachmentPoint::new_buffer_vectors()
{
	buffer_in = new double*[total_input_buffers];
	buffer_out = new double*[total_input_buffers];
}

int AAttachmentPoint::get_buffer_size()
{
//printf("AAttachmentPoint::get_buffer_size 1 %p\n", renderengine);
	return renderengine->edl->session->audio_module_fragment;
}

void AAttachmentPoint::render(double *audio_in, 
	double *audio_out, 
	long fragment_size, 
	long current_position)
{
// Arm buffer vectors
	buffer_in[current_buffer] = audio_in;
	buffer_out[current_buffer] = audio_out;
// Run base class
	AttachmentPoint::render(current_position, fragment_size);
}

void AAttachmentPoint::dispatch_plugin_server(int buffer_number, 
	long current_position, 
	long fragment_size)
{
//printf("AAttachmentPoint::dispatch_plugin_server 1\n");
// Current position must be relative to plugin position since the keyframes
// are relative.

	if(plugin_server->multichannel)
	{
		plugin_servers.values[0]->process_realtime(
			buffer_in, 
			buffer_out, 
			current_position /* - plugin->startproject */, 
			fragment_size,
			plugin->length);
	}
	else
	{
		plugin_servers.values[buffer_number]->process_realtime(
			&buffer_in[buffer_number], 
			&buffer_out[buffer_number],
			current_position /* - plugin->startproject */, 
			fragment_size,
			plugin->length);
	}
}


