#ifndef BYTEORDERPOPUP_H
#define BYTEORDERPOPUP_H

#include "guicast.h"

class ByteOrderList;
class ByteOrderText;

class ByteOrderPopup
{
public:
	ByteOrderPopup(BC_WindowBase *parent_window, int x, int y, int *output);
	~ByteOrderPopup();

	int create_objects();

	ArrayList<BC_ListBoxItem*> byteorder_items;
	BC_WindowBase *parent_window;
	int x;
	int y;
	int *output;
	ByteOrderList *menu;
	ByteOrderText *textbox;
};

class ByteOrderList : public BC_ListBox
{
public:
	ByteOrderList(ByteOrderPopup *popup, int x, int y);
	int handle_event();
	ByteOrderPopup *popup;
};

class ByteOrderText : public BC_TextBox
{
public:
	ByteOrderText(ByteOrderPopup *popup, int x, int y);
	int handle_event();
	ByteOrderPopup *popup;
};


#endif
