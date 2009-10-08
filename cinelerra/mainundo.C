
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

#include "asset.h"
#include "assets.h"
#include "edl.h"
#include "filexml.h"
#include "mainindexes.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "undostackitem.h"
#include "tracks.h"
#include <string.h>

// Minimum number of undoable operations on the undo stack
#define UNDOMINLEVELS 5
// Limits the bytes of memory used by the undo stack
#define UNDOMEMORY 50000000


class MainUndoStackItem : public UndoStackItem
{
public:
	MainUndoStackItem(MainUndo* undo, char* description,
			uint32_t load_flags, void* creator);
	virtual ~MainUndoStackItem();

	void set_data_before(char *data);
	virtual void undo();
	virtual int get_size();

private:
// type of modification
	unsigned long load_flags;
	
// data before the modification for undos
	char *data_before;          

	MainUndo *main_undo;

	void load_from_undo(FileXML *file, uint32_t load_flags);	// loads undo from the stringfile to the project
};


MainUndo::MainUndo(MWindow *mwindow)
{ 
	this->mwindow = mwindow;
	new_entry = 0;
	data_after = 0;
	last_update = new Timer;

// get the initial project so we have something that the last undo reverts to
	capture_state();
}

MainUndo::~MainUndo()
{
	delete [] data_after;
	delete last_update;
}

void MainUndo::update_undo(char *description, uint32_t load_flags, 
		void *creator, int changes_made)
{
	if (ignore_push(description, load_flags, creator))
	{
		capture_state();
		return;
	}

	MainUndoStackItem* new_entry = new MainUndoStackItem(this, description, load_flags, creator);

// the old data_after is the state before the change
	new_entry->set_data_before(data_after);

	push_undo_item(new_entry);
}

void MainUndo::push_undo_item(UndoStackItem *item)
{
// clear redo_stack
	while (redo_stack.last)
		redo_stack.remove(redo_stack.last);

// move item onto undo_stack
	undo_stack.append(item);
	prune_undo();

	capture_state();

	mwindow->session->changes_made = 1;
   mwindow->gui->lock_window("MainUndo::update_undo_before");
   mwindow->gui->mainmenu->undo->update_caption(item->description);
   mwindow->gui->mainmenu->redo->update_caption("");
   mwindow->gui->unlock_window();
}

void MainUndo::capture_state()
{
	FileXML file;
	mwindow->edl->save_xml(mwindow->plugindb, 
		&file, 
		"",
		0,
		0);
	file.terminate_string();
	delete [] data_after;
	data_after = new char[strlen(file.string)+1];
	strcpy(data_after, file.string);
}

bool MainUndo::ignore_push(char *description, uint32_t load_flags, void* creator)
{
// ignore this push under certain conditions:
// - if nothing was undone
	bool ignore = redo_stack.last == 0 &&
// - if it is not the first push
		undo_stack.last &&
// - if it has the same description as the previous undo
		strcmp(undo_stack.last->description, description) == 0 &&
// - if it originates from the same creator
		undo_stack.last->creator == creator &&
// - if it follows closely after the previous undo
		last_update->get_difference() < 300 /*millisec*/;
	last_update->update();
	return ignore;
}

void MainUndo::push_state(char *description, uint32_t load_flags, void* creator)
{
	if (ignore_push(description, load_flags, creator))
	{
		capture_state();
	}
	else
	{
		MainUndoStackItem* new_entry = new MainUndoStackItem(this, description, load_flags, creator);
// the old data_after is the state before the change
		new_entry->set_data_before(data_after);
		push_undo_item(new_entry);
	}
	mwindow->session->changes_made = 1;
}






int MainUndo::undo()
{
	UndoStackItem* current_entry = undo_stack.last;

	if(current_entry)
	{
// move item to redo_stack
		undo_stack.remove_pointer(current_entry);
		current_entry->undo();
		redo_stack.append(current_entry);
		capture_state();

		if(mwindow->gui)
		{
			mwindow->gui->mainmenu->redo->update_caption(current_entry->description);

			if(undo_stack.last)
				mwindow->gui->mainmenu->undo->update_caption(undo_stack.last->description);
			else
				mwindow->gui->mainmenu->undo->update_caption("");
		}
	}

	reset_creators();
	return 0;
}

int MainUndo::redo()
{
	UndoStackItem* current_entry = redo_stack.last;
	
	if(current_entry)
	{
// move item to undo_stack
		redo_stack.remove_pointer(current_entry);
		current_entry->undo();
		undo_stack.append(current_entry);
		capture_state();

		if(mwindow->gui)
		{
			mwindow->gui->mainmenu->undo->update_caption(current_entry->description);
			
			if(redo_stack.last)
				mwindow->gui->mainmenu->redo->update_caption(redo_stack.last->description);
			else
				mwindow->gui->mainmenu->redo->update_caption("");
		}
	}
	reset_creators();
	return 0;
}

// enforces that the undo stack does not exceed a size of UNDOMEMORY
// except that it always has at least UNDOMINLEVELS entries
void MainUndo::prune_undo()
{
	int size = 0;
	int levels = 0;

	UndoStackItem* i = undo_stack.last;
	while (i != 0 && (levels < UNDOMINLEVELS || size <= UNDOMEMORY))
	{
		size += i->get_size();
		++levels;
		i = i->previous;
	}

	if (i != 0)
	{
// truncate everything before and including i
		while (undo_stack.first != i)
			undo_stack.remove(undo_stack.first);
		undo_stack.remove(undo_stack.first);
	}
}





MainUndoStackItem::MainUndoStackItem(MainUndo* main_undo, char* description,
			uint32_t load_flags, void* creator)
{
	data_before = 0;
	this->load_flags = load_flags;
	this->main_undo = main_undo;
	set_description(description);
	set_creator(creator);
}

MainUndoStackItem::~MainUndoStackItem()
{
	delete [] data_before;
}

void MainUndoStackItem::set_data_before(char *data)
{
	data_before = new char[strlen(data) + 1];
	strcpy(data_before, data);
}

void MainUndoStackItem::undo()
{
// move the old data_after here
	char* before = data_before;
	data_before = 0;
	set_data_before(main_undo->data_after);

// undo the state
	FileXML file;

	file.read_from_string(before);
	load_from_undo(&file, load_flags);
}

int MainUndoStackItem::get_size()
{
	return data_before ? strlen(data_before) : 0;
}

// Here the master EDL loads 
void MainUndoStackItem::load_from_undo(FileXML *file, uint32_t load_flags)
{
	MWindow* mwindow = main_undo->mwindow;
	mwindow->edl->load_xml(mwindow->plugindb, file, load_flags);
	for(Asset *asset = mwindow->edl->assets->first;
		asset;
		asset = asset->next)
	{
		mwindow->mainindexes->add_next_asset(0, asset);
	}
	mwindow->mainindexes->start_build();
}


void MainUndo::reset_creators()
{
	for(UndoStackItem *current = undo_stack.first;
		current;
		current = NEXT)
	{
		current->set_creator(0);
	}
}



