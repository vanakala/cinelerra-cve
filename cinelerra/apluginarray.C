#include "amodule.h"
#include "apluginarray.h"
#include "atrack.h"
#include "cache.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "mwindow.h"
#include "playbackengine.h"
#include "pluginserver.h"
#include "preferences.h"
#include "recordableatracks.h"
#include "mainsession.h"

APluginArray::APluginArray()
 : PluginArray()
{
	realtime_buffers = 0;
}

APluginArray::~APluginArray()
{
	file->stop_audio_thread();
	for(int i = 0; i < total_tracks(); i++)
	{
		delete modules[i];
	}
	delete tracks;
}

void APluginArray::create_buffers()
{
	file->start_audio_thread(buffer_size, RING_BUFFERS);
//	if(!plugin_server->realtime) realtime_buffers = file->get_audio_buffer();
}

void APluginArray::create_modules()
{
	modules = new Module*[total_tracks()];
	for(int i = 0; i < total_tracks(); i++)
	{
		modules[i] = new AModule(0, 0, this, tracks->values[i]);
		modules[i]->cache = cache;
		modules[i]->edl = edl;
		modules[i]->create_objects();
		modules[i]->render_init();
	}
}

void APluginArray::load_module(int module, int64_t input_position, int64_t len)
{
	if(module == 0) realtime_buffers = file->get_audio_buffer();
//printf("APluginArray::load_module 1 %d %d %p %p\n", module, len, realtime_buffers, realtime_buffers[module]);
	((AModule*)modules[module])->render(realtime_buffers[module], 
		len, 
		input_position,
		PLAY_FORWARD,
		0);
}

void APluginArray::process_realtime(int module, int64_t input_position, int64_t len)
{
//printf("APluginArray::process_realtime 1 %d %p %p\n", len, realtime_buffers, realtime_buffers[0]);
	values[module]->process_realtime(realtime_buffers + module, 
			realtime_buffers + module,
			input_position, 
			len,
			0);
}

int APluginArray::process_loop(int module, int64_t &write_length)
{
//printf("APluginArray::process_loop 1\n");
	if(!realtime_buffers) realtime_buffers = file->get_audio_buffer();
//printf("APluginArray::process_loop 2\n");
	int result = values[module]->process_loop(&realtime_buffers[module], 
		write_length);
//printf("APluginArray::process_loop 3\n");
	return result;
}


int APluginArray::write_buffers(int64_t len)
{
//printf("APluginArray::write_buffers 1\n");
	int result = file->write_audio_buffer(len);
	realtime_buffers = 0;

//	if(!plugin_server->realtime && !done && !result) 
//		realtime_buffers = file->get_audio_buffer();
	return result;
}

int64_t APluginArray::get_bufsize()
{
	return mwindow->edl->session->sample_rate;
}


void APluginArray::get_recordable_tracks()
{
	tracks = new RecordableATracks(mwindow->edl->tracks);
}

int APluginArray::total_tracks()
{
	return tracks->total;
}

Track* APluginArray::track_number(int number)
{
	return (Track*)tracks->values[number];
}
