#include "bctheme.h"
#include "bcwindowbase.h"
#include "clip.h"
#include "filesystem.h"
#include "language.h"
#include "vframe.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

BC_Theme::BC_Theme()
{
	char *path_env = getenv("PATH");
	char executable_name[BCTEXTLEN];
	char *path_ptr = path_env;

// Get location of executable
	FILE *fd = fopen("/proc/self/cmdline", "r");
	if(!fd)
	{
		perror(_("BC_Theme::BC_Theme: can't open /proc/self/cmdline.\n"));
		return;
	}
	else
	{
		fread(executable_name, 1, BCTEXTLEN, fd);
		strcpy(path, executable_name);
		fclose(fd);
	}

	int done = 0;
	fd = 0;

// Search path environment for executable
	do
	{
		FileSystem fs;
// Ignore directories
		if(!fs.is_dir(path))
			fd = 0;
		else
			fd = fopen(path, "r");


		if(!fd)
		{
// Get next entry out of path_env
			while(*path_ptr)
			{
				if(*path_ptr != ':') 
					break;
				else
					path_ptr++;
			}

			if(!*path_ptr) break;

			char *new_ptr = path;
			while(*path_ptr)
			{
				if(*path_ptr == ':')
					break;
				else
					*new_ptr++ = *path_ptr++;
			}

			*new_ptr = 0;
			if(new_ptr - path > 0 && *(new_ptr - 1) != '/')
			{
				*new_ptr++ = '/';
				*new_ptr = 0;
			}
			strcat(path, executable_name);
		}
		else
		{
			fclose(fd);
			done = 1;
		}
	}while(!done);

	strcpy(default_path, path);
	data_buffer = 0;
	contents_buffer = 0;
	last_image = 0;
}

BC_Theme::~BC_Theme()
{
	image_sets.remove_all_objects();
	if(data_buffer) delete [] data_buffer;
	if(contents_buffer) delete [] contents_buffer;
}

void BC_Theme::dump()
{
	printf("BC_Theme::dump 1 image_sets=%d path=%s contents=%d\n", 
		image_sets.total, 
		path, 
		contents.total);
	for(int i = 0; i < contents.total; i++)
		printf("    %s %x\n", contents.values[i], offsets.values[i]);
}

BC_Resources* BC_Theme::get_resources()
{
	return BC_WindowBase::get_resources();
}


// These create single images for storage in the image_sets table.
VFrame* BC_Theme::new_image(char *title, char *path)
{
	VFrame *existing_image = title[0] ? get_image(title) : 0;
	if(existing_image) return existing_image;

	BC_ThemeSet *result = new BC_ThemeSet(1, 0, title);
	result->data[0] = new VFrame(get_image_data(path));
	image_sets.append(result);
	return result->data[0];
}

VFrame* BC_Theme::new_image(char *path)
{
	return new_image("", path);
}

// These create image sets which are stored in the image_sets table.
VFrame** BC_Theme::new_image_set(char *title, int total, va_list *args)
{
	VFrame **existing_image_set = title[0] ? get_image_set(title) : 0;
	if(existing_image_set) return existing_image_set;

	BC_ThemeSet *result = new BC_ThemeSet(total, 1, title);
	image_sets.append(result);
	for(int i = 0; i < total; i++)
	{
		char *path = va_arg(*args, char*);
		result->data[i] = new_image(path);
	}
	return result->data;
}

VFrame** BC_Theme::new_image_set(char *title, int total, ...)
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

VFrame* BC_Theme::get_image(char *title)
{
	for(int i = 0; i < image_sets.total; i++)
	{
		if(!strcmp(image_sets.values[i]->title, title))
		{
			return image_sets.values[i]->data[0];
		}
	}



// TODO: Need to return a default image
	return 0;
}

VFrame** BC_Theme::get_image_set(char *title)
{
	for(int i = 0; i < image_sets.total; i++)
	{
		if(!strcmp(image_sets.values[i]->title, title))
		{
			return image_sets.values[i]->data;
		}
	}



// TODO: Need to return a default image set
	return 0;
}













VFrame** BC_Theme::new_button(char *overlay_path, 
	char *up_path, 
	char *hi_path, 
	char *dn_path)
{
	VFrame default_data(get_image_data(overlay_path));
	BC_ThemeSet *result = new BC_ThemeSet(3, 1, "");

	result->data[0] = new_image(up_path);
	result->data[1] = new_image(hi_path);
	result->data[2] = new_image(dn_path);
	for(int i = 0; i < 3; i++)
		overlay(result->data[i], &default_data);
	return result->data;
}


VFrame** BC_Theme::new_button(char *overlay_path, 
	VFrame *up,
	VFrame *hi,
	VFrame *dn)
{
	VFrame default_data(get_image_data(overlay_path));
	BC_ThemeSet *result = new BC_ThemeSet(3, 0, "");

	result->data[0] = new VFrame(*up);
	result->data[1] = new VFrame(*hi);
	result->data[2] = new VFrame(*dn);
	for(int i = 0; i < 3; i++)
		overlay(result->data[i], &default_data);
	return result->data;
}


