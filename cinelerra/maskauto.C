#include "clip.h"
#include "filexml.h"
#include "maskauto.h"
#include "maskautos.h"

#include <stdlib.h>
#include <string.h>




MaskPoint::MaskPoint()
{
	x = 0;
	y = 0;
	control_x1 = 0;
	control_y1 = 0;
	control_x2 = 0;
	control_y2 = 0;
}

MaskPoint& MaskPoint::operator=(MaskPoint& ptr)
{
	this->x = ptr.x;
	this->y = ptr.y;
	this->control_x1 = ptr.control_x1;
	this->control_y1 = ptr.control_y1;
	this->control_x2 = ptr.control_x2;
	this->control_y2 = ptr.control_y2;
}

int MaskPoint::operator==(MaskPoint& ptr)
{
	return EQUIV(x, ptr.x) &&
		EQUIV(y, ptr.y) &&
		EQUIV(control_x1, ptr.control_x1) &&
		EQUIV(control_y1, ptr.control_y1) &&
		EQUIV(control_x2, ptr.control_x2) &&
		EQUIV(control_y2, ptr.control_y2);
}

SubMask::SubMask(MaskAuto *keyframe)
{
	this->keyframe = keyframe;
}

SubMask::~SubMask()
{
}

int SubMask::operator==(SubMask& ptr)
{
	if(points.total != ptr.points.total) return 0;

	for(int i = 0; i < points.total; i++)
	{
		if(!(*points.values[i] == *ptr.points.values[i]))
			return 0;
	}
	
	return 1;
}

void SubMask::copy_from(SubMask& ptr)
{
	points.remove_all_objects();
	for(int i = 0; i < ptr.points.total; i++)
	{
		MaskPoint *point = new MaskPoint;
		*point = *ptr.points.values[i];
		points.append(point);
	}
}

void SubMask::load(FileXML *file)
{
	points.remove_all_objects();

	int result = 0;
	while(!result)
	{
		result = file->read_tag();
		
		if(!result)
		{
			if(file->tag.title_is("/MASK"))
			{
				result = 1;
			}
			else
			if(file->tag.title_is("POINT"))
			{
				char string[BCTEXTLEN];
				string[0] = 0;
				file->read_text_until("/POINT", string);

				MaskPoint *point = new MaskPoint;
				char *ptr = string;
//printf("MaskAuto::load 1 %s\n", ptr);

				point->x = atof(ptr);
				ptr = strchr(ptr, ',');
//printf("MaskAuto::load 2 %s\n", ptr + 1);
				if(ptr) 
				{
					point->y = atof(ptr + 1);
					ptr = strchr(ptr + 1, ',');
				
					if(ptr)
					{
//printf("MaskAuto::load 3 %s\n", ptr + 1);
						point->control_x1 = atof(ptr + 1);
						ptr = strchr(ptr + 1, ',');
						if(ptr)
						{
//printf("MaskAuto::load 4 %s\n", ptr + 1);
							point->control_y1 = atof(ptr + 1);
							ptr = strchr(ptr + 1, ',');
							if(ptr)
							{
//printf("MaskAuto::load 5 %s\n", ptr + 1);
								point->control_x2 = atof(ptr + 1);
								ptr = strchr(ptr + 1, ',');
								if(ptr) point->control_y2 = atof(ptr + 1);
							}
						}
					}
					
				}
				points.append(point);
			}
		}
	}
}

void SubMask::copy(FileXML *file)
{
	if(points.total)
	{
		file->tag.set_title("MASK");
		file->tag.set_property("NUMBER", keyframe->masks.number_of(this));
		file->append_tag();
		file->append_newline();

		for(int i = 0; i < points.total; i++)
		{
			file->append_newline();
			file->tag.set_title("POINT");
			file->append_tag();
			char string[BCTEXTLEN];
			sprintf(string, "%.6e, %.6e, %.6e, %.6e, %.6e, %.6e", 
				points.values[i]->x, 
				points.values[i]->y, 
				points.values[i]->control_x1, 
				points.values[i]->control_y1, 
				points.values[i]->control_x2, 
				points.values[i]->control_y2);
			file->append_text(string);
			file->tag.set_title("/POINT");
			file->append_tag();
		}
		file->append_newline();

		file->tag.set_title("/MASK");
		file->append_tag();
		file->append_newline();
	}
}

