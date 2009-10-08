
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

#ifndef BCTIMER_H
#define BCTIMER_H

#include <stdint.h>
#include <sys/time.h>


class Timer
{
public:
	Timer();
	virtual ~Timer();
	
// set last update to now
	int update();
	
// get difference between now and last update in milliseconds
// must be positive or error results
	int64_t get_difference(struct timeval *result); // also stores in timeval
	int64_t get_difference();

// get difference in arbitrary units between now and last update    
	int64_t get_scaled_difference(long denominator);        
	static int delay(long milliseconds);

private:
	struct timeval current_time;
	struct timeval new_time;
	struct timeval delay_duration;
};



#endif
