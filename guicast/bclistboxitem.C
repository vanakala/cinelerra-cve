#include "bclistboxitem.h"
#include "bcpixmap.h"


#include <string.h>


// ====================================================== item

BC_ListBoxItem::BC_ListBoxItem()
{
	initialize();
	this->text = new char[1];
	text[0] = 0;
}

BC_ListBoxItem::BC_ListBoxItem(char *text, 
	BC_Pixmap *icon, 
	int color)
{
	initialize();
	this->text = new char[strlen(text) + 1];
	this->icon = icon;

	strcpy(this->text, text);
	this->color = color;
}

BC_ListBoxItem::BC_ListBoxItem(char *text, int color)
{
	initialize();
	this->text = new char[strlen(text) + 1];
	strcpy(this->text, text);
	this->color = color;
}

BC_ListBoxItem::~BC_ListBoxItem()
{
	if(text) delete text;
	if(sublist)
	{
		for(int i = 0; i < columns; i++)
			sublist[i].remove_all_objects();
		delete sublist;
	}
}

int BC_ListBoxItem::initialize()
{
	autoplace_icon = 1;
	autoplace_text = 1;
	text = 0;
	color = BLACK;
	selected = 0;
	icon = 0;
	text_x = -1;
	text_y = -1;
	icon_x = -1;
	icon_y = -1;
	searchable = 1;
	sublist = 0;
	return 0;
}

int BC_ListBoxItem::get_icon_x()
{
	return icon_x;
}

int BC_ListBoxItem::get_icon_y()
{
	return icon_y;
}

int BC_ListBoxItem::get_text_x()
{
	return text_x;
}

int BC_ListBoxItem::get_text_y()
{
	return text_y;
}

int BC_ListBoxItem::set_autoplace_icon(int value)
{
	autoplace_icon = value;
	return 0;
}

int BC_ListBoxItem::set_autoplace_text(int value)
{
	autoplace_text = value;
	return 0;
}

void BC_ListBoxItem::set_icon_x(int x)
{
	icon_x = x;
	autoplace_icon = 0;
}

void BC_ListBoxItem::set_icon_y(int y)
{
	icon_y = y;
	autoplace_icon = 0;
}

void BC_ListBoxItem::set_selected(int value)
{
	this->selected = value;
}

void BC_ListBoxItem::set_searchable(int value)
{
	this->searchable = value;
}

void BC_ListBoxItem::set_text_x(int x)
{
	text_x = x;
	autoplace_text = 0;
}
void BC_ListBoxItem::set_text_y(int y)
{
	text_y = y;
	autoplace_text = 0;
}

int BC_ListBoxItem::get_icon_w()
{
	return icon->get_w();
}

int BC_ListBoxItem::get_icon_h()
{
	return icon->get_h();
}

void BC_ListBoxItem::set_text(char *new_text)
{
	if(this->text) delete this->text;
	this->text = 0;

	if(new_text)
	{
		this->text = new char[strlen(new_text) + 1];
		strcpy(this->text, new_text);
	}
}

char* BC_ListBoxItem::get_text()
{
	return text;
}

void BC_ListBoxItem::set_icon(BC_Pixmap *icon)
{
	this->icon = icon;
}

void BC_ListBoxItem::set_color(int color)
{
	this->color = color;
}

int BC_ListBoxItem::get_color()
{
	return color;
}


BC_ListBoxItem& BC_ListBoxItem::operator=(BC_ListBoxItem& item)
{
	if(item.text) set_text(item.text);
	color = item.color;
	text_x = item.text_x;
	text_y = item.text_y;
	icon_x = item.icon_x;
	icon_y = item.icon_y;
	return *this;
}


ArrayList<BC_ListBoxItem*>* BC_ListBoxItem::new_sublist(int columns)
{
	sublist = new ArrayList<BC_ListBoxItem*>[columns];
	this->columns = columns;
	return sublist;
}

ArrayList<BC_ListBoxItem*>* BC_ListBoxItem::get_sublist()
{
	return sublist;
}







