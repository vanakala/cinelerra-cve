// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef UNDOSTACKITEM_H
#define UNDOSTACKITEM_H

#include "linklist.h"
#include <stddef.h>

#define UNDO_DESCLEN 80

class UndoStackItem : public ListItem<UndoStackItem>
{
public:
	UndoStackItem();

	void set_description(const char *description);
	void set_creator(void *creator);

// This function must be overridden in derived objects.
// It is called both to undo and to redo an operation.
// The override must do the following:
// - change the EDL to undo an operation;
// - change the internal information of the item so that the next invocation
//   of undo() does a redo (i.e. undoes the undone operation).
	virtual void undo() {};

// Return the amount of memory used for the data associated with this
// object in order to enable limiting the amount of memory used by the
// undo stack.
// Ignore overhead and just report the specific data values that the
// derived object adds.
	virtual size_t get_size() { return 0; };

// command description for the menu item
	char description[UNDO_DESCLEN];
// who created this item
	void* creator;
};

#endif
