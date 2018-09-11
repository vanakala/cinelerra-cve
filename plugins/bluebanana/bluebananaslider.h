/*
 * Cinelerra :: Blue Banana - color modification plugin for Cinelerra-CV
 * Copyright (C) 2012-2013 Monty <monty@xiph.org>
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

#ifndef BLUEBANANASLIDER_H
#define BLUEBANANASLIDER_H

#include "pluginvclient.h"
#include "bluebanana.h"
#include "bluebananawindow.h"

class BluebananaSlider : public BC_SubWindow
{
public:
	BluebananaSlider(BluebananaMain *plugin,
		BluebananaWindow *gui,
		int x,
		int y,
		int w,
		int h,
		float minval,
		float maxval);
	~BluebananaSlider();

	virtual void update();
	virtual void render() { BluebananaSlider::update(); };
	void reposition(int, int, int, int);
	virtual void update_histogram(float *,float *, float *, float *, int pixels);

	int active_bordercolor;
	int inactive_bordercolor;
	int dimtextcolor;
	int troughcolor;
	int slidercolor;
	int needlecolor;
	int sliderlit;
	int needlelit;
	int outline;

protected:
	virtual void trough_color(float hdel, float vdel,
		float &r, float &g, float &b, float &a)
			{ r = g = b = a = 0; }
	virtual int handle_event() { return 1; }
	virtual int button_press_event() { return 1; }
	int button_release_event();
	virtual int cursor_motion_event() { return 1; }

	BluebananaMain *plugin;
	BluebananaWindow *gui;

	friend class BluebannanaSliderSingle;
	friend class BluebannanaSliderBracket;
	friend class BluebannanaSliderCircular;
	friend class BluebannanaSliderGamma;
	friend class BluebannanaSliderFill;

	int fraction_to_pixel(float input);
	float pixel_to_fraction(int pixel);
	float pixel_to_value(int pixel);
	int value_to_pixel(float gamma);
	int minval;
	int maxval;

	void draw_midslider(int x, int lit);
	void draw_leftslider(int x, int lit);
	void draw_rightslider(int x, int lit);
	void draw_overslider(int x, int lit);
	void draw_topslider(int x, int lit);
	void draw_bottomslider(int x, int lit);

	int in_leftslider2(int px, int x);
	int in_midslider2(int px, int x);
	int in_rightslider2(int px, int x);
	int in_overslider2(int px, int x, int y);
	int in_topslider2(int px, int x, int y);
	int in_bottomslider2(int px, int x, int y);

	int in_leftslider(int px, int x, int y);
	int in_rightslider(int px, int x, int y);
	int in_midslider(int px, int x, int y);
	int in_overslider(int px, int x, int y);
	int in_topslider(int px, int x, int y);
	int in_bottomslider(int px, int x, int y);

	int drag;
	int xoff;
	int light;
	int highlight;

protected:
	VFrame *trough;
	int troughcols;
	int troughlines;
	float *histval;
	float *histred;
	float *histgreen;
	float *histblue;
};

class BluebananaSliderSingle : public BluebananaSlider
{
public:
	BluebananaSliderSingle(BluebananaMain *plugin,
		BluebananaWindow *gui,
		int x,
		int y,
		int w,
		int h,
		float minval,
		float maxval);

	virtual void update();
	virtual void render() { BluebananaSliderSingle::update(); };

	friend class BluebananaHASlider;
	friend class BluebananaOASlider;

protected:
	float val;
	float increment;

	virtual void trough_color(float hdel, float vdel, float &r, float &g, float &b, float &a) {}
	virtual int handle_event() { return 1; }

	virtual int button_press_event();
	virtual int cursor_motion_event();

	void set_val(float val);
};

class BluebananaSliderBracket : public BluebananaSlider
{
public:
	BluebananaSliderBracket(BluebananaMain *plugin,
		BluebananaWindow *gui,
		int x,
		int y,
		int w,
		int h,
		float minval,
		float maxval);

	virtual void update();
	virtual void render() { BluebananaSliderBracket::update(); };

	friend class BluebananaSSSlider;
	friend class BluebananaVSSlider;

protected:
	float loval;
	float midval;
	float hival;
	float overval;
	float increment;

	virtual void trough_color(float hdel, float vdel, float &r, float &g, float &b, float &a) {}
	virtual int handle_event() { return 1; }

	int button_press_event();
	int cursor_motion_event();

	virtual void set_lo(float val);
	virtual void set_mid(float val);
	virtual void set_hi(float val);
	virtual void set_delta(float val);
	void set_over(float val);

};

class BluebananaSliderCircular : public BluebananaSliderBracket
{
public:
	BluebananaSliderCircular(BluebananaMain *plugin,
		BluebananaWindow *gui,
		int x,
		int y,
		int w,
		int h,
		float minval,
		float maxval);

	virtual void update();

	friend class BluebananaHSSlider;

	virtual void trough_color(float hdel, float vdel, float &r, float &g, float &b, float &a) {}
	virtual int handle_event() { return 1; }

	void set_lo(float val);
	void set_mid(float val);
	void set_hi(float val);
	void set_delta(float val);
};

class BluebananaSliderChannel : public BluebananaSlider
{
public:
	BluebananaSliderChannel(BluebananaMain *plugin,
		BluebananaWindow *gui,
		int x,
		int y,
		int w,
		int h);

	virtual void update();
	virtual void render()
	{ BluebananaSliderChannel::update(); };
	virtual void update_histogram(float *,float *, float *, float *,int pixels);

	friend class BluebananaSASlider;
	friend class BluebananaVASlider;
	friend class BluebananaRASlider;
	friend class BluebananaGASlider;
	friend class BluebananaBASlider;
	friend class BluebananaYASlider;

	float increment;
	float zero;

protected:

	float loval;
	float gamma;
	float hival;

	virtual void trough_color(float hval, float &r, float &g, float &b) {};
	virtual int handle_event() { return 1; }

	int button_press_event();
	int cursor_motion_event();

	void set_lo(float lo);
	void set_hi(float hi);
	void set_mid(float mid);
	void set_range(float range);
	void set_gamma(float val);

	float pixel_to_gamma(float pixel);
	float gamma_to_pixel(float gamma);
	float pixel_to_value(float pixel);
	float value_to_pixel(float gamma);

private:
	int t_x0;
	int t_x1;
	int t_x2;
	int t_x3;
	int t_w01;
	int t_w12;
	int t_w23;
};

class BluebananaSliderFill : public BluebananaSlider
{
public:
	BluebananaSliderFill(BluebananaMain *plugin,
		BluebananaWindow *gui,
		int x,
		int y,
		int w,
		int h,
		float minval,
		float maxval,
		float maxover);

	virtual void update();
	virtual void render() { BluebananaSliderFill::update(); };

	friend class BluebananaFSSlider;
	float increment;

protected:
	float loval;
	float midval;
	float hival;
	float overval;

	virtual void trough_color(float hdel, float vdel, float &r, float &g, float &b, float &a) {};
	virtual int handle_event() { return 1; }

	int button_press_event();
	int cursor_motion_event();

	void set_fill(float lo, float mid, float hi);
	void set_over(float val);
	void set_delta(float val);

	float pixel_to_over(int pixel);
	int over_to_pixel(float over);

	float maxover;
};

#endif
