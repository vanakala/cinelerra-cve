#ifndef BCSUBWINDOW_H
#define BCSUBWINDOW_H

#include "arraylist.h"
#include "bcwindowbase.h"

class BC_SubWindow : public BC_WindowBase
{
public:
	BC_SubWindow(int x, int y, int w, int h, int bg_color = -1);
	virtual ~BC_SubWindow();

	virtual int initialize();
private:
};

class BC_SubWindowList : public ArrayList<BC_WindowBase*>
{
public:
	BC_SubWindowList();
	~BC_SubWindowList();


private:
	
};



#endif
