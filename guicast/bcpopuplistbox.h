#ifndef BCPOPUPLISTBOX_H
#define BCPOPUPLISTBOX_H

#include "bclistbox.h"
#include "bctextbox.h"

class BC_PopupListText : public BC_TextBox
{
public:
	BC_PopupListText(BC_PopupListBox *popup_listbox);
	~BC_PopupListText();
	
	int handle_event();
	
	BC_PopupListBox *popup_listbox;
};

class BC_PopupListBox : public BC_ListBox
{
public:
	BC_PopupListBox(int x, int y, int w, int h, 
		ArrayList<BC_ListBoxItem*> *data,
		char *text);
	~BC_PopupListBox();
	
	int initialize();

private:
	ArrayList<BC_ListBoxItem*> *data;

	BC_Listbox *listbox;
	BC_PopupListText *textbox;
};

#endif
