
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

#ifndef LINKLIST_H
#define LINKLIST_H

template<class TYPE>
class ListItem;

template<class TYPE>
class List                        // inherited by lists
{
public:
	List();
	virtual ~List();
// delete the item and the pointers to it
	void remove(TYPE *item);   
// remove the pointers to the item only
	void remove_pointer(ListItem<TYPE> *item);  

// these must be used to add an item to a list
	TYPE *append();  // create new node and return pointer of it
	TYPE *append(TYPE *new_item);   // append the new pointer to the list
	TYPE *insert_before(TYPE *item);  // create new node and return pointer of it
	TYPE *insert_before(TYPE *item, TYPE *new_item);  // create new node and return pointer of it
	TYPE *insert_after(TYPE *item);
	TYPE *insert_after(TYPE *item, TYPE *new_item);
	TYPE *get_item_number(int number);
	int get_item_number(TYPE *item);
	void swap(TYPE *item1, TYPE *item2);

// query the list
	int total();     // total number of nodes
	int number_of(TYPE *item);

// convenience macros
#define PREVIOUS current->previous
#define NEXT current->next

// references to list
	TYPE *first;
	TYPE *last;
};

template<class TYPE>
class ListItem                        // inherited by list items
{
public:
	ListItem();
	virtual ~ListItem();

	int get_item_number();
	TYPE *previous;
	TYPE *next;
	List<TYPE> *owner;             // list that owns this item for deletion
};

template<class TYPE>
List<TYPE>::List()
{
	last = first = 0;
}

template<class TYPE>
List<TYPE>::~List()     // delete nodes
{
	while(last)
	{
		delete last;
	}
}

template<class TYPE>
int List<TYPE>::total()     // total number of nodes
{
	int total = 0;
	TYPE* current;
	
	for(current = first; current; current = NEXT)
	{
		total++;
	}
	return total;
}

template<class TYPE>
int List<TYPE>::number_of(TYPE *item)
{
	TYPE* current;
	int total = 0;
	for(current = first; current; current = NEXT)
	{
		if(current == item) return total;
		total++;
	}
	return 0;
}

template<class TYPE>
TYPE* List<TYPE>::get_item_number(int number)
{
	TYPE* current;
	int i;
	for(i = 0, current = first; current && i < number; current = NEXT, i++)
	{
		;
	}
	return current;
}

template<class TYPE>
int List<TYPE>::get_item_number(TYPE *item)
{
	TYPE* current;
	int i;
	for(i = 0, current = first; current && current != item; current = NEXT, i++)
	{
		;
	}
	return i;
}


template<class TYPE>
TYPE* List<TYPE>::append()
{
	TYPE* current_item;

	if(!last)        // add first node
	{
		current_item = last = first = new TYPE;
		current_item->previous = current_item->next = 0;
		current_item->owner = this;
		return current_item;
	}
	else
	{                // append node
		current_item = last->next = new TYPE;
		current_item->previous = last;
		current_item->next = 0;
		last = current_item;
		current_item->owner = this;
		return current_item;
	}
}

template<class TYPE>
TYPE* List<TYPE>::append(TYPE *new_item)
{
	TYPE* current_item;
	
	if(!last)        // add first node
	{
		current_item = last = first = new_item;
		current_item->previous = current_item->next = 0;
		current_item->owner = this;
		return current_item;
	}
	else
	{                // append node
		current_item = last->next = new_item;
		current_item->previous = last;
		current_item->next = 0;
		last = current_item;
		current_item->owner = this;
		return current_item;
	}
}

template<class TYPE>
TYPE* List<TYPE>::insert_before(TYPE *item)
{
	TYPE* new_item = new TYPE;
	return insert_before(item, new_item);
}

template<class TYPE>
TYPE* List<TYPE>::insert_before(TYPE *item, TYPE *new_item)
{
	if(!item) return append(new_item);      // if item is null, append

	TYPE* current_item = new_item;

	if(item == first) first = current_item;   // set *first

	current_item->previous = item->previous;       // set this node's pointers
	current_item->next = item;
	
	if(current_item->previous) current_item->previous->next = current_item;         // set previous node's pointers

	if(current_item->next) current_item->next->previous = current_item;        // set next node's pointers
	
	current_item->owner = this;
	return current_item;
}

template<class TYPE>
TYPE* List<TYPE>::insert_after(TYPE *item)
{
	TYPE *new_item = new TYPE;
	return insert_after(item, new_item);
}

template<class TYPE>
TYPE* List<TYPE>::insert_after(TYPE *item, TYPE *new_item)
{
	if(!item) return append(new_item);      // if item is null, append

	TYPE* current_item = new_item;

	if(item == last) last = current_item;   // set *last

	current_item->previous = item;       // set this node's pointers
	current_item->next = item->next;
	
	if(current_item->previous) current_item->previous->next = current_item;         // set previous node's pointers

	if(current_item->next) current_item->next->previous = current_item;        // set next node's pointers
	
	current_item->owner = this;
	return current_item;
}

template<class TYPE>
void List<TYPE>::swap(TYPE *item1, TYPE *item2)
{
	TYPE **temp_array;
	int total = this->total();
	temp_array = new TYPE*[total];
// Copy entire list to a temp.  This rewrites the pointers in the original items.
	for(int i = 0; i < total; i++)
	{
		temp_array[i] = get_item_number(i);
	}
	while(last) remove_pointer(last);
	for(int i = 0; i < total; i++)
	{
		if(temp_array[i] == item1) append(item2);
		else
		if(temp_array[i] == item2) append(item1);
		else
			append(temp_array[i]);
	}
	delete [] temp_array;

#if 0
	TYPE *new_item0, *new_item1, *new_item2, *new_item3;

// old == item0 item1 item2 item3
// new == item0 item2 item1 item3

	new_item0 = item1->previous;
	new_item1 = item2;
	new_item2 = item1;
	new_item3 = item2->next;

	if(new_item0) 
		new_item0->next = new_item1;
	else
		first = new_item1;

	if(new_item1) new_item1->next = new_item2;
	if(new_item2) new_item2->next = new_item3;
	if(new_item3)
		new_item3->previous = new_item2;
	else
		last = new_item2;

	if(new_item2) new_item2->previous = new_item1;
	if(new_item1) new_item1->previous = new_item0;
#endif
}

template<class TYPE>
void List<TYPE>::remove(TYPE *item)
{
	if(!item) return;
	delete item;                        // item calls back to remove pointers
}

template<class TYPE>
void List<TYPE>::remove_pointer(ListItem<TYPE> *item)
{
//printf("List<TYPE>::remove_pointer %x %x %x\n", item, last, first);
	if(!item) return;

	item->owner = 0;

	if(item == last && item == first)
	{
// last item
		last = first = 0;
		return;
	}

	if(item == last) last = item->previous;      // set *last and *first
	else
	if(item == first) first = item->next;

	if(item->previous) item->previous->next = item->next;         // set previous node's pointers

	if(item->next) item->next->previous = item->previous;       // set next node's pointers
}

template<class TYPE>
ListItem<TYPE>::ListItem()
{
	next = previous = 0;
// don't delete the pointer to this if it's not part of a list
	owner = 0;
}

template<class TYPE>
ListItem<TYPE>::~ListItem()
{
// don't delete the pointer to this if it's not part of a list
	if(owner) owner->remove_pointer(this);
}

template<class TYPE>
int ListItem<TYPE>::get_item_number()
{
	return owner->get_item_number((TYPE*)this);
}

#endif
