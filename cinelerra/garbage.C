
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

#include "bcsignals.h"
#include "garbage.h"
#include "mutex.h"

#include <string.h>

Garbage *Garbage::garbage = 0;


GarbageObject::GarbageObject(const char *title)
{
	Garbage::garbage->add_object(this);
	users = 0;
	deleted = 0;
	this->title = new char[strlen(title) + 1];
	strcpy(this->title, title);
}

GarbageObject::~GarbageObject()
{
	if(!deleted) 
		printf("GarbageObject::~GarbageObject: title=%s users=%d was not deleted by Garbage::delete_object\n", title, users);
	delete [] title;
}


void GarbageObject::add_user()
{
	Garbage::garbage->lock->lock("GarbageObject::add_user");
	users++;
	Garbage::garbage->lock->unlock();
}

void GarbageObject::remove_user()
{
	Garbage::garbage->lock->lock("GarbageObject::add_user");
	users--;
	if(users < 0) printf("GarbageObject::remove_user: users=%d Should be >= 0.\n", users);
	Garbage::garbage->remove_expired();
// *this is now invalid
	Garbage::garbage->lock->unlock();
}





Garbage::Garbage()
{
	lock = new Mutex("Garbage::lock", 1);
}


Garbage::~Garbage()
{
	delete lock;
}

void Garbage::add_object(GarbageObject *ptr)
{
	lock->lock("Garbage::add_object");
	objects.append(ptr);
	lock->unlock();
}

void Garbage::delete_object(GarbageObject *ptr)
{
	Garbage *garbage = Garbage::garbage;
	garbage->lock->lock("Garbage::delete_object");
	ptr->deleted = 1;

// Remove expired objects here
	remove_expired();
	garbage->lock->unlock();
}

void Garbage::remove_expired()
{
	Garbage *garbage = Garbage::garbage;
	for(int i = 0; i < garbage->objects.total; i++)
	{
		GarbageObject *ptr = garbage->objects.values[i];
		if(ptr->users <= 0 && ptr->deleted)
		{
// Must remove pointer to prevent recursive deletion of the same object.
// But i is still invalid.
			garbage->objects.remove_number(i);

			delete ptr;
			i--;
		}
	}
}



