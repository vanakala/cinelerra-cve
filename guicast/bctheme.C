#include "bctheme.h"
#include "bcwindowbase.h"
#include "clip.h"
#include "vframe.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

BC_Theme::BC_Theme()
{
	char *path_env = getenv("PATH");
	char default_path[BCTEXTLEN];
	char *path_ptr = path_env;

// Get location of executable
	FILE *fd = fopen("/proc/self/cmdline", "r");
	if(!fd)
	{
		perror("BC_Theme::BC_Theme: can't open /proc/self/cmdline.\n");
		return;
	}
	else
	{
		fread(default_path, 1, BCTEXTLEN, fd);
		strcpy(path, default_path);
		fclose(fd);
	}

	int done = 0;
	fd = 0;
	do
	{
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
			strcat(path, default_path);
		}
		else
		{
			fclose(fd);
			done = 1;
		}
	}while(!done);


	data_buffer = 0;
	contents_buffer = 0;
	last_image = 0;
}

BC_Theme::~BC_Theme()
{
	for(int i = 0; i < image_sets.total; i++)
		delete image_sets.values[i];
	for(int i = 0; i < images.total; i++)
		delete images.values[i];
	if(data_buffer) delete [] data_buffer;
	if(contents_buffer) delete [] contents_buffer;
}

void BC_Theme::dump()
{
	printf("BC_Theme::dump 1 images=%d sets=%d path=%s contents=%d\n", 
		images.total, 
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

// These create image sets which are stored in the caches.
VFrame** BC_Theme::new_image_set(int total, ...)
{
	va_list list;
	BC_ThemeSet *result = new BC_ThemeSet(total);
	image_sets.append(result);


	va_start(list, total);
	for(int i = 0; i < total; i++)
	{
		char *path = va_arg(list, char*);
		result->data[i] = new_image(path);
	}
	va_end(list);

	return result->data;
}

VFrame** BC_Theme::new_button(char *overlay_path, 
	char *up_path, 
	char *hi_path, 
	char *dn_path)
{
	VFrame default_data(get_image_data(overlay_path));
	BC_ThemeSet *result = new BC_ThemeSet(3);

	result->data[0] = new_image(up_path);
	result->data[1] = new_image(hi_path);
	result->data[2] = new_image(dn_path);
	for(int i = 0; i < 3; i++)
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

VFrame* BC_Theme::new_image(char *title)
{
	VFrame *result = new VFrame(get_image_data(title));
	images.append(result);
	return result;
}

void BC_Theme::set_path(char *path)
{
	strcpy(this->path, path);
}

unsigned char* BC_Theme::get_image_data(char *title)
{
// Read contents
	if(!data_buffer)
	{
		FILE *fd = fopen(path, "r");

		if(!fd)
		{
			fprintf(stderr, "Theme::get_image: %s when opening %s\n", strerror(errno), path);
		}
		int data_offset, contents_offset;
		int total_bytes;
		int data_size;
		int contents_size;

		fseek(fd, -8, SEEK_END);
		total_bytes = ftell(fd);
		fread(&data_offset, 1, 4, fd);
		fread(&contents_offset, 1, 4, fd);


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
			}
			else
				i++;
		}
		fclose(fd);
	}

	if(last_image && !strcasecmp(last_image, title))
	{
		return (unsigned char*)(data_buffer + last_offset);
	}
	else
	for(int i = 0; i < contents.total; i++)
	{
		if(!strcasecmp(contents.values[i], title))
		{
			last_offset = offsets.values[i];
			last_image = contents.values[i];
			return (unsigned char*)(data_buffer + offsets.values[i]);
		}
	}

	fprintf(stderr, "Theme::get_image: %s not found.\n", title);
	return 0;
}











BC_ThemeSet::BC_ThemeSet(int total)
{
	data = new VFrame*[total];
}

BC_ThemeSet::~BC_ThemeSet()
{
	if(data) delete [] data;
}
