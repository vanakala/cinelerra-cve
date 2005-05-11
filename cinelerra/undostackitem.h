#ifndef UNDOSTACKITEM_H
#define UNDOSTACKITEM_H

#include "linklist.h"


class UndoStackItem : public ListItem<UndoStackItem>
{
public:
	UndoStackItem();
	virtual ~UndoStackItem();

	void set_description(char *description);

// These two virtual functions allow derived objects to be pushed
// on to the undo stack that are more efficient in time and memory
// usage than the default action of saving before and after copies
// of the EDL.  
	virtual void undo();
	virtual void redo();

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
