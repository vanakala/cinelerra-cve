#include "atrack.h"
#include "recordableatracks.h"
#include "tracks.h"

// This is only used for menu effects so use playable tracks instead.

RecordableATracks::RecordableATracks(Tracks *tracks)
 : ArrayList<ATrack*>()
{
	Track *current_track;
	
	for(current_track = tracks->first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record && current_track->data_type == TRACK_AUDIO) 
			append((ATrack*)current_track);
	}
}
