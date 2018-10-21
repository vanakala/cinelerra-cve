
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
#include "edl.inc"
#include "file.h"
#include "vedit.h"
#include "vedits.h"
#include "vframe.h"
#include "vtrack.h"

VEdit::VEdit(EDL *edl, Edits *edits)
 : Edit(edl, edits)
{
}

// ================================================== editing
int VEdit::read_frame(VFrame *video_out, 
	ptstime input_postime,
	CICache *cache,
	int use_nudge,
	int use_cache)
{
	File *file = cache->check_out(asset, edl);

	if(use_nudge) input_postime += track->nudge;

	if(file)
	{
		video_out->set_layer(channel);
		video_out->set_source_pts(input_postime - get_pts() + get_source_pts());
		if(use_cache) file->set_cache_frames(use_cache);
		int result = file->get_frame(video_out);
		if(use_cache) file->set_cache_frames(0);
		video_out->set_pts(video_out->get_source_pts() - get_source_pts() + get_pts());
		cache->check_in(asset);
		return result;
	}
	return 1;
}

ptstime VEdit::get_source_end()
{
	if(!asset)
		return 0;

	return asset->video_duration;
}
