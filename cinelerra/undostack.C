#include "stringfile.h"
#include "undostack.h"
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
#include <string.h>

UndoStack::UndoStack() : List<UndoStackItem>()
{
	current = 0;
   size = 0;
}

UndoStack::~UndoStack()
{
}

void UndoStack::push(UndoStackItem *item)
{
// current is only 0 if before first undo
	if(current)
		current = insert_after(current, item);
	else
		current = insert_before(first, item);
	
// delete future undos if necessary
	if(current && current->next)
	{
		while(current->next) 
      {
         size -= last->get_size();
         remove(last);
      }
	}

// delete oldest undo if necessary
   size += item->get_size();
	if(total() > UNDOLEVELS || size > UNDOMEMORY) 
   {
      size -= first->get_size();
      remove(first);
   }
}

UndoStackItem* UndoStack::push()
{
// current is only 0 if before first undo
	if(current)
		current = insert_after(current);
	else
		current = insert_before(first);
	
// delete future undos if necessary
	if(current && current->next)
	{
		while(current->next) 
      {
         size -= last->get_size();
         remove(last);
      }
	}

// delete oldest undo if necessary
	if(total() > UNDOLEVELS || size > UNDOMEMORY) 
   {
      size -= first->get_size();
      remove(first);
   }
	
	return current;
}

int UndoStack::pull()
{
	if(current) current = PREVIOUS;
}

void UndoStack::update_size()
{
   size += current->get_size();
// delete oldest undo if necessary
	if(total() > UNDOLEVELS || size > UNDOMEMORY) 
   {
      size -= first->get_size();
      remove(first);
   }
}

UndoStackItem* UndoStack::pull_next()
{
// use first entry if none
	if(!current) current = first;
	else
// use next entry if there is a next entry
	if(current->next)
		current = NEXT;
// don't change current if there is no next entry
	else
		return 0;
		
	return current;
}







UndoStackItem::UndoStackItem() : ListItem<UndoStackItem>()
{
	description = type = data_after = data_before = 0;
}

UndoStackItem::~UndoStackItem()
{
	if(description) delete [] description;
	if(type) delete [] type;
	if(data_after) delete [] data_after;
	if(data_before) delete [] data_before;
}

int UndoStackItem::set_description(char *description)
{
	this->description = new char[strlen(description) + 1];
	strcpy(this->description, description);
}

int UndoStackItem::set_type(char *type)
{
	this->type = new char[strlen(type) + 1];
	strcpy(this->type, type);
}

int UndoStackItem::set_data_before(char *data)
{
	this->data_before = new char[strlen(data) + 1];
	strcpy(this->data_before, data);
}

int UndoStackItem::set_data_after(char *data)
{
	this->data_after = new char[strlen(data) + 1];
	strcpy(this->data_after, data);
}

void UndoStackItem::undo()
{
		FileXML file;
		
		file.read_from_string(data_before);
		load_from_undo(&file, load_flags);
}

void UndoStackItem::redo()
{
		FileXML file;
		file.read_from_string(data_after);
		load_from_undo(&file, load_flags);
}

int UndoStackItem::get_size()
{
   return strlen(data_before) + strlen(data_after);
}

void UndoStackItem::set_mwindow(MWindow *mwindow)
{
   this->mwindow = mwindow;
}

// Here the master EDL loads 
int UndoStackItem::load_from_undo(FileXML *file, uint32_t load_flags)
{
	mwindow->edl->load_xml(mwindow->plugindb, file, load_flags);
	for(Asset *asset = mwindow->edl->assets->first;
		asset;
		asset = asset->next)
	{
		mwindow->mainindexes->add_next_asset(asset);
	}
	mwindow->mainindexes->start_build();
	return 0;
}
