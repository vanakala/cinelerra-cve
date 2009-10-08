
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

#include "asset.h"
#include "bcsignals.h"
#include "cache.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "mwindow.h"
#include "patch.h"
#include "preferences.h"
#include "mainsession.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "units.h"
#include "vedit.h"
#include "vedits.h"
#include "vframe.h"
#include "vtrack.h"

VEdit::VEdit(EDL *edl, Edits *edits)
 : Edit(edl, edits)
{
}


VEdit::~VEdit() { }

int VEdit::load_properties_derived(FileXML *xml)
{
	channel = xml->tag.get_property("CHANNEL", (int64_t)0);
	return 0;
}





// ================================================== editing



int VEdit::read_frame(VFrame *video_out, 
	int64_t input_position, 
	int direction,
	CICache *cache,
	int use_nudge,
	int use_cache,
	int use_asynchronous)
{
	File *file = cache->check_out(asset,
		edl);
	int result = 0;
	if(use_nudge) input_position += track->nudge;

	if(file)
	{

		input_position = (direction == PLAY_FORWARD) ? input_position : (input_position - 1);

		if(use_asynchronous)
			file->start_video_decode_thread();
		else
			file->stop_video_thread();

		file->set_layer(channel);
		file->set_video_position(input_position - startproject + startsource, edl->session->frame_rate);
		if(use_cache) file->set_cache_frames(use_cache);
		result = file->read_frame(video_out);
		if(use_cache) file->set_cache_frames(0);

		cache->check_in(asset);
	}
	else
		result = 1;
	
//for(int i = 0; i < video_out->get_w() * 3 * 20; i++) video_out->get_rows()[0][i] = 128;
	return result;
}

int VEdit::copy_properties_derived(FileXML *xml, int64_t length_in_selection)
{
	return 0;
}

int VEdit::dump_derived()
{
	printf("	VEdit::dump_derived\n");
	printf("		startproject %ld\n", startproject);
	printf("		length %ld\n", length);
}

int64_t VEdit::get_source_end(int64_t default_)
{
	if(!asset) return default_;   // Infinity

	return (int64_t)((double)asset->video_length / asset->frame_rate * edl->session->frame_rate + 0.5);
}
