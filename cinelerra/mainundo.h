#ifndef MAINUNDO_H
#define MAINUNDO_H


#include "bctimer.h"
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

// alternatively, call this one after the change
	void push_state(char *description, uint32_t load_flags);

	int undo();
	int redo();

private:
	List<UndoStackItem> undo_stack;
	List<UndoStackItem> redo_stack;
	MainUndoStackItem* new_entry;	// for setting the after buffer
	Timer timestamp;	// time of last undo
	MWindow *mwindow;
	char* data_after;	// the state after a change

	void capture_state();
	void prune_undo();

	friend class MainUndoStackItem;
};

#endif
