// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MAINUNDO_H
#define MAINUNDO_H

#include "bctimer.inc"
#include "linklist.h"
#include "filexml.inc"
#include "mwindow.inc"
#include "undostackitem.h"

#include <stdint.h>


class UndoStackItem;
class MainUndoStackItem;


class MainUndo
{
public:
	MainUndo();
	~MainUndo();

// Use this function for UndoStackItem subclasses with custom
// undo and redo functions.  All fields including description must
// be populated before calling this function.
	void push_undo_item(UndoStackItem *item);

	void update_undo(const char *description, 
		uint32_t load_flags, 
		void *creator = 0,
		int changes_made = 1);

// alternatively, call this one after the change
	void push_state(const char *description, uint32_t load_flags, void* creator);

// Used in undo and redo to reset the creators in all the records.
	void reset_creators();

	void undo();
	void redo();

private:
	List<UndoStackItem> undo_stack;
	List<UndoStackItem> redo_stack;
	MainUndoStackItem* new_entry;	// for setting the after buffer

	Timer *last_update;
	char* data_after;	// the state after a change

	void capture_state();
	void prune_undo();
	bool ignore_push(const char *description, uint32_t load_flags, void* creator);

	friend class MainUndoStackItem;
};


class MainUndoStackItem : public UndoStackItem
{
public:
	MainUndoStackItem(MainUndo* undo, const char* description,
		uint32_t load_flags, void* creator);
	virtual ~MainUndoStackItem();

	void set_data_before(const char *data);
	virtual void undo();
	virtual size_t get_size();

private:
// type of modification
	unsigned long load_flags;

// data before the modification for undos
	char *data_before;

	MainUndo *main_undo;

// loads undo from the stringfile to the project
	void load_from_undo(FileXML *file, uint32_t load_flags);
};

#endif
