#include "stringfile.h"
#include "undostack.h"
#include <string.h>

UndoStack::UndoStack() : List<UndoStackItem>()
{
	current = 0;
}

UndoStack::~UndoStack()
{
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
		while(current->next) remove(last);
	}

// delete oldest undo if necessary
	if(total() > UNDOLEVELS) remove(first);
	
	return current;
}

int UndoStack::pull()
{
	if(current) current = PREVIOUS;
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
	if(description) delete description;
	if(type) delete type;
	if(data_after) delete data_after;
	if(data_before) delete data_before;
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
