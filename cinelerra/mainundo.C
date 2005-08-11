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
#include <string.h>

// Minimum number of undoable operations on the undo stack
#define UNDOMINLEVELS 5
// Limits the bytes of memory used by the undo stack
#define UNDOMEMORY 50000000


class MainUndoStackItem : public UndoStackItem
{
public:
	MainUndoStackItem(MWindow* mwindow, char* description);
	virtual ~MainUndoStackItem();

	void set_data_before(char *data, uint32_t load_flags);
	void set_data_after(char *data);
	virtual void undo();
	virtual void redo();
	virtual int get_size();

private:
// type of modification
	unsigned long load_flags;
	
// data after the modification for redos
	char *data_after;          
	
// data before the modification for undos
	char *data_before;          

	MWindow *mwindow;

	void load_from_undo(FileXML *file, uint32_t load_flags);	// loads undo from the stringfile to the project
};


MainUndo::MainUndo(MWindow *mwindow)
{ 
	this->mwindow = mwindow;
	undo_before_updated = 0;
}

MainUndo::~MainUndo()
{
}

void MainUndo::push_undo_item(UndoStackItem *item)
{
// clear redo_stack
	while (redo_stack.last)
		redo_stack.remove(redo_stack.last);

// move item onto undo_stack
	undo_stack.append(item);
	prune_undo();

	mwindow->session->changes_made = 1;
   mwindow->gui->lock_window("MainUndo::update_undo_before");
   mwindow->gui->mainmenu->undo->update_caption(item->description);
   mwindow->gui->mainmenu->redo->update_caption("");
   mwindow->gui->unlock_window();
}

void MainUndo::update_undo_before(char *description, uint32_t load_flags)
{
	if(!undo_before_updated)
	{
		FileXML file;
		mwindow->edl->save_xml(mwindow->plugindb, 
			&file, 
			"",
			0,
			0);
		file.terminate_string();

		new_entry = new MainUndoStackItem(mwindow, description);
		new_entry->set_data_before(file.string, load_flags);

		undo_before_updated = 1;
	}
}

void MainUndo::update_undo_after()
{
	if(undo_before_updated)
	{
		FileXML file;
//printf("MainUndo::update_undo_after 1\n");
		mwindow->edl->save_xml(mwindow->plugindb, 
			&file, 
			"",
			0,
			0);
//printf("MainUndo::update_undo_after 1\n");
		file.terminate_string();
//printf("MainUndo::update_undo_after 1\n");
		new_entry->set_data_after(file.string);
		push_undo_item(new_entry);

//printf("MainUndo::update_undo_after 10\n");
		undo_before_updated = 0;
	}
}






int MainUndo::undo()
{
	if(undo_stack.last)
	{
		UndoStackItem* current_entry = undo_stack.last;

// move item to redo_stack
		undo_stack.remove_pointer(current_entry);
		redo_stack.append(current_entry);

		if(current_entry->description && mwindow->gui) 
			mwindow->gui->mainmenu->redo->update_caption(current_entry->description);

      current_entry->undo();

		if(mwindow->gui)
		{
			if(undo_stack.last)
				mwindow->gui->mainmenu->undo->update_caption(undo_stack.last->description);
			else
				mwindow->gui->mainmenu->undo->update_caption("");
		}
	}
	return 0;
}

int MainUndo::redo()
{
	UndoStackItem* current_entry = redo_stack.last;
	
	if(current_entry)
	{

// move item to undo_stack
		redo_stack.remove_pointer(current_entry);
		undo_stack.append(current_entry);

      current_entry->redo();

		if(mwindow->gui)
		{
			mwindow->gui->mainmenu->undo->update_caption(current_entry->description);
			
			if(redo_stack.last)
				mwindow->gui->mainmenu->redo->update_caption(redo_stack.last->description);
			else
				mwindow->gui->mainmenu->redo->update_caption("");
		}
	}
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





MainUndoStackItem::MainUndoStackItem(MWindow* mwindow, char* description)
{
	data_after = data_before = 0;
	load_flags = 0;
	this->mwindow = mwindow;
	set_description(description);
}

MainUndoStackItem::~MainUndoStackItem()
{
	delete [] data_after;
	delete [] data_before;
}

void MainUndoStackItem::set_data_before(char *data, uint32_t load_flags)
{
	this->data_before = new char[strlen(data) + 1];
	strcpy(this->data_before, data);
	this->load_flags = load_flags;
}

void MainUndoStackItem::set_data_after(char *data)
{
	this->data_after = new char[strlen(data) + 1];
	strcpy(this->data_after, data);
}

void MainUndoStackItem::undo()
{
	FileXML file;

	file.read_from_string(data_before);
	load_from_undo(&file, load_flags);
}

void MainUndoStackItem::redo()
{
	FileXML file;
	file.read_from_string(data_after);
	load_from_undo(&file, load_flags);
}

int MainUndoStackItem::get_size()
{
	return strlen(data_before) + strlen(data_after);
}

// Here the master EDL loads 
void MainUndoStackItem::load_from_undo(FileXML *file, uint32_t load_flags)
{
	mwindow->edl->load_xml(mwindow->plugindb, file, load_flags);
	for(Asset *asset = mwindow->edl->assets->first;
		asset;
		asset = asset->next)
	{
		mwindow->mainindexes->add_next_asset(asset);
	}
	mwindow->mainindexes->start_build();
}




