
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

#ifndef BCREPEATER_H
#define BCREPEATER_H

#include "bcrepeater.inc"
#include "bcwindowbase.inc"
#include "condition.inc"
#include "thread.h"
#include "bctimer.h"

class BC_Repeater : public Thread
{
public:
	BC_Repeater(BC_WindowBase *top_level, long delay);
	~BC_Repeater();

	void initialize();
	int start_repeating();
	int stop_repeating();
	void run();

	long repeat_id;
	long delay;
	int repeating;
	int interrupted;
// Prevent new signal until existing event is processed
	Condition *repeat_lock;

private:
	Timer timer;
	BC_WindowBase *top_level;
// Delay corrected for the time the last repeat took
	long next_delay;
	Condition *pause_lock;
	Condition *startup_lock;
};



#endif
