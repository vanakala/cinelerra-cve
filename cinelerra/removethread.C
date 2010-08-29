
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

#include "bcwindowbase.inc"
#include "bcsignals.h"
#include "condition.h"
#include "mutex.h"
#include "removethread.h"

#include <string.h>

RemoveThread::RemoveThread()
 : Thread()
{
	input_lock = new Condition(0, "RemoveThread::input_lock", 0);
	file_lock = new Mutex("RemoveThread::file_lock");
}

void RemoveThread::remove_file(const char *path)
{
// Rename to temporary
	char string[BCTEXTLEN];
	sprintf(string, "%s.%05d", path, number++);
	rename(path, string);

	file_lock->lock("RemoveThread::remove_file");
	files.append(strdup(string));
	file_lock->unlock();
	input_lock->unlock();
	if(number > 100000)
		number %= 100000;
}

void RemoveThread::create_objects()
{
	Thread::start();
}

void RemoveThread::run()
{
	number = (int)(get_tid() % 100000);
	while(1)
	{
		char string[BCTEXTLEN];
		string[0] = 0;
		input_lock->lock("RemoveThread::run");
		file_lock->lock("RemoveThread::remove_file");
		if(files.total)
		{
			strcpy(string, files.values[0]);
			files.remove_object_number(0);
		}
		file_lock->unlock();
		if(string[0])
		{
			remove(string);
		}
	}
}
