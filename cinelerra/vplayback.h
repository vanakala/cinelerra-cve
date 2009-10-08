
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

#ifndef VPLAYBACK_H
#define VPLAYBACK_H

#include "playbackengine.h"
#include "vwindow.inc"

class VPlayback : public PlaybackEngine
{
public:
	VPlayback(MWindow *mwindow, VWindow *vwindow, Canvas *output);

	int create_render_engine();
	void init_cursor();
	void stop_cursor();
	void goto_start();
	void goto_end();
	VWindow *vwindow;
};

#endif
