#ifndef TRACKLIST_H
#define TRACKLIST_H

#include "patchbay.h"
#include "arraylist.h"
#include "tracks.h"

class TrackList : public ArrayList<Track*>
{
public:
	TrackList(MWindow *mwindow);
	
	get_playable_audio(long position, int reverse);
	get_playable_video(long position, int reverse);

private:
	get_playable_type(long position, int reverse, int data_type);
	get_recordable_type(int data_type);
	MWindow *mwindow;
};



#endif
