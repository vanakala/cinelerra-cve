#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "playabletracks.h"
#include "preferences.h"
#include "renderengine.h"
#include "tracks.h"
#include "transportque.h"
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
	if(output_temp) delete output_temp;
}

void VirtualVConsole::get_playable_tracks()
{
	if(!playable_tracks)
		playable_tracks = new PlayableTracks(renderengine, 
			commonrender->current_position, 
			TRACK_VIDEO,
			1);
}

// void VirtualVConsole::new_input_buffer(int ring_buffer)
// {
// 	buffer_in = new VFrame*[total_tracks];
// //printf("VirtualVConsole::new_input_buffer 1\n");
// 	for(int i = 0; i < total_tracks; i++)
// 	{
// 		buffer_in[i] = new VFrame(0,
// 			playable_tracks->values[i]->track_w,
// 			playable_tracks->values[i]->track_h,
// 			renderengine->edl->session->color_model,
// 			-1);
// 	}
// }
// 
// void VirtualVConsole::delete_input_buffer(int ring_buffer)
// {
// 	for(int i = 0; i < total_tracks; i++)
// 	{
// 		delete buffer_in[i];
// 	}
// 	delete [] buffer_in;
// }

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

//printf("VirtualVConsole::process_buffer 1\n");
// clear output buffers
	for(i = 0; i < MAX_CHANNELS; i++)
	{
		if(vrender->video_out[i])
			vrender->video_out[i]->clear_frame();
	}


// Reset plugin rendering status
	reset_attachments();

// Render exit nodes from bottom to top
	for(int i = exit_nodes.total - 1; i >= 0; i--)
	{
		VirtualVNode *node = (VirtualVNode*)exit_nodes.values[i];
		Track *track = node->track;

// Create temporary output to match the track size, which is acceptable since
// most projects don't have variable track sizes.
		if(output_temp && 
			(output_temp->get_w() != track->track_w ||
			output_temp->get_h() != track->track_h))
		{
			delete output_temp;
			output_temp = 0;
		}

		if(!output_temp)
		{
			output_temp = new VFrame(0, 
				track->track_w, 
				track->track_h, 
				renderengine->edl->session->color_model,
				-1);
		}

//printf("VirtualVConsole::process_buffer 1 %p\n", output_temp);
		result |= node->render(output_temp,
			input_position + track->nudge,
			renderengine->edl->session->frame_rate);
	}

// // Arm input buffers
// 	for(i = 0; i < total_tracks; i++)
// 		result |= ((VModule*)virtual_modules[i]->real_module)->render(buffer_in[i],
// 			input_position,
// 			renderengine->command->get_direction(),
// 			1);
//
// // render nodes in sorted list
// 	for(i = 0; i < render_list.total; i++)
// 	{
// 		((VirtualVNode*)render_list.values[i])->render(vrender->video_out, 
// 				input_position);
// 	}
//printf("VirtualVConsole::process_buffer 10\n");

	return result;
}

