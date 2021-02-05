// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef CPLAYBACK_H
#define CPLAYBACK_H

#include "cwindow.inc"
#include "playbackengine.h"
#include "datatype.h"

class CPlayback : public PlaybackEngine
{
public:
	CPlayback(CWindow *cwindow, Canvas *output);

	void init_cursor();
	void stop_cursor();
	int brender_available(ptstime position);

	CWindow *cwindow;
};

#endif
