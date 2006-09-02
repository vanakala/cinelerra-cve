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




	if(debug_tree) 
		printf("VirtualVConsole::process_buffer begin exit_nodes=%d\n", 
			exit_nodes.total);


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
			output_temp = new VFrame(0, 
				track->track_w, 
				track->track_h, 
				renderengine->edl->session->color_model,
				-1);
		}

//printf("VirtualVConsole::process_buffer %p\n", output_temp->get_rows());
		result |= node->render(output_temp,
			input_position + track->nudge,
			renderengine->edl->session->frame_rate);
	}
//printf("VirtualVConsole::process_buffer timer=%lld\n", timer.get_difference());

	if(debug_tree) printf("VirtualVConsole::process_buffer end\n");
	return result;
}

