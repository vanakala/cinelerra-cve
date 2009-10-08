
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
#include "bctimer.h"
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "playabletracks.h"
#include "preferences.h"
#include "renderengine.h"
#include "tracks.h"
#include "transportque.h"
#include "vdevicex11.h"
#include "vframe.h"
#include "videodevice.h"
#include "virtualvconsole.h"
#include "virtualvnode.h"
#include "vmodule.h"
#include "vrender.h"
#include "vtrack.h"

VirtualVConsole::VirtualVConsole(RenderEngine *renderengine, VRender *vrender)
 : VirtualConsole(renderengine, vrender, TRACK_VIDEO)
{
	this->vrender = vrender;
	output_temp = 0;
}

VirtualVConsole::~VirtualVConsole()
{
	if(output_temp)
	{
		delete output_temp;
	}
}

VDeviceBase* VirtualVConsole::get_vdriver()
{
	return renderengine->video->get_output_base();
}

void VirtualVConsole::get_playable_tracks()
{
	if(!playable_tracks)
		playable_tracks = new PlayableTracks(renderengine, 
			commonrender->current_position, 
			TRACK_VIDEO,
			1);
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
int VirtualVConsole::process_buffer(int64_t input_position)
{
	int i, j, k;
	int result = 0;



// The use of single frame is determined in RenderEngine::arm_command
	use_opengl = (renderengine->video && 
		renderengine->video->out_config->driver == PLAYBACK_X11_GL);
// printf("VirtualVConsole::process_buffer %p %d %d\n", 
// renderengine->video, 
// renderengine->video->out_config->driver,
// use_opengl);

	if(debug_tree) 
		printf("VirtualVConsole::process_buffer begin exit_nodes=%d\n", 
			exit_nodes.total);


	if(use_opengl)
	{
// clear hardware framebuffer

		((VDeviceX11*)get_vdriver())->clear_output();

// que OpenGL driver that everything is overlaid in the framebuffer
		vrender->video_out->set_opengl_state(VFrame::SCREEN);

	}
	else
	{
// clear device buffer
		vrender->video_out->clear_frame();
	}






// Reset plugin rendering status
	reset_attachments();

Timer timer;
// Render exit nodes from bottom to top
	for(current_exit_node = exit_nodes.total - 1; current_exit_node >= 0; current_exit_node--)
	{
		VirtualVNode *node = (VirtualVNode*)exit_nodes.values[current_exit_node];
		Track *track = node->track;

// Create temporary output to match the track size, which is acceptable since
// most projects don't have variable track sizes.
// If the project has variable track sizes, this object is recreated for each track.





		if(output_temp && 
			(output_temp->get_w() != track->track_w ||
			output_temp->get_h() != track->track_h))
		{
			delete output_temp;
			output_temp = 0;
		}


		if(!output_temp)
		{
// Texture is created on demand
			output_temp = new VFrame(0, 
				track->track_w, 
				track->track_h, 
				renderengine->edl->session->color_model,
				-1);
		}

// Reset OpenGL state
		if(use_opengl)
			output_temp->set_opengl_state(VFrame::RAM);


// Assume openGL is used for the final stage and let console
// disable.
		output_temp->clear_stacks();
		result |= node->render(output_temp,
			input_position + track->nudge,
			renderengine->edl->session->frame_rate,
			use_opengl);

	}
//printf("VirtualVConsole::process_buffer timer=%lld\n", timer.get_difference());

	if(debug_tree) printf("VirtualVConsole::process_buffer end\n");
	return result;
}

