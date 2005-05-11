#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include "linklist.h"

// Minimum number of undoable operations on the undo stack
#define UNDOMINLEVELS 5
// Limits the bytes of memory used by the undo stack
#define UNDOMEMORY 50000000

class UndoStackItem;


class UndoStack : public List<UndoStackItem>
{
public:
	UndoStack();
	~UndoStack();
	
// delete future undos if in the middle
// delete undos older than UNDOLEVELS if last
   void push(UndoStackItem *item);

// move to the previous undo entry
	int pull();


// move to the next undo entry for a redo
	UndoStackItem* pull_next();
	
	UndoStackItem* current;

	void prune();
};

#endif
