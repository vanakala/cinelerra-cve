#ifndef BCTHEME_H
#define BCTHEME_H

#include "arraylist.h"
#include "bcresources.inc"
#include "bcwindowbase.inc"
#include "vframe.inc"
#include <stdarg.h>

class BC_ThemeSet;




class BC_Theme
{
public:
	BC_Theme();
	virtual ~BC_Theme();

// Must set path of resource file before loading first image.
// Path defaults to executable path.
	void set_path(char *path);

// Reverts the path to the executable path.  Useful for base classes
// which need to fill unset images with defaults after they've been created.
	void unset_path();

	VFrame** new_button(char *overlay_path, 
		char *up_path, 
		char *hi_path, 
		char *dn_path);
	VFrame** new_button(char *overlay_path,
		VFrame *up,
		VFrame *hi,
		VFrame *dn);
	VFrame** new_toggle(char *overlay_path,
		char *up_path,
		char *hi_path,
		char *checked_path,
		char *dn_path,
		char *checkedhi_path);
	VFrame** new_toggle(char *overlay_path,
		VFrame *up,
		VFrame *hi,
		VFrame *checked,
		VFrame *dn,
		VFrame *checkedhi);


// The two main routines for creating images are new_image_set and new_image.
// If the title already exists in the table, the existing entry is returned.
// These create image sets which are stored in the set table.
// Takes comma delimited char* pointers to filenames.
	VFrame** new_image_set(char *title, int total, va_list *args);
	VFrame** new_image_set(char *title, int total, ...);
	VFrame** new_image_set(int total, ...);


// Decompresses image and puts on images table before returning it.
	VFrame* new_image(char *title, char *path);
	VFrame* new_image(char *path);


// These retrieve images based on case sensitive title
	VFrame* get_image(char *title);
	VFrame** get_image_set(char *title);

// Loads compressed data into temporary
	unsigned char* get_image_data(char *title);

// Verify all images have been used after initialization.
	void check_used();

	void dump();
	BC_Resources* get_resources();

private:
	void overlay(VFrame *dst, VFrame *src, int in_x1 = -1, int in_x2 = -1);
	void init_contents();



// Decompressed image storage.
// Sets of images.
	ArrayList<BC_ThemeSet*> image_sets;
	char path[BCTEXTLEN];
	char default_path[BCTEXTLEN];

// Compressed images are loaded in here.
	char *data_buffer;
	char *contents_buffer;
	ArrayList<char*> contents;
	ArrayList<int> offsets;
	ArrayList<int> used;
	char *last_image;
	int last_offset;
};

class BC_ThemeSet
{
public:
// Set is_reference if the images are going to be created by new_image
	BC_ThemeSet(int total, int is_reference, char *title);
	~BC_ThemeSet();

	VFrame **data;
	char *title;
	int total;
	int is_reference;
};



#endif
