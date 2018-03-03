
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

#include "bcsignals.h"
#include "bcresources.h"
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mwindow.h"
#include "playabletracks.h"
#include "renderengine.h"
#include "tmpframecache.h"
#include "vframe.h"
#include "virtualvconsole.h"
#include "virtualvnode.h"
#include "vmodule.h"
#include "vrender.h"

VirtualVConsole::VirtualVConsole(RenderEngine *renderengine, VRender *vrender)
 : VirtualConsole(renderengine, vrender, TRACK_VIDEO)
{
	this->vrender = vrender;
	playable_tracks = new PlayableTracks(renderengine,
		commonrender->current_postime,
		TRACK_VIDEO, 1);
}

VirtualNode* VirtualVConsole::new_entry_node(Track *track, 
	Module *module,
	int track_number)
{
	return new VirtualVNode(renderengine, 
		this, 
		module,
		0,
		track,
		0);
}

// start of buffer in project if forward / end of buffer if reverse
void VirtualVConsole::process_buffer(ptstime input_postime)
{
	int i, j, k;

// clear device buffer
	vrender->video_out->clear_frame();

// Reset plugin rendering status
	reset_attachments();

// Render exit nodes from bottom to top
	for(current_exit_node = exit_nodes.total - 1; current_exit_node >= 0; current_exit_node--)
	{
		VirtualVNode *node = (VirtualVNode*)exit_nodes.values[current_exit_node];
		Track *track = node->track;

		output_temp = BC_Resources::tmpframes.get_tmpframe(
			track->track_w,
			track->track_h,
			renderengine->edl->session->color_model);

		output_temp->set_pts(input_postime + track->nudge);
		node->render();

		BC_Resources::tmpframes.release_frame(output_temp);
	}

	if(!exit_nodes.total)
		vrender->video_out->set_pts(input_postime);

	return;
}
