#include "assets.h"
#include "bitspopup.h"
#include "file.h"


BitsPopup::BitsPopup(BC_WindowBase *parent_window, 
	int x, 
	int y, 
	int *output, 
	int use_ima4, 
	int use_ulaw,
	int use_adpcm, 
	int use_float,
	int use_32linear)
{
	this->parent_window = parent_window;
	this->output = output;
	this->x = x;
	this->y = y;
	this->use_ima4 = use_ima4;
	this->use_ulaw = use_ulaw;
	this->use_adpcm = use_adpcm;
	this->use_float = use_float;
	this->use_32linear = use_32linear;
}

BitsPopup::~BitsPopup()
{
	delete menu;
	delete textbox;
	for(int i = 0; i < bits_items.total; i++)
		delete bits_items.values[i];
}

int BitsPopup::create_objects()
{
	bits_items.append(new BC_ListBoxItem(File::bitstostr(BITSLINEAR8)));
	bits_items.append(new BC_ListBoxItem(File::bitstostr(BITSLINEAR16)));
	bits_items.append(new BC_ListBoxItem(File::bitstostr(BITSLINEAR24)));
	if(use_32linear) bits_items.append(new BC_ListBoxItem(File::bitstostr(BITSLINEAR32)));
	if(use_ima4) bits_items.append(new BC_ListBoxItem(File::bitstostr(BITSIMA4)));
	if(use_ulaw) bits_items.append(new BC_ListBoxItem(File::bitstostr(BITSULAW)));
	if(use_adpcm) bits_items.append(new BC_ListBoxItem(File::bitstostr(BITS_ADPCM)));
	if(use_float) bits_items.append(new BC_ListBoxItem(File::bitstostr(BITSFLOAT)));

	parent_window->add_subwindow(textbox = new BitsPopupText(this, x, y));
	x += textbox->get_w();
	parent_window->add_subwindow(menu = new BitsPopupMenu(this, x, y));
	return 0;
}

int BitsPopup::get_w()
{
	return menu->get_w() + textbox->get_w();
}

BitsPopupMenu::BitsPopupMenu(BitsPopup *popup, int x, int y)
 : BC_ListBox(x,
 	y,
	120,
	100,
	LISTBOX_TEXT,
	&popup->bits_items,
	0,
	0,
	1,
	0,
	1)
{
	this->popup = popup;
}

int BitsPopupMenu::handle_event()
{
	popup->textbox->update(get_selection(0, 0)->get_text());
	popup->textbox->handle_event();
	return 1;
}

BitsPopupText::BitsPopupText(BitsPopup *popup, int x, int y)
 : BC_TextBox(x, y, 120, 1, File::bitstostr(*popup->output))
{
	this->popup = popup;
}

int BitsPopupText::handle_event()
{
	*popup->output = File::strtobits(get_text());
	return 1;
}
