#ifndef SELECTIONS_H
#define SELECTIONS_H

#include "edits.h"
#include "edl.inc"
#include "selection.inc"
#include "track.inc"


// Only used by video tracks
class Selections : public Edits
{
public:
	Selections(EDL *edl, Track *track);
	~Selections();

	virtual Selections& operator=(Selections& selections);

	Track *track;
};

#endif
