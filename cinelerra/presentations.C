#include "presentations.h"


Presentations::Presentations(EDL *edl)
 : Labels(edl, "PRESENTATIONS")
{
}

Presentations::~Presentations()
{
}





Presentation::Presentation(EDL *edl, Presentations *presentations, long position, int type)
 : Label(edl, presentations, position)
{
	this->type = type;
}

Presentation::~Presentation()
{
}





