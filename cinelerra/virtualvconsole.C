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
//printf("VirtualVConsole::VirtualVConsole\n");
	this->vrender = vrender;
	buffer_in = 0;
}

VirtualVConsole::~VirtualVConsole()
{
//printf("VirtualVConsole::~VirtualVConsole 1\n");
}

int VirtualVConsole::total_ring_buffers()
{
	return 1;
}

void VirtualVConsole::get_playable_tracks()
{
//printf("VirtualVConsole::get_playable_tracks 1\n");
	if(!playable_tracks)
		playable_tracks = new PlayableTracks(renderengine, 
			commonrender->current_position, 
			TRACK_VIDEO);
}

void VirtualVConsole::new_input_buffer(int ring_buffer)
{
	buffer_in = new VFrame*[total_tracks];
//printf("VirtualVConsole::new_input_buffer 1\n");
	for(int i = 0; i < total_tracks; i++)
	{
		buffer_in[i] = new VFrame(0,
			playable_tracks->values[i]->track_w,
			playable_tracks->values[i]->track_h,
			renderengine->edl->session->color_model,
			-1);
	}
}

void VirtualVConsole::delete_input_buffer(int ring_buffer)
{
	for(int i = 0; i < total_tracks; i++)
	{
		delete buffer_in[i];
	}
	delete [] buffer_in;
}

VirtualNode* VirtualVConsole::new_toplevel_node(Track *track, 
	Module *module,
	int track_number)
{
	return new VirtualVNode(renderengine, 
		this, 
		module,
		0,
		track,
		0,
		buffer_in[track_number],
		buffer_in[track_number],
		1,
		1,
		1,
		1);
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

//printf("VirtualVConsole::process_buffer 2 %d\n", total_tracks);
// Arm input buffers
	for(i = 0; i < total_tracks; i++)
		result |= ((VModule*)virtual_modules[i]->real_module)->render(buffer_in[i],
			input_position,
			renderengine->command->get_direction());
//return;

//printf("VirtualVConsole::process_buffer 3 %d\n", render_list.total);
// render nodes in sorted list
	for(i = 0; i < render_list.total; i++)
	{
		((VirtualVNode*)render_list.values[i])->render(vrender->video_out, 
				input_position);
	}

//printf("VirtualVConsole::process_buffer 4\n");
	return result;
}

















int VirtualVConsole::init_rendering(int duplicate)
{
	return 0;
}


int VirtualVConsole::stop_rendering(int duplicate)
{
	return 0;
}


int VirtualVConsole::send_last_output_buffer()
{
	return 0;
}
