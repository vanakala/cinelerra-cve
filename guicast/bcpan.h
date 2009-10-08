
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

#ifndef BCPAN_H
#define BCPAN_H

// pan  angles
//
//        360/0
//
//     270      90
//
//         180

#include "bcsubwindow.h"
#include "rotateframe.inc"
#include "vframe.inc"

#define PAN_UP 0
#define PAN_HI 1
#define PAN_POPUP 2
#define PAN_CHANNEL 3
#define PAN_STICK 4
#define PAN_CHANNEL_SMALL 5
#define PAN_STICK_SMALL 6
#define PAN_IMAGES 7



class BC_Pan : public BC_SubWindow
{
public:
	BC_Pan(int x, 
		int y, 
		int virtual_r, 
		float maxvalue, 
		int total_values, 
		int *value_positions, 
		int stick_x, 
		int stick_y, 
		float *values);
	virtual ~BC_Pan();

	int initialize();
	int update(int x, int y);
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();
	int cursor_enter_event();
	int cursor_leave_event();
	int repeat_event(int64_t duration);
	virtual int handle_event() { return 0; };
// change radial positions of channels
	int change_channels(int new_channels, int *value_positions);
// update values from stick position
	int stick_to_values();
// Generic conversion from stick to values for channel changes with no GUI
	static int stick_to_values(float *values,
		int total_values, 
		int *value_positions, 
		int stick_x, 
		int stick_y,
		int virtual_r,
		float maxvalue);
	int get_total_values();
	float get_value(int channel);
	int get_stick_x();
	int get_stick_y();
	void set_images(VFrame **data);
	static void calculate_stick_position(int total_values, 
		int *value_positions, 
		float *values, 
		float maxvalue, 
		int virtual_r,
		int &stick_x,
		int &stick_y);
	static int rdtoxy(int &x, int &y, int a, int virtual_r);
	int activate(int popup_x = -1, int popup_y = -1);
	int deactivate();
	float* get_values();

private:
	void draw();
	void draw_popup();
// update values from stick position
	static float distance(int x1, int x2, int y1, int y2);
// get x and y positions of channels
	static int get_channel_positions(int *value_x, 
		int *value_y, 
		int *value_positions,
		int virtual_r,
		int total_values);

	int virtual_r;
	float maxvalue;
	int total_values;
	int *value_positions;
	int stick_x;
	int stick_y;
// Cursor origin on button press
	int x_origin, y_origin;
// Stick origin on button press
	int stick_x_origin, stick_y_origin;
	float *values;
	int highlighted;
// virtual x and y positions
	int *value_x, *value_y;     	
	int active;

// Used in popup
	BC_Pixmap *images[PAN_IMAGES];
	VFrame *temp_channel;
	RotateFrame *rotater;
	BC_Popup *popup;
};

#endif
