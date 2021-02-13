// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctimer.h"
#include "edl.h"
#include "edlsession.inc"
#include "filexml.h"
#include "mainindexes.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include <string.h>

// Minimum number of undoable operations on the undo stack
#define UNDOMINLEVELS 5
// Limits the bytes of memory used by the undo stack
#define UNDOMEMORY 50000000

MainUndo::MainUndo()
{
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

void MainUndo::update_undo(const char *description, uint32_t load_flags, 
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

	mainsession->changes_made = 1;
	mwindow_global->update_undo_text(item->description);
	mwindow_global->update_redo_text(0);
}

void MainUndo::capture_state()
{
	FileXML file;
	master_edl->save_xml(&file, "", 0, 0);
	delete [] data_after;
	data_after = new char[strlen(file.string)+1];
	strcpy(data_after, file.string);
}

bool MainUndo::ignore_push(const char *description, uint32_t load_flags, void* creator)
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

void MainUndo::push_state(const char *description, uint32_t load_flags, void* creator)
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
	mainsession->changes_made = 1;
}

void MainUndo::undo()
{
	UndoStackItem* current_entry = undo_stack.last;

	if(current_entry)
	{
// move item to redo_stack
		undo_stack.remove_pointer(current_entry);
		current_entry->undo();
		redo_stack.append(current_entry);
		capture_state();

		if(mwindow_global)
		{
			mwindow_global->update_redo_text(current_entry->description);

			if(undo_stack.last)
				mwindow_global->update_undo_text(undo_stack.last->description);
			else
				mwindow_global->update_undo_text(0);
		}
	}
	reset_creators();
}

void MainUndo::redo()
{
	UndoStackItem* current_entry = redo_stack.last;

	if(current_entry)
	{
// move item to undo_stack
		redo_stack.remove_pointer(current_entry);
		current_entry->undo();
		undo_stack.append(current_entry);
		capture_state();

		if(mwindow_global)
		{
			mwindow_global->update_undo_text(current_entry->description);

			if(redo_stack.last)
				mwindow_global->update_redo_text(redo_stack.last->description);
			else
				mwindow_global->update_redo_text(0);
		}
	}
	reset_creators();
}

// enforces that the undo stack does not exceed a size of UNDOMEMORY
// except that it always has at least UNDOMINLEVELS entries
void MainUndo::prune_undo()
{
	size_t size = 0;
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


MainUndoStackItem::MainUndoStackItem(MainUndo* main_undo, const char* description,
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

void MainUndoStackItem::set_data_before(const char *data)
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

size_t MainUndoStackItem::get_size()
{
	size_t size = sizeof(*this);

	if(data_before)
		size += strlen(data_before);
	return size;
}

// Here the master EDL loads 
void MainUndoStackItem::load_from_undo(FileXML *file, uint32_t load_flags)
{
	master_edl->load_xml(file, edlsession);
	for(int i = 0; i < master_edl->assets->total; i++)
	{
		mwindow_global->mainindexes->add_next_asset(master_edl->assets->values[i]);
	}
	mwindow_global->mainindexes->start_build();
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
