#ifndef BCTUMBLE_H
#define BCTUMBLE_H

#include "bcsubwindow.h"

class BC_Tumbler : public BC_SubWindow
{
public:
	BC_Tumbler(int x, int y, VFrame **data = 0);
	virtual ~BC_Tumbler();

	virtual int handle_up_event() { return 0; };
	virtual int handle_down_event() { return 0; };
	int repeat_event(int64_t repeat_id);

	int initialize();
	int set_images(VFrame **data);
	int cursor_enter_event();
	int cursor_leave_event();
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int update_bitmaps(VFrame **data);
	int reposition_window(int x, int y);
	virtual void set_boundaries(int64_t min, int64_t max) {};
	virtual void set_boundaries(float min, float max) {};
	virtual void set_increment(float value) {};

private:
	int draw_face();

	BC_Pixmap *images[4];
	int status;
	int64_t repeat_count;
	VFrame **data;
};

class BC_ITumbler : public BC_Tumbler
{
public:
	BC_ITumbler(BC_TextBox *textbox, int64_t min, int64_t max, int x, int y);
	virtual ~BC_ITumbler();

	int handle_up_event();
	int handle_down_event();
	void set_increment(float value);
	void set_boundaries(int64_t min, int64_t max);

	int64_t min, max;
	int64_t increment;
	BC_TextBox *textbox;
};

class BC_FTumbler : public BC_Tumbler
{
public:
	BC_FTumbler(BC_TextBox *textbox, float min, float max, int x, int y);
	virtual ~BC_FTumbler();
	
	int handle_up_event();
	int handle_down_event();
	void set_boundaries(float min, float max);
	void set_increment(float value);

	float min, max;
	float increment;
	BC_TextBox *textbox;
};

#endif
