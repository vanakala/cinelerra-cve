#ifndef KEYFRAME_H
#define KEYFRAME_H

#include "auto.h"
#include "filexml.inc"
#include "keyframes.inc"
#include "messages.inc"

// The default constructor is used for menu effects and pasting effects.

class KeyFrame : public Auto
{
public:
	KeyFrame();
	KeyFrame(EDL *edl, KeyFrames *autos);
	virtual ~KeyFrame();
	
	void load(FileXML *file);
	void copy(int64_t start, int64_t end, FileXML *file, int default_only);
	void copy_from(Auto *that);
	void copy_from(KeyFrame *that);
	void copy_from_common(KeyFrame *that);
	int operator==(Auto &that);
	int operator==(KeyFrame &that);
	void dump();
	int identical(KeyFrame *src);

	char data[MESSAGESIZE];
};

#endif