VFrame** BC_Theme::new_toggle(char *overlay_path, 
	char *up_path,
	char *hi_path,
	char *checked_path,
	char *dn_path,
	char *checkedhi_path)
{
	VFrame default_data(get_image_data(overlay_path));
	BC_ThemeSet *result = new BC_ThemeSet(5, 1, "");

	result->data[0] = new_image(up_path);
	result->data[1] = new_image(hi_path);
	result->data[2] = new_image(checked_path);
	result->data[3] = new_image(dn_path);
	result->data[4] = new_image(checkedhi_path);
	for(int i = 0; i < 5; i++)
		overlay(result->data[i], &default_data);
	return result->data;
}

VFrame** BC_Theme::new_toggle(char *overlay_path, 
	VFrame *up,
	VFrame *hi,
	VFrame *checked,
	VFrame *dn,
	VFrame *checkedhi)
{
	VFrame default_data(get_image_data(overlay_path));
	BC_ThemeSet *result = new BC_ThemeSet(5, 0, "");

	result->data[0] = new VFrame(*up);
	result->data[1] = new VFrame(*hi);
	result->data[2] = new VFrame(*checked);
	result->data[3] = new VFrame(*dn);
	result->data[4] = new VFrame(*checkedhi);
	for(int i = 0; i < 5; i++)
		overlay(result->data[i], &default_data);
	return result->data;
}
void BC_Theme::overlay(VFrame *dst, VFrame *src, int in_x1, int in_x2)
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
	in_rows = src->get_rows();
	out_rows = dst->get_rows();

	switch(src->get_color_model())
	{
		case BC_RGBA8888:
			switch(dst->get_color_model())
			{
				case BC_RGBA8888:
					for(int i = 0; i < h; i++)
					{
						unsigned char *in_row = in_rows[i] + in_x1 * 4;
						unsigned char *out_row = out_rows[i];
						for(int j = 0; j < w; j++)
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
					for(int i = 0; i < h; i++)
					{
						unsigned char *in_row = in_rows[i] + in_x1 * 4;
						unsigned char *out_row = out_rows[i];
						for(int j = 0; j < w; j++)
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

void BC_Theme::set_path(char *path)
{
	strcpy(this->path, path);
}

void BC_Theme::unset_path()
{
	strcpy(this->path, default_path);
	if(data_buffer) delete [] data_buffer;
	if(contents_buffer) delete [] contents_buffer;
	data_buffer = 0;
	contents_buffer = 0;
	last_image = 0;
	contents.remove_all();
	offsets.remove_all();
	used.remove_all();
}

unsigned char* BC_Theme::get_image_data(char *title)
{
// Read contents
	if(!data_buffer)
	{
		FileSystem fs;
		if(!fs.is_dir(path))
		{
			fprintf(stderr, _("Theme::get_image: %s is a directory.\n"), path);
			return 0;
		}
		FILE *fd = fopen(path, "r");
		int result = 0;

		if(!fd)
		{
			fprintf(stderr, _("Theme::get_image: %s when opening %s\n"), strerror(errno), path);
		}
		int data_offset, contents_offset;
		int total_bytes;
		int data_size;
		int contents_size;

		if(fseek(fd, -8, SEEK_END) < 0)
		{
			fprintf(stderr, _("BC_Theme::get_image_data fseek %s\n"), strerror(errno));
			return 0;
		}
		total_bytes = ftell(fd);
		if(fread(&data_offset, 1, 4, fd) != 4)
		{
			fprintf(stderr, _("BC_Theme::get_image_data fread 1 %s\n"), strerror(errno));
			return 0;
		}
		if(fread(&contents_offset, 1, 4, fd) != 4)
		{
			fprintf(stderr, _("BC_Theme::get_image_data fread 2 %s\n"), strerror(errno));
			return 0;
		}


		fseek(fd, data_offset, SEEK_SET);
		data_size = contents_offset - data_offset;
		data_buffer = new char[data_size];
		fread(data_buffer, 1, data_size, fd);

		fseek(fd, contents_offset, SEEK_SET);
		contents_size = total_bytes - contents_offset;
		contents_buffer = new char[contents_size];
		fread(contents_buffer, 1, contents_size, fd);

		char *start_of_title = contents_buffer;
		for(int i = 0; i < contents_size; )
		{
			if(contents_buffer[i] == 0)
			{
				contents.append(start_of_title);
				i++;
				offsets.append(*(int*)(contents_buffer + i));
				i += 4;
				start_of_title = contents_buffer + i;
				used.append(0);
			}
			else
				i++;
		}
		fclose(fd);
	}

// Image is the same as the last one
	if(last_image && !strcasecmp(last_image, title))
	{
		return (unsigned char*)(data_buffer + last_offset);
	}
	else
// Search for image anew.
	for(int i = 0; i < contents.total; i++)
	{
		if(!strcasecmp(contents.values[i], title))
		{
			last_offset = offsets.values[i];
			last_image = contents.values[i];
			used.values[i] = 1;
			return (unsigned char*)(data_buffer + offsets.values[i]);
		}
	}

	fprintf(stderr, _("Theme::get_image: %s not found.\n"), title);
	return 0;
}

void BC_Theme::check_used()
{
// Can't use because some images are gotten the old fashioned way.
return;
	int got_it = 0;
	for(int i = 0; i < used.total; i++)
	{
		if(!used.values[i])
		{
			if(!got_it)
				printf(_("BC_Theme::check_used: Images aren't used.\n"));
			printf("%s ", contents.values[i]);
			got_it = 1;
		}
	}
	if(got_it) printf("\n");
}












BC_ThemeSet::BC_ThemeSet(int total, int is_reference, char *title)
{
	this->total = total;
	this->title = title;
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
}





