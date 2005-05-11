#include "undostack.h"
#include "undostackitem.h"


UndoStack::UndoStack() : List<UndoStackItem>()
{
	current = 0;
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
		while(current->next) remove(last);
	}
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

// enforces that the undo stack does not exceed a size of UNDOMEMORY
// except that it always has at least UNDOMINLEVELS entries
void UndoStack::prune()
{
	int size = 0;
	int levels = 0;

	UndoStackItem* i = last;
	while (i != 0 && (levels < UNDOMINLEVELS || size <= UNDOMEMORY))
	{
		size += i->get_size();
		++levels;
		i = i->previous;
	}

	if (i != 0)
	{
// truncate everything before and including i
		while (first != i)
			remove(first);
		remove(first);
	}
}
