// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctheme.h"
#include "bcwindowbase.h"
#include "bcsignals.h"
#include "clip.h"
#include "vframe.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

BC_Theme::BC_Theme()
{
	data_ptr = 0;
	contents_ptr = 0;
	last_image = 0;
	last_pointer = 0;
}

BC_Theme::~BC_Theme()
{
	image_sets.remove_all_objects();
}

void BC_Theme::dump()
{
	printf("BC_Theme::dump 1 image_sets=%d contents=%d\n", 
		image_sets.total, 
		contents.total);
	for(int i = 0; i < contents.total; i++)
		printf("    %s %p\n", contents.values[i], pointers.values[i]);
}

BC_Resources* BC_Theme::get_resources()
{
	return BC_WindowBase::get_resources();
}

// These create single images for storage in the image_sets table.
VFrame* BC_Theme::new_image(const char *title, const char *path)
{
	VFrame *existing_image = (title && title[0]) ? get_image(title, 0) : 0;

	if(existing_image)
		return existing_image;

	BC_ThemeSet *result = new BC_ThemeSet(1, 0, title);

	result->data[0] = new VFrame(get_image_data(path));
	image_sets.append(result);
	return result->data[0];
}

VFrame* BC_Theme::new_image(const char *path)
{
	return new_image("", path);
}

// These create image sets which are stored in the image_sets table.
VFrame** BC_Theme::new_image_set(const char *title, int total, va_list *args)
{
	VFrame **existing_image_set = (title && title[0]) ? get_image_set(title, 0) : 0;

	if(existing_image_set)
		return existing_image_set;

	BC_ThemeSet *result = new BC_ThemeSet(total, 1, title);
	image_sets.append(result);

	for(int i = 0; i < total; i++)
	{
		char *path = va_arg(*args, char*);
		result->data[i] = new_image(path);
	}
	return result->data;
}

VFrame** BC_Theme::new_image_set_images(const char *title, int total, ...)
{
	va_list list;
	va_start(list, total);
	BC_ThemeSet *existing_image_set = (title && title[0]) ?
		get_image_set_object(title) : 0;

	if(existing_image_set)
		image_sets.remove_object(existing_image_set);

	BC_ThemeSet *result = new BC_ThemeSet(total, 0, title);

	image_sets.append(result);

	for(int i = 0; i < total; i++)
		result->data[i] = va_arg(list, VFrame*);

	va_end(list);
	return result->data;
}

VFrame** BC_Theme::new_image_set(const char *title, int total, ...)
{
	va_list list;
	va_start(list, total);
	VFrame **result = new_image_set(title, total, &list);
	va_end(list);

	return result;
}

VFrame** BC_Theme::new_image_set(int total, ...)
{
	va_list list;
	va_start(list, total);
	VFrame **result = new_image_set("", total, &list);
	va_end(list);

	return result;
}

VFrame* BC_Theme::get_image(const char *title, int use_default)
{
	if(title && title[0])
	{
		for(int i = 0; i < image_sets.total; i++)
		{
			if(!image_sets.values[i]->title)
				continue;
			if(!strcmp(image_sets.values[i]->title, title))
				return image_sets.values[i]->data[0];
		}
	}
	else
	{
		for(int i = 0; i < image_sets.total; i++)
			if(!image_sets.values[i]->title)
				return image_sets.values[i]->data[0];
	}

// Return the first image it can find.  This should always work.
	if(use_default)
	{
		printf("BC_Theme::get_image: image \"%s\" not found.\n",
			title);
		if(image_sets.total)
			return image_sets.values[0]->data[0];
	}
	return 0;
}

VFrame** BC_Theme::get_image_set(const char *title, int use_default)
{
	if(title && title[0])
	{
		for(int i = 0; i < image_sets.total; i++)
		{
			if(!image_sets.values[i]->title)
				continue;
			if(!strcmp(image_sets.values[i]->title, title))
				return image_sets.values[i]->data;
		}
	}
	else
	{
		for(int i = 0; i < image_sets.total; i++)
			if(!image_sets.values[i]->title)
				return image_sets.values[i]->data;
	}

// Get the image set with the largest number of images.
	if(use_default)
	{
		printf("BC_Theme::get_image_set: image set \"%s\" not found.\n",
			title);
		int max_total = 0;
		int max_number = -1;
		for(int i = 0; i < image_sets.total; i++)
		{
			if(image_sets.values[i]->total > max_total)
			{
				max_total = image_sets.values[i]->total;
				max_number = i;
			}
		}

		if(max_number >= 0)
			return image_sets.values[max_number]->data;
	}
	return 0;
}

