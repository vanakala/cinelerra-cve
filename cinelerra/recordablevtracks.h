#ifndef RECORDABLEVTRACKS_H
#define RECORDABLEVTRACKS_H

#include "arraylist.h"
#include "tracks.inc"
#include "vtrack.inc"

class RecordableVTracks : public ArrayList<VTrack*>
{
public:
	RecordableVTracks(Tracks *tracks);
};



#endif
