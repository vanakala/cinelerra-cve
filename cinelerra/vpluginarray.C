#include "cache.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "mwindow.h"
#include "pluginserver.h"
#include "preferences.h"
#include "recordablevtracks.h"
#include "mainsession.h"
#include "vframe.h"
#include "vmodule.h"
#include "vpluginarray.h"
#include "vtrack.h"

VPluginArray::VPluginArray()
 : PluginArray()
{
	realtime_buffers = 0;
}

VPluginArray::~VPluginArray()
{
	file->stop_video_thread();
	for(int i = 0; i < total_tracks(); i++)
	{
		delete modules[i];
	}
	delete tracks;
}

void VPluginArray::get_recordable_tracks()
{
	tracks = new RecordableVTracks(mwindow->edl->tracks);
}

int64_t VPluginArray::get_bufsize()
{
	return 1;
}

void VPluginArray::create_buffers()
{
	file->start_video_thread(buffer_size,
		mwindow->edl->session->color_model,
		RING_BUFFERS,
		0);
//	if(!plugin_server->realtime) realtime_buffers = file->get_video_buffer();
}

void VPluginArray::create_modules()
{
	modules = new Module*[total_tracks()];
	for(int i = 0; i < total_tracks(); i++)
	{
		modules[i] = new VModule(0, 0, this, tracks->values[i]);
		modules[i]->cache = cache;
		modules[i]->edl = edl;
		modules[i]->create_objects();
		modules[i]->render_init();
	}
}

void VPluginArray::load_module(int module, int64_t input_position, int64_t len)
{
	if(module == 0) realtime_buffers = file->get_video_buffer();
	((VModule*)modules[module])->render(realtime_buffers[module][0],
		input_position,
		PLAY_FORWARD,
		0);
}

void VPluginArray::process_realtime(int module, int64_t input_position, int64_t len)
{
// Convert from array of frames to array of tracks
	VFrame **temp_buffer;
	temp_buffer = new VFrame*[total_tracks()];
	for(int i = 0; i < total_tracks(); i++)
	{
		temp_buffer[i] = realtime_buffers[i][0];
	}
	values[module]->process_realtime(&temp_buffer[module], 
			realtime_buffers[module],
			input_position, 
			0);
	delete [] temp_buffer;
}

int VPluginArray::process_loop(int module, int64_t &write_length)
{
	if(!realtime_buffers) realtime_buffers = file->get_video_buffer();

// Convert from array of frames to array of tracks
	VFrame **temp_buffer;
	temp_buffer = new VFrame*[total_tracks()];
	for(int i = 0; i < total_tracks(); i++)
	{
		temp_buffer[i] = realtime_buffers[i][0];
	}

	int result = values[module]->process_loop(realtime_buffers[module], write_length);
	delete [] temp_buffer;
	return result;
}

int VPluginArray::write_buffers(int64_t len)
{
	int result = file->write_video_buffer(len);
	realtime_buffers = 0;

//	if(!plugin_server->realtime && !done && !result) realtime_buffers = file->get_video_buffer();
	return result;
}


int VPluginArray::total_tracks()
{
	return tracks->total;
}

Track* VPluginArray::track_number(int number)
{
	return (Track*)tracks->values[number];
}


