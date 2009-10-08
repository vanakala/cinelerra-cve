
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

#ifndef UNDOSTACKITEM_H
#define UNDOSTACKITEM_H

#include "linklist.h"


class UndoStackItem : public ListItem<UndoStackItem>
{
public:
	UndoStackItem();
	virtual ~UndoStackItem();

	void set_description(char *description);
	void set_creator(void *creator);

// This function must be overridden in derived objects.
// It is called both to undo and to redo an operation.
// The override must do the following:
// - change the EDL to undo an operation;
// - change the internal information of the item so that the next invocation
//   of undo() does a redo (i.e. undoes the undone operation).
	virtual void undo();

// Return the amount of memory used for the data associated with this
// object in order to enable limiting the amount of memory used by the
// undo stack.
// Ignore overhead and just report the specific data values that the
// derived object adds.
	virtual int get_size();
	
	
// command description for the menu item
	char *description;
// who created this item
	void* creator;
};

#endif
