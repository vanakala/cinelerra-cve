#ifndef RECCONFIRMDELETE_H
#define RECCONFIRMDELETE_H


#include "guicast.h"

class RecConfirmDelete : public BC_Window
{
public:
	RecConfirmDelete(MWindow *mwindow);
	~RecConfirmDelete();
	
	int create_objects(char *string);
};



#endif
