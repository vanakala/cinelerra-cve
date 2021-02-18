// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef VPLAYBACK_H
#define VPLAYBACK_H

#include "playbackengine.h"
#include "vwindow.inc"

class VPlayback : public PlaybackEngine
{
public:
	VPlayback(VWindow *vwindow, Canvas *output);

	void init_cursor();
	void stop_cursor();

	VWindow *vwindow;
};

#endif