void SubMask::dump()
{
	for(int i = 0; i < points.total; i++)
	{
		printf("	  point=%d x=%.2f y=%.2f in_x=%.2f in_y=%.2f out_x=%.2f out_y=%.2f\n",
			i,
			points.values[i]->x, 
			points.values[i]->y, 
			points.values[i]->control_x1, 
			points.values[i]->control_y1, 
			points.values[i]->control_x2, 
			points.values[i]->control_y2);
	}
}


MaskAuto::MaskAuto(EDL *edl, MaskAutos *autos)
 : Auto(edl, autos)
{
	mode = MASK_SUBTRACT_ALPHA;
	feather = 0;
	value = 100;

// We define a fixed number of submasks so that interpolation for each
// submask matches.

	for(int i = 0; i < SUBMASKS; i++)
		masks.append(new SubMask(this));
}

MaskAuto::~MaskAuto()
{
	masks.remove_all_objects();
}

int MaskAuto::operator==(Auto &that)
{
	return identical((MaskAuto*)&that);
}



int MaskAuto::operator==(MaskAuto &that)
{
	return identical((MaskAuto*)&that);
}


int MaskAuto::identical(MaskAuto *src)
{
	if(value != src->value ||
		mode != src->mode ||
		feather != src->feather ||
		masks.total != src->masks.total) return 0;

	for(int i = 0; i < masks.total; i++)
		if(!(*masks.values[i] == *src->masks.values[i])) return 0;

	return 1;
}

void MaskAuto::copy_from(Auto *src)
{
	copy_from((MaskAuto*)src);
}

void MaskAuto::copy_from(MaskAuto *src)
{
	Auto::copy_from(src);

	mode = src->mode;
	feather = src->feather;
	value = src->value;

	masks.remove_all_objects();
	for(int i = 0; i < src->masks.total; i++)
	{
		masks.append(new SubMask(this));
		masks.values[i]->copy_from(*src->masks.values[i]);
	}
}

SubMask* MaskAuto::get_submask(int number)
{
	CLAMP(number, 0, masks.total - 1);
	return masks.values[number];
}

void MaskAuto::load(FileXML *file)
{
	mode = file->tag.get_property("MODE", mode);
	feather = file->tag.get_property("FEATHER", feather);
	value = file->tag.get_property("VALUE", value);
	for(int i = 0; i < masks.total; i++)
	{
		delete masks.values[i];
		masks.values[i] = new SubMask(this);
	}

	int result = 0;
	while(!result)
	{
		result = file->read_tag();

		if(!result)
		{
			if(file->tag.title_is("/AUTO")) 
				result = 1;
			else
			if(file->tag.title_is("MASK"))
			{
				SubMask *mask = masks.values[file->tag.get_property("NUMBER", 0)];
				mask->load(file);
			}
		}
	}
//	dump();
}

void MaskAuto::copy(int64_t start, int64_t end, FileXML *file, int default_auto)
{
	file->tag.set_title("AUTO");
	file->tag.set_property("MODE", mode);
	file->tag.set_property("VALUE", value);
	file->tag.set_property("FEATHER", feather);
	if(default_auto)
		file->tag.set_property("POSITION", 0);
	else
		file->tag.set_property("POSITION", position - start);
	file->append_tag();
	file->append_newline();

	for(int i = 0; i < masks.total; i++)
	{
		masks.values[i]->copy(file);
	}

	file->append_newline();
	file->tag.set_title("/AUTO");
	file->append_tag();
	file->append_newline();
}

void MaskAuto::dump()
{
	printf("	 mode=%d value=%d\n", mode, value);
	for(int i = 0; i < masks.total; i++)
	{
		printf("	 submask %d\n", i);
		masks.values[i]->dump();
	}
}