BC_ThemeSet* BC_Theme::get_image_set_object(const char *title)
{
	if(title && title[0])
	{
		for(int i = 0; i < image_sets.total; i++)
		{
			if(!image_sets.values[i]->title)
				continue;
			if(!strcmp(image_sets.values[i]->title, title))
				return image_sets.values[i];
		}
	}
	else
	{
		for(int i = 0; i < image_sets.total; i++)
			if(!image_sets.values[i]->title)
				return image_sets.values[i];
	}
	return 0;
}

VFrame** BC_Theme::new_button(const char *overlay_path,
	const char *up_path,
	const char *hi_path,
	const char *dn_path,
	const char *title)
{
	VFrame default_data(get_image_data(overlay_path));
	BC_ThemeSet *result = new BC_ThemeSet(3, 1, title);

	if(title)
		image_sets.append(result);

	result->data[0] = new_image(up_path);
	result->data[1] = new_image(hi_path);
	result->data[2] = new_image(dn_path);

	for(int i = 0; i < 3; i++)
		overlay(result->data[i], &default_data, -1, -1, (i == 2));

	return result->data;
}

VFrame** BC_Theme::new_button4(const char *overlay_path,
	const char *up_path,
	const char *hi_path,
	const char *dn_path,
	const char *disabled_path,
	const char *title)
{
	VFrame default_data(get_image_data(overlay_path));
	BC_ThemeSet *result = new BC_ThemeSet(4, 1, title);

	if(title)
		image_sets.append(result);

	result->data[0] = new_image(up_path);
	result->data[1] = new_image(hi_path);
	result->data[2] = new_image(dn_path);
	result->data[3] = new_image(disabled_path);

	for(int i = 0; i < 4; i++)
		overlay(result->data[i], &default_data, -1, -1, (i == 2));

	return result->data;
}

VFrame** BC_Theme::new_button(const char *overlay_path,
	VFrame *up,
	VFrame *hi,
	VFrame *dn,
	const char *title)
{
	VFrame default_data(get_image_data(overlay_path));
	BC_ThemeSet *result = new BC_ThemeSet(3, 0, title);

	if(title)
		image_sets.append(result);

	result->data[0] = new VFrame(*up);
	result->data[1] = new VFrame(*hi);
	result->data[2] = new VFrame(*dn);
	for(int i = 0; i < 3; i++)
		overlay(result->data[i], &default_data, -1, -1, (i == 2));
	return result->data;
}

VFrame** BC_Theme::new_toggle(const char *overlay_path, 
	const char *up_path,
	const char *hi_path,
	const char *checked_path,
	const char *dn_path,
	const char *checkedhi_path,
	const char *title)
{
	VFrame default_data(get_image_data(overlay_path));
	BC_ThemeSet *result = new BC_ThemeSet(5, 1, title);

	if(title)
		image_sets.append(result);

	result->data[0] = new_image(up_path);
	result->data[1] = new_image(hi_path);
	result->data[2] = new_image(checked_path);
	result->data[3] = new_image(dn_path);
	result->data[4] = new_image(checkedhi_path);

	for(int i = 0; i < 5; i++)
		overlay(result->data[i], &default_data, -1, -1, (i == 3));

	return result->data;
}

VFrame** BC_Theme::new_toggle(const char *overlay_path,
	VFrame *up,
	VFrame *hi,
	VFrame *checked,
	VFrame *dn,
	VFrame *checkedhi,
	const char *title)
{
	VFrame default_data(get_image_data(overlay_path));
	BC_ThemeSet *result = new BC_ThemeSet(5, 0, title);

	if(title)
		image_sets.append(result);

	result->data[0] = new VFrame(*up);
	result->data[1] = new VFrame(*hi);
	result->data[2] = new VFrame(*checked);
	result->data[3] = new VFrame(*dn);
	result->data[4] = new VFrame(*checkedhi);
	for(int i = 0; i < 5; i++)
		overlay(result->data[i], &default_data, -1, -1, (i == 3));
	return result->data;
}

