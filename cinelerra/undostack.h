#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include "linklist.h"
#include "stringfile.inc"

#define UNDOLEVELS 500

class UndoStackItem : public ListItem<UndoStackItem>
{
public:
	UndoStackItem();
	~UndoStackItem();

	int set_data_before(char *data);
	int set_data_after(char *data);
	int set_type(char *type);
	int set_description(char *description);
	
	
// command description for the menu item
	char *description;

// type of modification
	unsigned long load_flags;
	char *type;            
	
// data after the modification for redos
	char *data_after;          
	
// data before the modification for undos
	char *data_before;          
};

class UndoStack : public List<UndoStackItem>
{
public:
	UndoStack();
	~UndoStack();
	
// create a new undo entry
// delete future undos if in the middle
// delete undos older than UNDOLEVELS if last
	UndoStackItem* push();

// move to the previous undo entry
	int pull();


// move to the next undo entry for a redo
	UndoStackItem* pull_next();
	
	UndoStackItem* current;
};

#endif
