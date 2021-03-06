
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
		double maxvalue,
		int total_values, 
		int *value_positions, 
		int stick_x, 
		int stick_y, 
		double *values);
	virtual ~BC_Pan();

	void initialize();
	void update(int x, int y);
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();
	int cursor_enter_event();
	void cursor_leave_event();
	void repeat_event(int duration);
	virtual int handle_event() { return 0; };
// change radial positions of channels
	void change_channels(int new_channels, int *value_positions);
// update values from stick position
	void stick_to_values();
// Generic conversion from stick to values for channel changes with no GUI
	static void stick_to_values(double *values,
		int total_values, 
		int *value_positions, 
		int stick_x, 
		int stick_y,
		int virtual_r,
		double maxvalue);
	int get_total_values();
	double get_value(int channel);
	int get_stick_x();
	int get_stick_y();
	void set_images(VFrame **data);
	static void calculate_stick_position(int total_values, 
		int *value_positions, 
		double *values,
		double maxvalue,
		int virtual_r,
		int &stick_x,
		int &stick_y);
	static void rdtoxy(int &x, int &y, int a, int virtual_r);
	void activate(int popup_x = -1, int popup_y = -1);
	void deactivate();
	double *get_values();

private:
	void draw();
	void draw_popup();
// update values from stick position
	static double distance(int x1, int x2, int y1, int y2);
// get x and y positions of channels
	static void get_channel_positions(int *value_x, 
		int *value_y, 
		int *value_positions,
		int virtual_r,
		int total_values);

	int virtual_r;
	double maxvalue;
	int total_values;
	int *value_positions;
	int stick_x;
	int stick_y;
// Cursor origin on button press
	int x_origin, y_origin;
// Stick origin on button press
	int stick_x_origin, stick_y_origin;
	double *values;
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
