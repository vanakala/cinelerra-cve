
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

#include "cache.h"
#include "bcsignals.h"
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


#define RING_BUFFERS 2



VPluginArray::VPluginArray()
 : PluginArray(TRACK_VIDEO)
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
	tracks = new RecordableVTracks(edl->tracks);
}

int VPluginArray::get_bufsize()
{
	return 1;
}

void VPluginArray::create_buffers()
{
	file->start_video_thread(buffer_size,
		edl->session->color_model,
		RING_BUFFERS,
		0);
}

void VPluginArray::get_buffers()
{
	if(!realtime_buffers) realtime_buffers = file->get_video_buffer();
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

void VPluginArray::process_realtime(int module, ptstime pts, ptstime len)
{
	(*realtime_buffers[module])->set_pts(pts);
	values[module]->process_buffer(realtime_buffers[module], len);
}

int VPluginArray::process_loop(int module, int &write_length)
{
	if(!realtime_buffers) realtime_buffers = file->get_video_buffer();

// Convert from array of frames to array of tracks
	VFrame **temp_buffer;
	temp_buffer = new VFrame*[total_tracks()];
	for(int i = 0; i < total_tracks(); i++)
	{
		temp_buffer[i] = realtime_buffers[i][0];
	}

	int result = values[module]->process_loop(realtime_buffers[module]);
	write_length = 1;
	delete [] temp_buffer;
	return result;
}

int VPluginArray::write_buffers(int len)
{
	int result = file->write_video_buffer(len);
	realtime_buffers = 0;

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

posnum VPluginArray::to_units(ptstime pts)
{
	return round(pts * edl->session->frame_rate);
}

ptstime VPluginArray::from_units(posnum pos)
{
	return (ptstime)pos / edl->session->frame_rate;
}
