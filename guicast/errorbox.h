#ifndef ERRORBOX_H
#define ERRORBOX_H

#include "guicast.h"

class ErrorBox : public BC_Window
{
public:
	ErrorBox(char *title, 
		int x = (int)BC_INFINITY, 
		int y = (int)BC_INFINITY, 
		int w = 400, 
		int h = 120);
	virtual ~ErrorBox();

	int create_objects(char *text);
};

#endif

