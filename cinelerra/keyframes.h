#ifndef KEYFRAMES_H
#define KEYFRAMES_H

#include "autos.h"
#include "filexml.inc"
#include "keyframe.inc"


// Keyframes inherit from Autos to reuse the editing commands but
// keyframes don't belong to tracks.  Instead they belong to plugins
// because their data is specific to plugins.


class KeyFrames : public Autos
{
public:
	KeyFrames(EDL *edl, Track *track);
	~KeyFrames();

	Auto* new_auto();
	void dump();
};

#endif
