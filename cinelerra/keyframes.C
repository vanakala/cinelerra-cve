#include "filexml.h"
#include "keyframe.h"
#include "keyframes.h"

KeyFrames::KeyFrames(EDL *edl, Track *track)
 : Autos(edl, track)
{
}

KeyFrames::~KeyFrames()
{
}

Auto* KeyFrames::new_auto()
{
	return new KeyFrame(edl, this);
}


void KeyFrames::dump()
{
	printf("    DEFAULT_KEYFRAME\n");
	((KeyFrame*)default_auto)->dump();
	printf("    KEYFRAMES total=%d\n", total());
	for(KeyFrame *current = (KeyFrame*)first;
		current;
		current = (KeyFrame*)NEXT)
	{
		current->dump();
	}
}

