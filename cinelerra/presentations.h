#ifndef PRESENTATIONS_H
#define PRESENTATIONS_H

#include "labels.h"
#include "presentations.inc"

class Presentation : public Label
{
public:
	Presentation(EDL *edl, Presentations *presentations, long position, int type);
	~Presentation();
	
	int type;
};

class Presentations : public Labels
{
public:
	Presentations(EDL *edl);
	~Presentations();
};


#endif
