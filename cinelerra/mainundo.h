#ifndef MAINUNDO_H
#define MAINUNDO_H


#include "filexml.inc"
#include "mwindow.inc"
#include "undostack.h"

#include <stdint.h>

class MainUndo
{
public:
	MainUndo(MWindow *mwindow);
	~MainUndo();

	void update_undo_before(char *description, uint32_t load_flags);
	void update_undo_after();


	int undo();
	int redo();

private:
	int load_from_undo(FileXML *file, uint32_t load_flags);    // loads undo from the stringfile to the project

	UndoStack undo_stack;
	UndoStackItem* current_entry; // for setting the after buffer
	MWindow *mwindow;
	int undo_before_updated;
};

#endif
