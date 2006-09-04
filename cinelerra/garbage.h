#ifndef GARBAGE_H
#define GARBAGE_H


#include "arraylist.h"
#include "garbage.inc"
#include "mutex.inc"

// Garbage collection
// The objects inherit from GarbageObject.
// The constructor sets users to 0 so the caller must call add_user if it
// wants to use it.  If it doesn't intend to use it after calling the constructor,
// it should not call add_user.
// Other users of the object must call add_user to increment the user count
// and remove_user to decriment the user count.
// The object is only deleted if a call to Garbage::delete_object is made and
// the user count is 0.
// A user who is using it and wants to delete it must first call 
// remove_user and then call Garbage::delete_object.


// They are deleted by calling delete_object.  They get deleted at a
// random point later on.  The objects must not change anything in their
// destructors.

// Can't make graphics elements inherit because they must be deleted
// when the window is locked and they change their parent pointers.

// Elements of link lists must first be unlinked with remove_pointer and then
// passed to delete_object.

// ArrayList objects must be deleted one at a time with delete_object.
// Then the pointers must be deleted with remove_all.
class GarbageObject
{
public:
	GarbageObject(char *title);
	virtual ~GarbageObject();

// Called when user begins to use the object.
	void add_user();
// Called when user is done with the object.
	void remove_user();
	
	int users;
	int deleted;
	char *title;
};



class Garbage
{
public:
	Garbage();
	~Garbage();

// Called by GarbageObject constructor
	void add_object(GarbageObject *ptr);

// Called by user to delete the object.
// Flags the object for deletion as soon as it has no users.
	static void delete_object(GarbageObject *ptr);


// Called by remove_user and delete_object
	static void remove_expired();
	Mutex *lock;
	ArrayList<GarbageObject*> objects;

// Global garbage collector
	static Garbage *garbage;
};



#endif
