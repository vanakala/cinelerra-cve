#ifndef UNDOSTACKITEM_H
#define UNDOSTACKITEM_H

#include "linklist.h"


class UndoStackItem : public ListItem<UndoStackItem>
{
public:
	UndoStackItem();
	virtual ~UndoStackItem();

	void set_description(char *description);

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
};

#endif
