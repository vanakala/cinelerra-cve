#include "bcbutton.h"
#include "bcmeter.h"
#include "bcpixmap.h"
#include "bcresources.h"
#include "bcwindow.h"
#include "colors.h"
#include "fonts.h"
#include "vframe.h"
#include <string.h>

#define METER_NORMAL 0
#define METER_GREEN 1
#define METER_RED 2
#define METER_YELLOW 3
#define METER_OVER 4

#define METER_LEFT 0
#define METER_MID 1
#define METER_RIGHT 3


BC_Meter::BC_Meter(int x, 
	int y, 
	int orientation, 
	int pixels, 
	float min, 
	int mode, 
	int use_titles,
	long over_delay,
	long peak_delay)
 : BC_SubWindow(x, y, -1, -1)
{
	this->use_titles = use_titles;
	this->over_delay = over_delay;
	this->peak_delay = peak_delay;
	this->min = min;
	this->mode = mode;
	this->orientation = orientation;
	this->pixels = pixels;
	for(int i = 0; i < TOTAL_METER_IMAGES; i++) images[i] = 0;
	title_pixel = 0;
	db_titles = 0;
	meter_titles = 0;
}

BC_Meter::~BC_Meter()
{
	if(db_titles)
	{
		for(int i = 0; i < meter_titles; i++) delete [] db_titles[i];
		delete [] db_titles;
	}
	if(title_pixel) delete [] title_pixel;
	for(int i = 0; i < TOTAL_METER_IMAGES; i++) delete images[i];
}

int BC_Meter::get_title_w()
{
	return get_resources()->meter_title_w;
}

int BC_Meter::get_meter_w()
{
	return get_resources()->ymeter_images[0]->get_w() + 2;
}


int BC_Meter::set_delays(int over_delay, int peak_delay)
{
	this->over_delay = over_delay;
	this->peak_delay = peak_delay;
	return 0;
}

int BC_Meter::initialize()
{
	peak_timer = 0;
	level_pixel = peak_pixel = 0;
	over_timer = 0;
	over_count = 0;
	peak = level = -100;

	if(orientation == METER_VERT)
	{
		set_images(get_resources()->ymeter_images);
		h = pixels;
		w = images[0]->get_w();
		if(use_titles) w += get_title_w();
	}
	else
	{
		set_images(get_resources()->xmeter_images);
		h = images[0]->get_h();
		w = pixels;
		if(use_titles) h += get_title_w();
	}

// calibrate the db titles
	get_divisions();

	BC_SubWindow::initialize();
	draw_titles();
	draw_face();
	return 0;
}

void BC_Meter::set_images(VFrame **data)
{
	for(int i = 0; i < TOTAL_METER_IMAGES; i++) delete images[i];
	for(int i = 0; i < TOTAL_METER_IMAGES; i++) 
		images[i] = new BC_Pixmap(parent_window, data[i], PIXMAP_ALPHA);
}

int BC_Meter::reposition_window(int x, int y, int pixels)
{
	if(pixels < 0) pixels = this->pixels;
	this->pixels = pixels;
	if(orientation == METER_VERT)
		BC_SubWindow::reposition_window(x, y, get_w(), pixels);
	else
		BC_SubWindow::reposition_window(x, y, pixels, get_h());

//printf("BC_Meter::reposition_window 1 %d %d %d %d\n", x, y, w, h);
	get_divisions();

//set_color(WHITE);
//draw_box(0, 0, w, h);
//flash();	
//return 0;
	draw_titles();
	draw_face();
	return 0;
}

int BC_Meter::reset()
{
	level = min;
	peak = min;
	level_pixel = peak_pixel = 0;
	peak_timer = 0;
	over_timer = 0;
	over_count = 0;
	draw_face();
	return 0;
}

int BC_Meter::button_press_event()
{
	if(cursor_inside() && is_event_win())
	{
		reset_over();
		return 1;
	}
	return 0;
}


int BC_Meter::reset_over()
{
	over_timer = 0;
	return 0;
}

int BC_Meter::change_format(int mode, float min)
{
	this->mode = mode;
	this->min = min;
	reposition_window(get_x(), get_y(), pixels);
	return 0;
}

int BC_Meter::level_to_pixel(float level)
{
	int result;
	if(mode == METER_DB)
	{
		result = pixels - 4 - (int)((level / min) * (pixels - 4));
		if(level == 0) result = pixels;
	}
	else
	{
		result = (int)(level * (pixels - 4));
	}
	
	return result;
}


