#ifndef PLAYABLETRACKS_H
#define PLAYABLETRACKS_H

#include "arraylist.h"
#include "datatype.h"
#include "mwindow.inc"
#include "renderengine.inc"
#include "track.h"

class PlayableTracks : public ArrayList<Track*>
{
public:
	PlayableTracks(RenderEngine *renderengine, 
		long current_position,  // Position in native units of tracks
		int data_type,
		int use_nudge);
	~PlayableTracks();

// return 1 if the track is playable at the position
	int is_playable(Track *current_track, 
		long position,
		int use_nudge);
// return 1 if the track is in the list
	int is_listed(Track *track);

	RenderEngine *renderengine;
	int data_type;
	MWindow *mwindow;
};



#endif
