#ifndef RECORDABLEATRACKS_H
#define RECORDABLEATRACKS_H

#include "arraylist.h"
#include "tracks.inc"

class RecordableATracks : public ArrayList<ATrack*>
{
public:
	RecordableATracks(Tracks *tracks);
};



#endif
