
/*
 * CINELERRA
 * Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#ifndef RENDERBASE_H
#define RENDERBASE_H

#include "condition.inc"
#include "edl.inc"
#include "plugin.inc"
#include "renderbase.inc"
#include "renderengine.h"
#include "thread.h"
#include "trackrender.inc"

class RenderBase : public Thread
{
public:
	RenderBase(RenderEngine *renderengine, EDL *edl);

	void arm_command();
	void start_command();
	virtual void run();
// advance playback position
// returns NZ when next frame must be flashed
	int advance_position(ptstime duration);
// Check that shared plugin is ready for rendering
	int is_shared_ready(Plugin *plugin, ptstime pts);

protected:
	virtual void init_frames() {};

	RenderEngine *renderengine;
	EDL *edl;
	ptstime render_pts;
	ptstime render_start;
	ptstime render_end;
	int render_direction;
	int render_loop;
	int render_single;
	int render_realtime;
	int last_playback;
	double render_speed;
	Condition *start_lock;
};

#endif
