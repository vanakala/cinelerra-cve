#include "undostackitem.h"
#include <string.h>



UndoStackItem::UndoStackItem() : ListItem<UndoStackItem>()
{
	description = 0;
	creator = 0;
}

UndoStackItem::~UndoStackItem()
{
	delete [] description;
}

void UndoStackItem::set_description(char *description)
{
	this->description = new char[strlen(description) + 1];
	strcpy(this->description, description);
}

void UndoStackItem::set_creator(void *creator)
{
	this->creator = creator;
}

void UndoStackItem::undo()
{
}

int UndoStackItem::get_size()
{
	return 0;
}
