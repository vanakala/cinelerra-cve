#include "bclistbox.h"
#include "bcpopuplistbox.h"

BC_PopupListText::BC_PopupListText(BC_PopupListBox *popup_listbox)
 : BC_TextBox(popup_listbox->x - popup_listbox->w + BCPOPUPLISTBOX_W,
 	popup_listbox->y, 
	popup_listbox->w - BCPOPUPLISTBOX_W, 
	1,
	popup_listbox->text)
{
public:
	~BC_PopupListText();

	int handle_event()





BC_PopupListBox::BC_PopupListBox(BC_SubWindow *parent_window,
	int x, int y, int w, int h, 
	ArrayList<BC_ListBoxItem*> *data,
	char *text)
 : BC_ListBox(x + w - BCPOPUPLISTBOX_W, y, w, h,
 	LISTBOX_TEXT,
	data,
	0,
	0,
	1,
	0,
	1,
	LISTBOX_SINGLE)
{
	this->data = data;
}

BC_PopupListBox::~BC_PopupListBox()
{
	delete listbox;
}

int BC_PopupListBox::initialize()
{
	BC_ListBox::initialize();
	
}
