#include "vtrack.h"
#include "recordablevtracks.h"
#include "tracks.h"

// This is only used for menu effects so use playable tracks instead.

RecordableVTracks::RecordableVTracks(Tracks *tracks)
 : ArrayList<VTrack*>()
{
	Track *current_track;
	for(current_track = tracks->first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record && current_track->data_type == TRACK_VIDEO) 
			append((VTrack*)current_track);
	}
}
