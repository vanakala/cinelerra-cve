#ifndef SELECTION_H
#define SELECTION_H

#include "edl.inc"
#include "edit.h"
#include "selections.inc"


class Selection : public Edit
{
public:
	Selection(EDL *edl, Selections *selections);
	virtual ~Selection();
	
	void synchronize_params(Selection *selection);
	
	EDL *edl;
	Selections *selections;
};

#endif
