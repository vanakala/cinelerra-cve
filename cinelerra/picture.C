
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include <ctype.h>
#include "bchash.h"
#include "picture.h"
#include <string.h>





PictureItem::PictureItem()
{
	name[0] = 0;
	min = 0;
	max = 0;
	default_ = 0;
	step = 0;
	type = 0;
	value = 0;
	device_id = 0;
}


PictureItem::~PictureItem()
{
}

void PictureItem::copy_from(PictureItem *src)
{
	strcpy(this->name, src->name);
	this->device_id = src->device_id;
	this->min = src->min;
	this->max = src->max;
	this->default_ = src->default_;
	this->step = src->step;
	this->type = src->type;
	this->value = src->value;
}

char* PictureItem::get_default_string(char *string)
{
	sprintf(string, "VIDEO_%s_VALUE", name);
	for(int i = 0; i < strlen(string); i++)
		string[i] = toupper(string[i]);
	return string;
}








PictureConfig::PictureConfig(BC_Hash *defaults)
{
	this->defaults = defaults;
	brightness = -1;
	hue = -1;
	color = -1;
	contrast = -1;
	whiteness = -1;

	use_brightness = 0;
	use_contrast = 0;
	use_color = 0;
	use_hue = 0;
	use_whiteness = 0;
}

PictureConfig::~PictureConfig()
{
	controls.remove_all_objects();
}

void PictureConfig::copy_settings(PictureConfig *picture)
{
	this->brightness = picture->brightness;
	this->hue = picture->hue;
	this->color = picture->color;
	this->contrast = picture->contrast;
	this->whiteness = picture->whiteness;


	for(int i = 0; i < picture->controls.total; i++)
	{
		PictureItem *src_item = picture->controls.values[i];
		PictureItem *dst_item = 0;
		for(int j = 0; j < controls.total; j++)
		{
			if(controls.values[j]->device_id == src_item->device_id)
			{
				dst_item = controls.values[j];
				break;
			}
		}
		if(!dst_item)
		{
			dst_item = new PictureItem;
			controls.append(dst_item);
		}
		dst_item->copy_from(src_item);
	}
}

void PictureConfig::copy_usage(PictureConfig *picture)
{
	this->use_brightness = picture->use_brightness;
	this->use_contrast = picture->use_contrast;
	this->use_color = picture->use_color;
	this->use_hue = picture->use_hue;
	this->use_whiteness = picture->use_whiteness;

// Add the controls if they don't exists but don't replace existing values.
	for(int i = 0; i < picture->controls.total; i++)
	{
		PictureItem *src = picture->controls.values[i];
		int got_it = 0;
		for(int j = 0; j < controls.total; j++)
		{
			if(controls.values[j]->device_id == src->device_id)
			{
				got_it = 1;
				break;
			}
		}
		if(!got_it)
		{
			PictureItem *dst = new PictureItem;
			controls.append(dst);
			dst->copy_from(src);
		}
	}
}

void PictureConfig::load_defaults()
{
	if(!defaults)
	{
		printf("PictureConfig::load_defaults: no defaults pointer.\n");
		return;
	}
	brightness = defaults->get("VIDEO_BRIGHTNESS", 0);
	hue = defaults->get("VIDEO_HUE", 0);
	color = defaults->get("VIDEO_COLOR", 0);
	contrast = defaults->get("VIDEO_CONTRAST", 0);
	whiteness = defaults->get("VIDEO_WHITENESS", 0);

// The device must be probed first to keep unsupported controls from getting 
// displayed.
	for(int i = 0; i < controls.total; i++)
	{
		PictureItem *item = controls.values[i];
		char string[BCTEXTLEN];
		item->get_default_string(string);
		item->value = defaults->get(string, item->value);
//printf("PictureConfig::load_defaults %s %d %d\n", item->name, item->device_id, item->value);
	}
}

void PictureConfig::save_defaults()
{
	if(!defaults)
	{
		printf("PictureConfig::save_defaults: no defaults pointer.\n");
		return;
	}
	defaults->update("VIDEO_BRIGHTNESS", brightness);
	defaults->update("VIDEO_HUE", hue);
	defaults->update("VIDEO_COLOR", color);
	defaults->update("VIDEO_CONTRAST", contrast);
	defaults->update("VIDEO_WHITENESS", whiteness);
	for(int i = 0; i < controls.total; i++)
	{
		PictureItem *item = controls.values[i];
		char string[BCTEXTLEN];
		item->get_default_string(string);
		defaults->update(string, item->value);
//printf("PictureConfig::save_defaults %s %d %d\n", string, item->device_id, item->value);
	}
}

void PictureConfig::dump()
{
	printf("    VIDEO_BRIGHTNESS=%d\n", brightness);
	printf("    VIDEO_HUE=%d\n", hue);
	printf("    VIDEO_COLOR=%d\n", color);
	printf("    VIDEO_CONTRAST=%d\n", contrast);
	printf("    VIDEO_WHITENESS=%d\n", whiteness);
	for(int i = 0; i < controls.total; i++)
	{
		PictureItem *item = controls.values[i];
		char string[BCTEXTLEN];
		item->get_default_string(string);
		printf("    %s=%d\n", string, item->value);
	}
}

PictureItem* PictureConfig::new_item(const char *name)
{
	for(int i = 0; i < controls.total; i++)
	{
		if(!strcmp(controls.values[i]->name, name)) return controls.values[i];
	}
	PictureItem *item = new PictureItem;
	strcpy(item->name, name);
	controls.append(item);
	return item;
}

PictureItem* PictureConfig::get_item(const char *name, int id)
{
	for(int i = 0; i < controls.total; i++)
	{
		if(!strcmp(controls.values[i]->name, name) &&
			controls.values[i]->device_id == id) return controls.values[i];
	}
	return 0;
}

void PictureConfig::set_item(int device_id, int value)
{
	for(int i = 0; i < controls.total; i++)
	{
		if(controls.values[i]->device_id == device_id)
		{
			controls.values[i]->value = value;
			return;
		}
	}
}








