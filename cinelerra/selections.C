#include "selection.h"
#include "selections.h"
#include "track.h"

Selections::Selections(EDL *edl, Track *track)
 : Edits(edl, track)
{
	this->track = track;
}

Selections::~Selections()
{
}


Selections& Selections::operator=(Selections& selections)
{
	while(last) delete last;
	for(Selection *current = (Selection*)selections.first; current; current = (Selection*)NEXT)
	{
		Selection *new_selection = (Selection*)append(new Selection(edl, this));
		*new_selection = *current;
	}
	return *this;
}
