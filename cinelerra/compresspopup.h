#ifndef COMPRESSPOPUP_H
#define COMPRESSPOPUP_H


#include "guicast.h"
#include "compresspopup.inc"

class CompressPopup : public BC_PopupTextBox
{
public:
	CompressPopup(int x, int y, int use_dv, char *output);
	~CompressPopup();

	int create_objects();

	int add_items();         // add initial items
	char format[256];           // current setting 
	ArrayList<BC_ListBoxItem*> compression_items;
	char *output;
	int use_dv;
	int x, y;
};







#endif
