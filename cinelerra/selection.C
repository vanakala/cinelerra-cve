#include "selection.h"
#include "selections.h"


Selection::Selection(EDL *edl, Selections *selections)
 : Edit(edl, selections)
{
	this->selections = selections;
}


Selection::~Selection()
{
}

void Selection::synchronize_params(Selection *selection)
{
}