void BC_Meter::get_divisions()
{
	int i;
	float j, j_step;
	float division, division_step;
	char string[1024];

	if(db_titles)
	{
		for(int i = 0; i < meter_titles; i++) delete [] db_titles[i];
		delete [] db_titles;
	}
	if(title_pixel) delete [] title_pixel;

	meter_titles = labs((int)(min / 5)) + 1;
	title_pixel = new int[meter_titles];

	if(use_titles)
	{
		db_titles = new char*[meter_titles];
		for(i = 0; i < meter_titles; i++) db_titles[i] = 0;
	}

	division = METER_MARGIN;
	division_step = (float)(pixels - METER_MARGIN * 3) / (meter_titles - 1);
	j = 0;     // number for title
	j_step = min / (meter_titles - 1);

	for(i = 0; i < meter_titles; i++)
	{

		if(use_titles)
		{
			sprintf(string, "%.0f", fabs(-j));
			db_titles[i] = new char[strlen(string) + 1];
			strcpy(db_titles[i], string);
		}

		title_pixel[i] = (int)(division); 

		division += division_step;
		j += j_step;
	}

// Fix divisions at 5 and 20
	if(meter_titles > 4)
	{
		medium_division = pixels - title_pixel[1];
		low_division = pixels - title_pixel[4];
	}
	else
// Boundary condition
	{
		medium_division = pixels - title_pixel[(meter_titles - 1) * 1 / 6];
		low_division = pixels - title_pixel[(meter_titles - 1) * 2 / 5];
	}
}

void BC_Meter::draw_titles()
{
	if(!use_titles) return;

	set_font(get_resources()->meter_font);

	if(orientation == METER_HORIZ)
	{
		draw_top_background(parent_window, 0, 0, get_w(), get_title_w());

		for(int i = 0; i < meter_titles; i++)
		{
			draw_text(0, title_pixel[i], db_titles[i]);
		}

		flash(0, 0, get_w(), get_title_w());
	}
	else
	if(orientation == METER_VERT)
	{
		draw_top_background(parent_window, 0, 0, get_title_w(), get_h());


		for(int i = 0; i < meter_titles; i++)
		{
// Tick marks
			if(i < meter_titles - 1)
			{
				for(int j = 0; j < 6; j++)
				{
					int y1;
					int y2;
					int y;

					y1 = title_pixel[i];
					y2 = title_pixel[i + 1];
					y = (int)((float)(y2 - y1) * j / 5 + 0.5) + y1;

					if(j == 0 || j == 5)
					{
						set_color(get_resources()->meter_font_color);
						draw_line(get_title_w() - 10 - 1, y, get_title_w() - 1, y);
						if(get_resources()->meter_3d)
						{
							set_color(BLACK);
							draw_line(get_title_w() - 10, y + 1, get_title_w(), y + 1);
						}
					}
					else
					{
						set_color(get_resources()->meter_font_color);
						draw_line(get_title_w() - 5 - 1, y, get_title_w() - 1, y);
						if(get_resources()->meter_3d)
						{
							set_color(BLACK);
							draw_line(get_title_w() - 5, y + 1, get_title_w(), y + 1);
						}
					}
				}
			}

			set_color(get_resources()->meter_font_color);
			if(i == 0)
				draw_text(0, title_pixel[i] + get_text_height(SMALLFONT_3D) / 2, db_titles[i]);
			else
				draw_text(0, title_pixel[i] + get_text_height(SMALLFONT_3D) / 2, db_titles[i]);
		}

		flash(0, 0, get_title_w(), get_h());
	}
}

int BC_Meter::region_pixel(int region)
{
	VFrame **reference_images = get_resources()->xmeter_images;
	int result;

	if(region == METER_RIGHT) 
		result = region * reference_images[0]->get_w() / 4;
	else
		result = region * reference_images[0]->get_w() / 4;

	return result;
}

int BC_Meter::region_pixels(int region)
{
	int x1;
	int x2;
	int result;
	VFrame **reference_images = get_resources()->xmeter_images;
	
	x1 = region * reference_images[0]->get_w() / 4;
	x2 = (region + 1) * reference_images[0]->get_w() / 4;
	if(region == METER_MID) 
		result = (x2 - x1) * 2;
	else 
		result = x2 - x1;
	return result;
}

