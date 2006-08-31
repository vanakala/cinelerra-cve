#ifndef BITSPOPUP_H
#define BITSPOPUP_H

#include "guicast.h"

class BitsPopupMenu;
class BitsPopupText;

class BitsPopup
{
public:
	BitsPopup(BC_WindowBase *parent_window, 
		int x, 
		int y, 
		int *output, 
		int use_ima4, 
		int use_ulaw, 
		int use_adpcm,
		int use_float,
		int use_32linear);
	~BitsPopup();
	int create_objects();
	int get_w();
	int get_h();
	
	BitsPopupMenu *menu;
	ArrayList<BC_ListBoxItem*> bits_items;
	BitsPopupText *textbox;
	int x, y, use_ima4, use_ulaw, use_float, use_adpcm, *output;
	int use_32linear;
	BC_WindowBase *parent_window;
};

class BitsPopupMenu : public BC_ListBox
{
public:
	BitsPopupMenu(BitsPopup *popup, int x, int y);
	int handle_event();
	BitsPopup *popup;
};

class BitsPopupText : public BC_TextBox
{
public:
	BitsPopupText(BitsPopup *popup, int x, int y);
	int handle_event();
	BitsPopup *popup;
};





#endif
