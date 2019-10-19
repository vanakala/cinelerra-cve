
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

#include "condition.h"
#include "edl.h"
#include "renderbase.h"
#include "renderengine.h"
#include "thread.h"
#include "trackrender.h"
#include "transportcommand.h"

RenderBase::RenderBase(RenderEngine *renderengine, EDL *edl)
 : Thread(THREAD_SYNCHRONOUS)
{
	this->renderengine = renderengine;
	this->edl = edl;
	start_lock = new Condition(0, "CommonRender::start_lock");
}

void RenderBase::arm_command()
{
	render_pts = renderengine->command.playbackstart;
	render_start = renderengine->command.start_position;
	render_end = renderengine->command.end_position;
	render_direction = renderengine->command.get_direction();
}

void RenderBase::start_command()
{
	if(renderengine->command.realtime)
	{
		Thread::start();
		start_lock->lock("RenderBase::start_command");
	}
}

void RenderBase::run()
{
	start_lock->unlock();
}
