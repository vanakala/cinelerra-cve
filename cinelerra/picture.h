#ifndef PICTURE_H
#define PICTURE_H


// Container for picture controls

#include "arraylist.h"
#include "bcwindowbase.inc"
#include "defaults.inc"

class PictureItem
{
public:
	PictureItem();
	~PictureItem();
	
	void copy_from(PictureItem *src);
	char* get_default_string(char *string);
	char name[BCTEXTLEN];
	int device_id;
	int min;
	int max;
	int default_;
	int step;
	int type;
	int value;
};


class Picture
{
public:
	Picture();
	~Picture();
	void copy_settings(Picture *picture);
	void copy_usage(Picture *picture);
	void load_defaults(Defaults *defaults);
	void save_defaults(Defaults *defaults);
	void set_item(int device_id, int value);

	int brightness;
	int hue;
	int color;
	int contrast;
	int whiteness;

// Flags for picture settings the device uses
	int use_brightness;
	int use_contrast;
	int use_color;
	int use_hue;
	int use_whiteness;

// For the latest APIs the controls are defined by the driver
// Search for existing driver with name.  If none exists, create it.
	PictureItem* new_item(char *name);
	ArrayList<PictureItem*> controls;
	
};


#endif
