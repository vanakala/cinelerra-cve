#ifndef BCTHEME_H
#define BCTHEME_H

#include "arraylist.h"
#include "bcresources.inc"
#include "bcwindowbase.inc"
#include "vframe.inc"

class BC_ThemeSet;




class BC_Theme
{
public:
	BC_Theme();
	virtual ~BC_Theme();

// Must set path of resource file before loading first image.
// Path defaults to executable path.
	void set_path(char *path);

// These create image sets which are stored in the set table.
// Takes comma delimited char* pointers to filenames.
	VFrame** new_image_set(int total, ...);
	VFrame** new_button(char *overlay_path, 
		char *up_path, 
		char *hi_path, 
		char *dn_path);
	VFrame** new_toggle(char *overlay_path,
		char *up_path,
		char *hi_path,
		char *checked_path,
		char *dn_path,
		char *checkedhi_path);
// Decompresses image and puts on images table before returning it.
	VFrame* new_image(char *title);

	void dump();
	BC_Resources* get_resources();

private:
	void overlay(VFrame *dst, VFrame *src, int in_x1 = -1, int in_x2 = -1);
	void init_contents();


// Loads compressed data into temporary
	unsigned char* get_image_data(char *title);

// Decompressed image storage
	ArrayList<VFrame*> images;
	ArrayList<BC_ThemeSet*> image_sets;
	char path[BCTEXTLEN];

// Compressed images are loaded in here.
	char *data_buffer;
	char *contents_buffer;
	ArrayList<char*> contents;
	ArrayList<int> offsets;
	char *last_image;
	int last_offset;
};

class BC_ThemeSet
{
public:
	BC_ThemeSet(int total);
	~BC_ThemeSet();

	VFrame **data;
};

#endif