void BC_Meter::draw_face()
{
	VFrame **reference_images = get_resources()->xmeter_images;
	int level_pixel = level_to_pixel(level);
	int peak_pixel2 = level_to_pixel(peak);
	int peak_pixel1 = peak_pixel2 - 2;
	int left_pixel = region_pixel(METER_MID);
	int right_pixel = pixels - region_pixels(METER_RIGHT);
	int pixel = 0;
	int image_number = 0, region = 0;
	int in_span, in_start;
	int x = use_titles ? get_title_w() : 0;
	int w = use_titles ? this->w - get_title_w() : this->w;

	draw_top_background(parent_window, x, 0, w, h);
//printf("BC_Meter::draw_face %d %d\n", w, h);

	while(pixel < pixels)
	{
// Select image to draw
		if(pixel < level_pixel ||
			(pixel >= peak_pixel1 && pixel < peak_pixel2))
		{
			if(pixel < low_division)
				image_number = METER_GREEN;
			else
			if(pixel < medium_division)
				image_number = METER_YELLOW;
			else
				image_number = METER_RED;
		}
		else
		{
			image_number = METER_NORMAL;
		}

// Select region of image to duplicate
		if(pixel < left_pixel)
		{
			region = METER_LEFT;
			in_start = pixel + region_pixel(region);
			in_span = region_pixels(region) - (in_start - region_pixel(region));
		}
		else
		if(pixel < right_pixel)
		{
			region = METER_MID;
			in_start = region_pixel(region);
			in_span = region_pixels(region);
		}
		else
		{
			region = METER_RIGHT;
			in_start = (pixel - right_pixel) + region_pixel(region);
			in_span = region_pixels(region) - (in_start - region_pixel(region));;
		}

//printf("BC_Meter::draw_face region %d pixel %d pixels %d in_start %d in_span %d\n", region, pixel, pixels, in_start, in_span);
		if(in_span > 0)
		{
// Clip length to peaks
			if(pixel < level_pixel && pixel + in_span > level_pixel)
				in_span = level_pixel - pixel;
			else
			if(pixel < peak_pixel1 && pixel + in_span > peak_pixel1)
				in_span = peak_pixel1 - pixel;
			else
			if(pixel < peak_pixel2 && pixel + in_span > peak_pixel2) 
				in_span = peak_pixel2 - pixel;

// Clip length to color changes
			if(image_number == METER_GREEN && pixel + in_span > low_division)
				in_span = low_division - pixel;
			else
			if(image_number == METER_YELLOW && pixel + in_span > medium_division)
				in_span = medium_division - pixel;

// Clip length to regions
			if(pixel < left_pixel && pixel + in_span > left_pixel)
				in_span = left_pixel - pixel;
			else
			if(pixel < right_pixel && pixel + in_span > right_pixel)
				in_span = right_pixel - pixel;

//printf("BC_Meter::draw_face image_number %d pixel %d pixels %d in_start %d in_span %d\n", image_number, pixel, pixels, in_start, in_span);
//printf("BC_Meter::draw_face %d %d %d %d\n", orientation, region, images[image_number]->get_h() - in_start - in_span);
			if(orientation == METER_HORIZ)
				draw_pixmap(images[image_number], 
					pixel, 
					x, 
					in_span + 1, 
					get_h(), 
					in_start, 
					0);
			else
				draw_pixmap(images[image_number],
					x,
					get_h() - pixel - in_span,
					get_w(),
					in_span + 1,
					0,
					images[image_number]->get_h() - in_start - in_span);

			pixel += in_span;
		}
	}

	if(over_timer)
	{
		if(orientation == METER_HORIZ)
			draw_pixmap(images[METER_OVER], 
				10, 
				2);
		else
			draw_pixmap(images[METER_OVER],
				x + 2,
				get_h() - 100);

		over_timer--;
	}

   	if(orientation == METER_HORIZ)
		flash(0, 0, pixels, get_h());
	else
		flash(x, 0, w, pixels);
}

int BC_Meter::update(float new_value, int over)
{
	peak_timer++;

	if(mode == METER_DB)
	{
		if(new_value == 0) 
			level = min;
		else
			level = db.todb(new_value);        // db value
	}

	if(level > peak || peak_timer > peak_delay)
	{
		peak = level;
		peak_timer = 0;
	}

//printf("BC_Meter::update %f\n", level);
	if(over) over_timer = over_delay;	
// only draw if window is visible

	draw_face();
	return 0;
}
