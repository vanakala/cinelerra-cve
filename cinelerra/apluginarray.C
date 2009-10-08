
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


#define RING_BUFFERS 2


APluginArray::APluginArray()
 : PluginArray(TRACK_AUDIO)
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

void APluginArray::get_buffers()
{
	if(!realtime_buffers) realtime_buffers = file->get_audio_buffer();
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

void APluginArray::process_realtime(int module, int64_t input_position, int64_t len)
{
	values[module]->process_buffer(realtime_buffers + module,
			input_position, 
			len,
			edl->session->sample_rate,
			0,
			PLAY_FORWARD);
}

int APluginArray::process_loop(int module, int64_t &write_length)
{
	if(!realtime_buffers) realtime_buffers = file->get_audio_buffer();
	int result = values[module]->process_loop(&realtime_buffers[module], 
		write_length);
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
	return edl->session->sample_rate;
}


void APluginArray::get_recordable_tracks()
{
	tracks = new RecordableATracks(edl->tracks);
}

int APluginArray::total_tracks()
{
	return tracks->total;
}

Track* APluginArray::track_number(int number)
{
	return (Track*)tracks->values[number];
}
