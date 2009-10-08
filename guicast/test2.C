
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

#include "guicast.h"
#include "vframe.h"
#include <sys/time.h>

class TestWindow : public BC_Window
{
public:
	TestWindow() : BC_Window("test", 
				0,
				0,
				640, 
				480)
	{
	}
};




typedef struct
{
	struct timeval start_time;
	struct timeval last_time;
	int64_t frames;
} status_t;



static void init_status(status_t *status)
{
	gettimeofday(&status->start_time, 0);
	gettimeofday(&status->last_time, 0);
	status->frames = 0;
}

static void update_status(status_t *status)
{
	struct timeval new_time;
	gettimeofday(&new_time, 0);
	if(new_time.tv_sec - status->last_time.tv_sec > 1)
	{
		fprintf(stderr, "%lld frames.  %lld frames/sec      \r", 
			status->frames,
			(int64_t)status->frames / 
			(int64_t)(new_time.tv_sec - status->start_time.tv_sec));
		fflush(stdout);
		status->last_time = new_time;
	};
}

static void stop_status()
{
	fprintf(stderr, "\n\n");
}



int main(int argc, char *argv[])
{
	TestWindow window;
	status_t status;
	
//	int result = window.accel_available(BC_YUV422);
//	printf("accel_available == %d\n", result);
	window.start_video();
	BC_Bitmap *bitmap1 = window.new_bitmap(640, 480, BC_BGR8888);
	
	init_status(&status);
	while(1)
	{
		window.draw_bitmap(bitmap1, 1);
		window.flash();
		update_status(&status);
		status.frames++;
	}
};
