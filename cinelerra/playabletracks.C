#include "automation.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "patchbay.h"
#include "playabletracks.h"
#include "plugin.h"
#include "preferences.h"
#include "renderengine.h"
#include "intauto.h"
#include "intautos.h"
#include "tracks.h"
#include "transportque.h"


PlayableTracks::PlayableTracks(RenderEngine *renderengine, 
	long current_position, 
	int data_type,
	int use_nudge)
 : ArrayList<Track*>()
{
	this->renderengine = renderengine;
	this->data_type = data_type;

//printf("PlayableTracks::PlayableTracks 1 %d\n", renderengine->edl->tracks->total());
	for(Track *current_track = renderengine->edl->tracks->first; 
		current_track; 
		current_track = current_track->next)
	{
		if(is_playable(current_track, current_position, use_nudge))
		{
//printf("PlayableTracks::PlayableTracks 1 %p %d %d\n", this, total, current_position);
			append(current_track);
		}
	}
//printf("PlayableTracks::PlayableTracks %d %d %d\n", data_type, total, current_position);
}

PlayableTracks::~PlayableTracks()
{
}


int PlayableTracks::is_playable(Track *current_track, 
	long position,
	int use_nudge)
{
	int result = 1;
	int direction = renderengine->command->get_direction();
	if(use_nudge) position += current_track->nudge;

	Auto *current = 0;
	int *do_channel;
	switch(data_type)
	{
		case TRACK_AUDIO:
			do_channel = renderengine->config->aconfig->do_channel;
			break;
		case TRACK_VIDEO:
			do_channel = renderengine->config->vconfig->do_channel;
			break;
	}

	if(current_track->data_type != data_type) result = 0;
	

// Track is off screen and not bounced to other modules


	if(result)
	{
		if(!current_track->plugin_used(position, direction) &&
			!current_track->channel_is_playable(position, direction, do_channel))
			result = 0;
	}

// Test play patch
	if(!current_track->play)
	{
		result = 0;
	}

	if(result)
	{
// Test for playable edit
		if(!current_track->playable_edit(position, direction))
		{
// Test for playable effect
			if(!current_track->is_synthesis(renderengine,
						position,
						direction))
			{
				result = 0;
			}
		}
	}

	return result;
}


int PlayableTracks::is_listed(Track *track)
{
	for(int i = 0; i < total; i++)
	{
		if(values[i] == track) return 1;
	}
	return 0;
}
