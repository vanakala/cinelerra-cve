#ifndef BCMETER_H
#define BCMETER_H

#include "bcmeter.inc"
#include "bcsubwindow.h"
#include "units.h"

#define METER_TYPES 2

#define METER_DB 0
#define METER_INT 1
#define METER_VERT 0
#define METER_HORIZ 1
#define METER_MARGIN 4

class BC_Meter : public BC_SubWindow
{
public:
	BC_Meter(int x, 
		int y, 
		int orientation, 
		int pixels, 
		float min = -40, 
		int mode = METER_DB, 
		int use_titles = 0,
		long over_delay = 150,
		long peak_delay = 15);
	virtual ~BC_Meter();

	int initialize();
	void set_images(VFrame **data);
	int set_delays(int over_delay, int peak_delay);
	int region_pixel(int region);
	int region_pixels(int region);
	virtual int button_press_event();

	static int get_title_w();
	static int get_meter_w();
	int update(float new_value, int over);
	int reposition_window(int x, int y, int pixels);
	int reset();
	int reset_over();
	int change_format(int mode, float min);

private:
	void draw_titles();
	void draw_face();
	int level_to_pixel(float level);
	void get_divisions();

	BC_Pixmap *images[TOTAL_METER_IMAGES];
	int orientation;
	int pixels;
	int low_division;
	int medium_division;
	int use_titles;
	int *title_pixel;
	char **db_titles;
	int meter_titles;
	float level, peak;
	int mode;
	DB db;
	int peak_timer;






	int peak_pixel, level_pixel, peak_pixel1, peak_pixel2;
	int over_count, over_timer;
	float min;
	long over_delay;       // Number of updates the over warning lasts.
	long peak_delay;       // Number of updates the peak lasts.
};

#endif
