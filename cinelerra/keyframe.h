// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef KEYFRAME_H
#define KEYFRAME_H

#include "auto.h"
#include "filexml.inc"
#include "keyframes.inc"

// The default constructor is used for menu effects and pasting effects.

class KeyFrame : public Auto
{
public:
	KeyFrame();
	KeyFrame(EDL *edl, KeyFrames *autos);
	~KeyFrame();

	void load(FileXML *file);
	char *get_data();
	size_t data_size();
	void set_data(const char *string);
	void save_xml(FileXML *file);
	void copy(Auto *that, ptstime start, ptstime end);
	void copy_from(Auto *that);
	void copy_from(KeyFrame *that);
	int operator==(Auto &that);
	int operator==(KeyFrame &that);
	void dump(int indent = 0);
	int identical(KeyFrame *src);
	inline int has_drawn(int pos_x) { return pos_x == drawn_x; };
	inline int get_pos_x() { return drawn_x; };
	void drawing(int pos_x);
	size_t get_size();

private:
	char *data;
	int drawn_x;
};

#endif
