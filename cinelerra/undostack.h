#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include "linklist.h"
#include "mwindow.h"
#include "filexml.h"
#include "edl.h"
#include "stringfile.inc"

// Minimum number of undoable operations on the undo stack
#define UNDOMINLEVELS 5
// Limits the bytes of memory used by the undo stack
#define UNDOMEMORY 50000000

class UndoStackItem : public ListItem<UndoStackItem>
{
public:
	UndoStackItem();
	virtual ~UndoStackItem();

	int set_data_before(char *data);
	int set_data_after(char *data);
	int set_type(char *type);
	int set_description(char *description);

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

// type of modification
	unsigned long load_flags;
	char *type;            
	
// data after the modification for redos
	char *data_after;          
	
// data before the modification for undos
	char *data_before;          

   MWindow *mwindow;

   void set_mwindow(MWindow *mwindow);

	int load_from_undo(FileXML *file, uint32_t load_flags);    // loads undo from the stringfile to the project
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
   void push(UndoStackItem *item);

// move to the previous undo entry
	int pull();


// move to the next undo entry for a redo
	UndoStackItem* pull_next();
	
	UndoStackItem* current;

	void prune();
};

#endif