void BC_Theme::overlay(VFrame *dst, VFrame *src, int in_x1, int in_x2, int shift)
{
	int w;
	int h;
	unsigned char **in_rows;
	unsigned char **out_rows;

	if(in_x1 < 0)
	{
		w = MIN(src->get_w(), dst->get_w());
		h = MIN(dst->get_h(), src->get_h());
		in_x1 = 0;
		in_x2 = w;
	}
	else
	{
		w = in_x2 - in_x1;
		h = MIN(dst->get_h(), src->get_h());
	}
	switch(src->get_color_model())
	{
	case BC_RGBA8888:
		switch(dst->get_color_model())
		{
		case BC_RGBA8888:
			for(int i = shift; i < h; i++)
			{
				unsigned char *in_row = 0;
				unsigned char *out_row;

				if(!shift)
				{
					in_row = src->get_row_ptr(i) + in_x1 * 4;
					out_row = dst->get_row_ptr(i);
				}
				else
				{
					in_row = src->get_row_ptr(i - 1) + in_x1 * 4;
					out_row = dst->get_row_ptr(i) + 4;
				}

				for(int j = shift; j < w; j++)
				{
					int opacity = in_row[3];
					int transparency = 0xff - opacity;

					out_row[0] = (in_row[0] * opacity + out_row[0] * transparency) / 0xff;
					out_row[1] = (in_row[1] * opacity + out_row[1] * transparency) / 0xff;
					out_row[2] = (in_row[2] * opacity + out_row[2] * transparency) / 0xff;
					out_row[3] = MAX(in_row[3], out_row[3]);
					out_row += 4;
					in_row += 4;
				}
			}
			break;

		case BC_RGB888:
			for(int i = shift; i < h; i++)
			{
				unsigned char *in_row;
				unsigned char *out_row = out_rows[i];

				if(!shift)
				{
					in_row = src->get_row_ptr(i) + in_x1 * 3;
					out_row = dst->get_row_ptr(i);
				}
				else
				{
					in_row = src->get_row_ptr(i - 1) + in_x1 * 3;
					out_row = dst->get_row_ptr(i) + 3;
				}

				for(int j = shift; j < w; j++)
				{
					int opacity = in_row[3];
					int transparency = 0xff - opacity;
					out_row[0] = (in_row[0] * opacity + out_row[0] * transparency) / 0xff;
					out_row[1] = (in_row[1] * opacity + out_row[1] * transparency) / 0xff;
					out_row[2] = (in_row[2] * opacity + out_row[2] * transparency) / 0xff;
					out_row += 3;
					in_row += 4;
				}
			}
			break;
		}
		break;
	}
}

void BC_Theme::set_data(unsigned char *ptr)
{
	contents_ptr = (char*)(ptr + sizeof(int));
	int contents_size = *(int*)ptr - sizeof(int);
	data_ptr = contents_ptr + contents_size;

	for(int i = 0; i < contents_size; )
	{
		used.append(0);
		contents.append(contents_ptr + i);
		while(contents_ptr[i] && i < contents_size)
			i++;
		if(i < contents_size)
		{
			i++;
			pointers.append((unsigned char*)data_ptr +
				*(unsigned int*)(contents_ptr + i));
			i += 4;
		}
		else
		{
			pointers.append((unsigned char*)data_ptr);
			break;
		}
	}
}

unsigned char* BC_Theme::get_image_data(const char *title)
{
	if(!data_ptr)
	{
		fprintf(stderr, "BC_Theme::get_image_data: no data set\n");
		return 0;
	}

// Image is the same as the last one
	if(last_image && !strcasecmp(last_image, title))
		return last_pointer;
	else
// Search for image anew.
	for(int i = 0; i < contents.total; i++)
	{
		if(!strcasecmp(contents.values[i], title))
		{
			last_pointer = pointers.values[i];
			last_image = contents.values[i];
			used.values[i] = 1;
			return pointers.values[i];
		}
	}

	fprintf(stderr, "BC_Theme::get_image: %s not found.\n", title);
	return 0;
}

void BC_Theme::check_used()
{
	int got_it = 0;

	for(int i = 0; i < used.total; i++)
	{
		if(!used.values[i])
		{
			if(!got_it)
				printf("BC_Theme::check_used: Unused images:\n");
			printf("%s\n", contents.values[i]);
			got_it = 1;
		}
	}
	if(got_it)
		printf("\n");
}

BC_ThemeSet::BC_ThemeSet(int total, int is_reference, const char *title)
{
	this->total = total;
	if(title && title[0])
	{
		this->title = new char[strlen(title) + 1];
		strcpy(this->title, title);
	}
	else
		this->title = 0;
	this->is_reference = is_reference;
	data = new VFrame*[total];
}

BC_ThemeSet::~BC_ThemeSet()
{
	if(data)
	{
		if(!is_reference)
		{
			for(int i = 0; i < total; i++)
				delete data[i];
		}
		delete [] data;
	}
	delete [] title;
}
