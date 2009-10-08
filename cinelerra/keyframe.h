
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

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
