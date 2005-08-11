#ifndef MAINUNDO_H
#define MAINUNDO_H


#include "linklist.h"
#include "mwindow.inc"

#include <stdint.h>


class UndoStackItem;
class MainUndoStackItem;


class MainUndo
{
public:
	MainUndo(MWindow *mwindow);
	~MainUndo();

   // Use this function for UndoStackItem subclasses with custom
   // undo and redo functions.  All fields including description must
   // be populated before calling this function.
   void push_undo_item(UndoStackItem *item);

   // Use the following functions for the default save/restore undo method
	void update_undo_before(char *description, uint32_t load_flags);
	void update_undo_after();

	int undo();
	int redo();

private:
	List<UndoStackItem> undo_stack;
	List<UndoStackItem> redo_stack;
	MainUndoStackItem* new_entry;	// for setting the after buffer
	MWindow *mwindow;
	int undo_before_updated;

	void prune_undo();
};

#endif
