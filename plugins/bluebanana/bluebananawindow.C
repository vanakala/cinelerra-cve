// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2012 Monty <monty@xiph.org>
// Cinelerra :: Blue Banana - color modification plugin for Cinelerra-CV

#include <unistd.h>
#include <math.h>
#include "bcsignals.h"
#include "bctitle.h"
#include "cursors.h"
#include "bluebanana.h"
#include "bluebananaconfig.h"
#include "bluebananaslider.h"
#include "bluebananawindow.h"
#include "keys.h"
#include "language.h"
#include "brender.h"

#include "bluebananacolor.c"

PLUGIN_THREAD_METHODS

class BluebananaHActive : public BC_CheckBox
{
public:
	BluebananaHActive(BluebananaMain *plugin, BluebananaWindow *gui);

	virtual int handle_event();
	void update();

	BluebananaMain *plugin;
	BluebananaWindow *gui;
};

class BluebananaSActive : public BC_CheckBox
{
public:
	BluebananaSActive(BluebananaMain *plugin, BluebananaWindow *gui);

	virtual int handle_event();
	void update();

	BluebananaMain *plugin;
	BluebananaWindow *gui;
};

class BluebananaVActive : public BC_CheckBox
{
public:
	BluebananaVActive(BluebananaMain *plugin, BluebananaWindow *gui);

	virtual int handle_event();
	void update();

	BluebananaMain *plugin;
	BluebananaWindow *gui;
};


// -------------------------------------------------------------------------------
// The below code is because the stock TextTumbler trips too many bugs
// in guicast to be useful. If this plugin is to be shipped standalone
// and work, we need an independent version that avoids the bugs here.

BB_Tumble::BB_Tumble(BluebananaMain *plugin,
	BluebananaWindow *gui,
	float min,
	float mid,
	float max,
	int precision,
	float increment,
	int text_w)
 : BC_TextBox(-1, -1, text_w, 1, mid, 1, MEDIUMFONT, precision)
{
	// We must pass in the precision for initialization to get the
	// geometry right.  But then bctextbox.C immediately clobbers it, so
	// set it again
	set_precision(precision);

	this->gui = gui;
	this->plugin = plugin;
	this->x = -1;
	this->y = -1;
	this->min = min;
	this->mid = mid;
	this->max = max;
	this->text_w = text_w;
	this->precision = precision;
	this->increment = increment;
	this->active = 0;
	this->suppress_events = 0;
}

float BB_Tumble::get_value()
{
	char *text = get_text();
	if(!text || text[0] == 0) return 0;
	return atof(text);
}

int BB_Tumble::handle_event()
{
	if(!suppress_events) value_event();
	return 1;
}

void BB_Tumble::activate()
{
	// we want to restore the previous value on ESC
	prev = get_value();
	// the textbox active member is private, make our own
	active = 1;
	BC_TextBox::activate();
}

void BB_Tumble::deactivate()
{
	if(active)
	{
		// only fire an event if the value changed; this makes 0/360
		// behavior in the hue selection readout stick with what the user
		// actually sets (as opposed to sanity checking flipping the value
		// if an event fires)
		if(get_value() != prev)
			value_event();
		active = 0;
		suppress_events = 0;
	}
	BC_TextBox::deactivate();
}

int BB_Tumble::button_press_event()
{
	if(is_event_win())
	{
		if(get_buttonpress() < 4)
			return BC_TextBox::button_press_event();
		if(get_buttonpress() == 4)
		{
			float v = get_value() + increment;
			if(v > max) v = max;
			update(v);
			value_event();
		}
		else if(get_buttonpress() == 5)
		{
			float v = get_value() - increment;
			if(v < min) v = min;
			update(v);
			value_event();
		}
		return 1;
	}
	return 0;
}

int BB_Tumble::keypress_event()
{
	if(active && get_keypress() == ESC)
		update(prev);

	// don't fire update events from every keypress when editing; that
	// will trigger slider updates, which will then sanity-check/clamp
	// values, and then update/clobber the readout text while we're
	// editing it.
	suppress_events = 1;
	int ret = BC_TextBox::keypress_event();
	suppress_events = 0;
	return ret;
}

// --------------------------------------------------------------------------------
//  initialization:
//    1) constructor constructs sliders and readouts with
//       limits but no values
//    2) constructor then calls slider update()
//       update computes and sets slider vals from config
//       update computes and sets readout vals from config
//
//  slider drag event:
//    1) slider calls handle_event(). This updates config.
//    2) slider calls update()
//       update computes and [re]sets slider vals from config;
//          this is where the snap-to-value behavior comes from
//       update computes and sets readout vals from config
//
//  readout event:
//    1) readout calls handle_event(). This updates config.
//    2) readout calls slider update() directly
//       slider update computes and sets slider vals from config
//       slider update computes and [re]sets readout vals from config;
//           this does not result in a further handle_event() call, so
//           no infinite recursion.
//
//  importantly, readout and slider values are set from the config
//  (and vice-versa) in only one place each.

// ---------------------------- hue adjustment slider -------------------------------

class BluebananaHAReadout : public BB_Tumble
{
public:
	BluebananaHAReadout(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, rint(-180), 0, rint(180), 0, 1, w) {}
	int value_event();
};

class BluebananaHASlider : public BluebananaSliderSingle
{
public:
	BluebananaHASlider(BluebananaMain *plugin, BluebananaWindow *gui,
			int x, int y, int w, int h)
		: BluebananaSliderSingle(plugin, gui, x, y, w, h, -180, 180) {}
	virtual int handle_event()
	{
		plugin->config.Hadj_val = val;
		return 1;
	}
	void reset()
	{
		plugin->config.Hadj_val = 0;
		update();
	}
	void update()
	{
		val = plugin->config.Hadj_val;
		highlight = plugin->config.active && plugin->config.Hadj_active;
		gui->Hadj_readout->update(plugin->config.Hadj_val);
		gui->slider_labels[7]->set_color(highlight && plugin->config.Hadj_val ?
			get_resources()->default_text_color : dimtextcolor);
		gui->enter_config_change();
		gui->commit_config_change();
	}
	void trough_color(float hdel, float vdel, float &r, float &g, float &b, float &a)
	{
		float Hshift = plugin->config.Hsel_active ? (plugin->config.Hsel_lo + plugin->config.Hsel_hi) / 720.f - .5f : 0.f;
		float deg = hdel + Hshift;
		if(deg < 0)deg += 1;
		if(deg > 1)deg -= 1;
		HSpV_to_RGB(deg * 6.f, 1., .2, r, g, b);
		a = 1.;
	}
};

int BluebananaHAReadout::value_event()
{
	float val = get_value();
	if(val < -180) val = -180;
	if(val > 180) val = 180;
	plugin->config.Hadj_val = val;
	gui->Hadj_slider->update();
	return 1;
}

// ------------------------------ Filter selection slider --------------------------------
class BluebananaFSReadout0 : public BB_Tumble
{
public:
	BluebananaFSReadout0(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, -FSrange, 0, FSrange, 0, 1, w) {}
	int value_event();
};
class BluebananaFSReadout1 : public BB_Tumble
{
public:
	BluebananaFSReadout1(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, -FSrange, 0, FSrange, 0, 1, w) {}
	int value_event();
};
class BluebananaFSReadout2 : public BB_Tumble
{
public:
	BluebananaFSReadout2(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, -FSrange, 0, FSrange, 0, 1, w) {}
	int value_event();
};
class BluebananaFSReadout3 : public BB_Tumble
{
public:
	BluebananaFSReadout3(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 0, FSovermax, 0, 1, w) {}
	int value_event();
};

static void paint_dot(float *array, int w, int h, float cx, float cy, float r, int inv)
{
	int x, y;
	int x0 = floor(cx - r);
	int x1 = ceil(cx + r) + 1;
	int y0 = floor(cy - r);
	int y1 = ceil(cy + r) + 1;

	if(x0 < 0) x0 = 0;
	if(x1 < 0) x1 = 0;
	if(y0 < 0) y0 = 0;
	if(y1 < 0) y1 = 0;

	if(x0 > w) x0 = w;
	if(x1 > w) x1 = w;
	if(y0 > h) y0 = h;
	if(y1 > h) y1 = h;

	for(y = y0; y < y1; y++)
	{
		float *row = array + y * w;
		for(x = x0; x < x1; x++)
		{
			float rr = hypotf(x - cx, y - cy);
			// not correct, but this is merely cosmetic anyway 
			float v = (r - rr + .5);
			if(v > 0)
			{
				if(inv)
				{
					row[x] -= v;
					if(row[x] > 1.)row[x] = 0.;
				}
				else
				{
					row[x] += v;
					if(row[x] > 1.)row[x] = 1.;
				}
			}
		}
	}
}

class BluebananaFSSlider : public BluebananaSliderFill
{
public:
	BluebananaFSSlider(BluebananaMain *plugin, BluebananaWindow *gui,
			int x, int y, int w, int h)
		: BluebananaSliderFill(plugin, gui, x, y, w, h, -FSrange, FSrange, FSovermax)
	{
		trough_alpha = 0;
		recompute_trough_alpha = 1;
		erode = -1;
	}

	~BluebananaFSSlider()
	{
		if(trough_alpha) delete[] trough_alpha;
	}

	virtual int handle_event()
	{
		plugin->config.Fsel_lo = (int)rint(loval);
		plugin->config.Fsel_mid = (int)rint(midval);
		plugin->config.Fsel_hi = (int)rint(hival);
		plugin->config.Fsel_over = (int)rint(overval);
		recompute_trough_alpha = 1;
		return 1;
	}

	void update()
	{
		if(plugin->config.Fsel_lo > 0)plugin->config.Fsel_lo = 0;
		if(plugin->config.Fsel_hi < 0)plugin->config.Fsel_hi = 0;

		if(highlight != plugin->config.Fsel_active ||
				erode != plugin->config.Fsel_erode ||
				loval != plugin->config.Fsel_lo ||
				midval != plugin->config.Fsel_mid ||
				hival != plugin->config.Fsel_hi ||
				overval != plugin->config.Fsel_over)
		{
			recompute_trough_alpha = 1;
		}
		erode = plugin->config.Fsel_erode;
		loval = plugin->config.Fsel_lo;
		midval = plugin->config.Fsel_mid;
		hival = plugin->config.Fsel_hi;
		overval = plugin->config.Fsel_over;
		highlight = plugin->config.Fsel_active;
		gui->Fsel_readout0->update(plugin->config.Fsel_lo);
		gui->Fsel_readout1->update(plugin->config.Fsel_mid);
		gui->Fsel_readout2->update(plugin->config.Fsel_hi);
		gui->Fsel_readout3->update(plugin->config.Fsel_over);
		gui->slider_labels[3]->set_color(highlight &&
			(plugin->config.Hsel_active || plugin->config.Ssel_active || plugin->config.Vsel_active) &&
			(plugin->config.Fsel_lo  != 0 ||
			plugin->config.Fsel_mid != 0 ||
			plugin->config.Fsel_hi  != 0 ||
			plugin->config.Fsel_over != 0 ) ?
			get_resources()->default_text_color : dimtextcolor);

		gui->erode_label->set_color (highlight && plugin->config.Fsel_lo &&
			plugin->config.Fsel_hi &&
			(plugin->config.Hsel_active ||
			plugin->config.Ssel_active || plugin->config.Vsel_active) ?
			get_resources()->default_text_color : dimtextcolor);

		gui->enter_config_change();
		gui->commit_config_change();
	}

	void trough_color(float hdel, float vdel, float &r, float &g, float &b, float &a)
	{
		int x = rint(hdel * troughcols - .5);
		int y = rint(vdel * troughlines - .5);
		float deg = plugin->config.Hsel_active ?
			vdel * (plugin->config.Hsel_hi - plugin->config.Hsel_lo) + plugin->config.Hsel_lo :
			vdel * 360.f;
		float sat = plugin->config.Ssel_active ?
			(plugin->config.Ssel_hi + plugin->config.Ssel_lo) / 200. : .5;
		float val = plugin->config.Vsel_active ?
			(plugin->config.Vsel_hi * 3 + plugin->config.Vsel_lo) / 400 : .75;

		if(deg >= 360)deg -= 360.f;
		HSpV_to_RGB(deg / 60., sat, val, r, g, b);
		a = trough_alpha[troughcols * y + x];
	}

	void render()
	{

		if(!trough_alpha)
			trough_alpha = new float[troughcols * troughlines];

		if(recompute_trough_alpha)
		{
			int trough_border = FSrange;
			int tw = troughcols * 3 + trough_border * 2;
			int th = troughlines * 3 + trough_border * 2;

			float work1[tw * th];
			float work2[tw * th];

			memset(work1, 0, sizeof(work1));

			float loval = 1;
			float hival = FSrange * 2 - 1;
			float y0 = (th - 1) / 2.;
			float y1 = (th - 1) / 2.;

			int spacing = rint(FSrange) * 2;
			int rowflag = 0;
			int x, y;

			while((y0 + spacing * .5) > 0)
			{
				for(x = (rowflag ? spacing / 2. : 0.); x - spacing * .5 < tw; x += spacing)
				{
					float r = (((float)x / tw) * (hival - loval) + loval) * .5;
					if(y0 == y1)
					{
						paint_dot(work1, tw, th, x, y0, r, 0);
					}
					else
					{
						paint_dot(work1, tw, th, x, y0, r, 0);
						paint_dot(work1, tw, th, x, y1, r, 0);
					}
				}
				y0 -= spacing / 2.;
				y1 += spacing / 2.;
				rowflag = (rowflag + 1) & 1;
			}

			float *final = work1;
			if(plugin->config.Fsel_active &&
					(plugin->config.Fsel_lo || plugin->config.Fsel_hi || plugin->config.Fsel_over))
				final = plugin->fill_selection(work1, work2, tw, th, NULL);

			// subsample into trough
			float *in = final + (tw + 1) * trough_border;
			for(y = 0; y < troughlines; y++)
			{
				float *out = trough_alpha + troughcols * y;
				for(x = 0; x < troughcols; x++)
					out[x] = (in[x * 3] + in[x * 3 + 1] + in[x * 3 + 2] +
						in[tw + x * 3] + in[tw + x * 3 + 1] + in[x * 3 + 2] +
						in[tw * 2 + x * 3] + in[tw * 2 + x * 3 + 1] + in[tw * 2 + x * 3 + 2]) * .1111;
				in += tw * 3;
			}

		}
		recompute_trough_alpha = 0;
		BluebananaSliderFill::update();
	}

	float *trough_alpha;
	int trough_border;
	int recompute_trough_alpha;
	int erode;
};

int BluebananaFSReadout0::value_event()
{
	float val = rint(get_value());
	if(val < -FSrange) val = -FSrange;
	if(val > 0)val = 0;
	if(val > plugin->config.Fsel_mid) val = plugin->config.Fsel_mid;
	plugin->config.Fsel_lo = val;
	gui->Fsel_slider->update();
	return 1;
}

int BluebananaFSReadout1::value_event()
{
	float val = rint(get_value());
	if(val < -FSrange) val = -FSrange;
	if(val > FSrange) val = FSrange;
	if(val < plugin->config.Fsel_lo) plugin->config.Fsel_lo = val;
	if(val > plugin->config.Fsel_hi) plugin->config.Fsel_hi = val;
	plugin->config.Fsel_mid = val;
	gui->Fsel_slider->update();
	return 1;
}

int BluebananaFSReadout2::value_event()
{
	float val = rint(get_value());
	if(val < 0) val = 0;
	if(val > FSrange)val = FSrange;
	if(val < plugin->config.Fsel_mid) val = plugin->config.Fsel_mid;
	plugin->config.Fsel_hi = val;
	gui->Fsel_slider->update();
	return 1;
}

int BluebananaFSReadout3::value_event()
{
	float val = rint(get_value());
	if(val < 0) val = 0;
	if(val > FSovermax)val = FSovermax;
	plugin->config.Fsel_over = val;
	gui->Fsel_slider->update();
	return 1;
}

// ------------------------------ value selection slider --------------------------------
class BluebananaVSReadout0 : public BB_Tumble
{
public:
	BluebananaVSReadout0(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 0, 100., 0, 1, w) {}
	int value_event();
};
class BluebananaVSReadout1 : public BB_Tumble
{
public:
	BluebananaVSReadout1(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 0, 100., 0, 1, w) {}
	int value_event();
};
class BluebananaVSReadout2 : public BB_Tumble
{
public:
	BluebananaVSReadout2(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 0, 100., 0, 1, w) {}
	int value_event();
};

class BluebananaVSSlider : public BluebananaSliderBracket
{
public:
	BluebananaVSSlider(BluebananaMain *plugin, BluebananaWindow *gui,
			int x, int y, int w, int h)
		: BluebananaSliderBracket(plugin, gui, x, y, w, h, 0, 100) { }

	virtual int handle_event()
	{
		plugin->config.Vsel_lo = rint(loval);
		plugin->config.Vsel_hi = rint(hival);
		plugin->config.Vsel_over = rint(overval);
		return 1;
	}

	void pick()
	{
		int delta = plugin->config.Vsel_hi - plugin->config.Vsel_lo;
		double r, g, b;
		float h, s, v;

		plugin->get_picker_colors(&r, &g, &b);
		RGB_to_HSpV(r, g, b, h, s, v);
		h *= 60.f;
		v = rint(v * 100.f);
		if(v < 0)v = 0;
		if(v > 100)v = 100;
		if(delta > 25)delta = 25;
		int lo = v - delta / 2;
		int hi = lo + delta;
		// shrink the brackets if necessary
		if(lo < 0)
		{
			lo = 0;
			if(hi - lo < 10) hi = 10;
		}
		if(hi > 100)
		{
			hi = 100;
			if(hi - lo < 10) lo = 90;
		}
		plugin->config.Vsel_lo = lo;
		plugin->config.Vsel_hi = hi;
		plugin->config.Vsel_active = 1;
		gui->Vsel_active->update(); // this will also call the slider update
	}

	void update()
	{
		loval = plugin->config.Vsel_lo;
		hival = plugin->config.Vsel_hi;
		midval = (loval + hival) / 2.f;
		overval = plugin->config.Vsel_over;
		highlight = plugin->config.Vsel_active;

		gui->Vsel_readout0->update(plugin->config.Vsel_lo);
		gui->Vsel_readout1->update(plugin->config.Vsel_hi);
		gui->Vsel_readout2->update(plugin->config.Vsel_over);
		gui->slider_labels[2]->set_color (highlight &&
			(plugin->config.Vsel_lo != 0 || plugin->config.Vsel_hi != 100) ?
			get_resources()->default_text_color : dimtextcolor);

		gui->enter_config_change();
		if(gui->Fsel_slider)gui->Fsel_slider->update();
		gui->commit_config_change();
	}

	void trough_color(float hdel, float vdel, float &r, float &g, float &b, float &a)
	{
		float deg = plugin->config.Hsel_active ?
			vdel * (plugin->config.Hsel_hi - plugin->config.Hsel_lo) + plugin->config.Hsel_lo :
			vdel * 360.f;
		float sat = plugin->config.Ssel_active ?
			(plugin->config.Ssel_hi * 3 + plugin->config.Ssel_lo) / 400. : .5;

		if(deg >= 360) deg -= 360.f;
		HSpV_to_RGB(deg / 60., sat, hdel, r, g, b);
		a = plugin->val_select_alpha(hdel);
	}
};

int BluebananaVSReadout0::value_event()
{
	float val = get_value();
	if(val < 0) val = 0;
	if(val > 100) val = 100;
	if(val > plugin->config.Vsel_hi) val = plugin->config.Vsel_hi;
	plugin->config.Vsel_lo = val;
	gui->Vsel_slider->update();
	return 1;
}

int BluebananaVSReadout1::value_event()
{
	float val = get_value();
	if(val < 0)val = 0;
	if(val > 100)val = 100;
	if(val < plugin->config.Vsel_lo) val = plugin->config.Vsel_lo;
	plugin->config.Vsel_hi = val;
	gui->Vsel_slider->update();
	return 1;
}

int BluebananaVSReadout2::value_event()
{
	float val = get_value();
	if(val < 0)val = 0;
	if(val > 100)val = 100;
	plugin->config.Vsel_over = val;
	gui->Vsel_slider->update();
	return 1;
}

// ----------------------------- saturation selection slider -----------------------------
class BluebananaSSReadout0 : public BB_Tumble
{
public:
	BluebananaSSReadout0(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 0, 100., 0, 1, w) {}
	int value_event();
};
class BluebananaSSReadout1 : public BB_Tumble
{
public:
	BluebananaSSReadout1(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 0, 100., 0, 1, w) {}
	int value_event();
};
class BluebananaSSReadout2 : public BB_Tumble
{
public:
	BluebananaSSReadout2(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 0, 100., 0, 1, w) {}
	int value_event();
};

class BluebananaSSSlider : public BluebananaSliderBracket
{
public:
	BluebananaSSSlider(BluebananaMain *plugin, BluebananaWindow *gui,
			int x, int y, int w, int h)
		: BluebananaSliderBracket(plugin, gui, x, y, w, h, 0, 100) { }

	int handle_event()
	{
		plugin->config.Ssel_lo = rint(loval);
		plugin->config.Ssel_hi = rint(hival);
		plugin->config.Ssel_over = rint(overval);
		return 1;
	}

	void pick()
	{
		int delta = plugin->config.Ssel_hi - plugin->config.Ssel_lo;
		double r, g, b;
		float h, s, v;

		plugin->get_picker_colors(&r, &g, &b);
		RGB_to_HSpV(r, g, b, h, s, v);
		h *= 60.f;
		s = rint(s * 100.f);
		if(s < 0) s = 0;
		if(s > 100) s = 100;
		if(delta > 25) delta = 25;
		int lo = s - delta / 2;
		int hi = lo + delta;
		// shrink the brackets if necessary
		if(lo < 0)
		{
			lo = 0;
			if(hi - lo < 10)hi = 10;
		}
		if(hi > 100)
		{
			hi = 100;
			if(hi - lo < 10)lo = 90;
		}
		plugin->config.Ssel_lo = lo;
		plugin->config.Ssel_hi = hi;
		plugin->config.Ssel_active = 1;
		gui->Ssel_active->update(); // this will also call the slider update
	}

	void update()
	{
		loval = plugin->config.Ssel_lo;
		hival = plugin->config.Ssel_hi;
		midval = (loval + hival) / 2.f;
		overval = plugin->config.Ssel_over;
		highlight = plugin->config.Ssel_active;

		gui->Ssel_readout0->update(plugin->config.Ssel_lo);
		gui->Ssel_readout1->update(plugin->config.Ssel_hi);
		gui->Ssel_readout2->update(plugin->config.Ssel_over);
		gui->slider_labels[1]->set_color(highlight  &&
			(plugin->config.Ssel_lo != 0 || plugin->config.Ssel_hi != 100) ?
			 get_resources()->default_text_color : dimtextcolor);

		gui->enter_config_change();
		if(gui->Fsel_slider)gui->Fsel_slider->update();
		gui->commit_config_change();
	}

	void trough_color(float hdel, float vdel, float &r, float &g, float &b, float &a)
	{
		float deg = plugin->config.Hsel_active ?
			vdel * (plugin->config.Hsel_hi - plugin->config.Hsel_lo) + plugin->config.Hsel_lo :
			vdel * 360.;
		if(deg >= 360) deg -= 360.f;
		HSpV_to_RGB(deg / 60.f, hdel, .7 + .3 * hdel, r, g, b);
		a = plugin->sat_select_alpha(hdel);
	}
};

int BluebananaSSReadout0::value_event()
{
	float val = get_value();
	if(val < 0) val = 0;
	if(val > 100) val = 100;
	if(val > plugin->config.Ssel_hi) val = plugin->config.Ssel_hi;
	plugin->config.Ssel_lo = val;
	gui->Ssel_slider->update();
	return 1;
}

int BluebananaSSReadout1::value_event()
{
	float val = get_value();
	if(val < 0) val = 0;
	if(val > 100) val = 100;
	if(val < plugin->config.Ssel_lo) val = plugin->config.Ssel_lo;
	plugin->config.Ssel_hi = val;
	gui->Ssel_slider->update();
	return 1;
}

int BluebananaSSReadout2::value_event()
{
	float val = get_value();
	if(val < 0) val = 0;
	if(val > 100) val = 100;
	plugin->config.Ssel_over = val;
	gui->Ssel_slider->update();
	return 1;
}

// ----------------------------- hue selection slider ---------------------------------
class BluebananaHSReadout0 : public BB_Tumble
{
public:
	BluebananaHSReadout0(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 0, 360., 0, 1, w) {}
	int value_event();
};

class BluebananaHSReadout1 : public BB_Tumble
{
public:
	BluebananaHSReadout1(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 0, 360., 0, 1, w) {}
	int value_event();
};

class BluebananaHSReadout2 : public BB_Tumble
{
public:
	BluebananaHSReadout2(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 360., 360., 0, 1, w) {}
	int value_event();
};

class BluebananaHSSlider : public BluebananaSliderCircular
{
public:
	BluebananaHSSlider(BluebananaMain *plugin, BluebananaWindow *gui,
			int x, int y, int w, int h)
		: BluebananaSliderCircular(plugin, gui, x, y, w, h, 0, 360) {}

	int handle_event()
	{
		plugin->config.Hsel_lo = rint(loval);
		plugin->config.Hsel_hi = plugin->config.Hsel_lo +
			(midval < loval ? rint(midval * 2 - loval * 2 + 720) : rint(midval * 2 - loval * 2));
		plugin->config.Hsel_over = rint(overval);
		return 1;
	}

	void pick()
	{
		int delta = plugin->config.Hsel_hi - plugin->config.Hsel_lo;
		double r, g, b;
		float h, s, v;

		plugin->get_picker_colors(&r, &g, &b);
		RGB_to_HSpV(r, g, b, h, s, v);
		h = rint(h * 60.f);
		if(h < 0) h = 0;
		if(h > 360) h = 360;
		if(delta > 30) delta = 30;
		int lo = h - delta / 2;
		int hi = lo + delta;
		if(lo < 0)
		{
			lo += 360;
			hi += 360;
		}
		plugin->config.Hsel_lo = lo;
		plugin->config.Hsel_hi = hi;
		plugin->config.Hsel_active = 1;
		gui->Hsel_active->update(); // this will also call the slider update
	}

	void update()
	{
		// Significantly more complex than the other sliders due to the
		// circular scale.
		float delta = (plugin->config.Hsel_hi - plugin->config.Hsel_lo);
		loval = plugin->config.Hsel_lo;
		overval = plugin->config.Hsel_over;
		float newhival = plugin->config.Hsel_hi;
		float newmidval = (loval + newhival) / 2.;
		if(loval < 0) loval += 360.;
		if(loval > 360.) loval -= 360.;
		if(newmidval < 0) newmidval += 360.;
		if(newmidval > 360.) newmidval -= 360.;
		if(newhival < 0) newhival += 360.;
		if(newhival > 360.) newhival -= 360.;
		highlight = plugin->config.Hsel_active;

		float checkhi = plugin->config.Hsel_hi;
		if(checkhi > 360) checkhi -= 360;

		// one last weirdness; 0 and 360 are technically the same value on
		// the circular scale, but the user can set either.  This isn't a
		// problem with lo as it doesn't wrap, but mid/hi is computed for
		// the slider and could end up at 0 or 360, and then clobber the
		// slider/readout.  To avoid annoying the user, don't override an
		// existing readout/slider setting with its equivalent.
		if(newhival == 0 && hival == 360.)
		{
			newhival = 360.;
			checkhi = 360;
		}
		else if(newhival == 360 && hival == 0.)
		{
			newhival = 0.;
			checkhi = 0;
		}
		else if(checkhi == 0 && gui->Hsel_readout2->get_value() == 360)
		{
			newhival = 360.;
			checkhi = 360;
		}
		else if(checkhi == 360 && gui->Hsel_readout2->get_value() == 0)
		{
			newhival = 0.;
			checkhi = 0;
		}

		if(newmidval < 1 && midval > 359)
		{
			newmidval = 360.;
		}
		else if(newmidval > 359.f && midval < 1.)
		{
			newmidval = 0.;
		}
		midval = newmidval;
		hival = newhival;

		gui->Hsel_readout0->update(plugin->config.Hsel_lo);
		gui->Hsel_readout1->update(checkhi);
		gui->Hsel_readout2->update(plugin->config.Hsel_over);
		gui->slider_labels[0]->set_color(highlight &&
			(plugin->config.Hsel_hi - plugin->config.Hsel_lo != 360) ?
			get_resources()->default_text_color : dimtextcolor);

		gui->enter_config_change();
		if(gui->Fsel_slider) gui->Fsel_slider->update();
		gui->commit_config_change();
	}

	void trough_color(float hdel, float vdel, float &r, float &g, float &b, float &a)
	{
		float val, sat, deg = hdel * 360.f;
		if(deg >= 360) deg -= 360.f;

		HSpV_to_RGB(deg / 60.f, 1.f, 1.f, r, g, b);
		a = plugin->hue_select_alpha(hdel * 360.f);
	}
	friend class BluebananaHSReadout1;
	friend class BluebananaHSReadout2;
};

int BluebananaHSReadout0::value_event()
{
	plugin->config.Hsel_lo = get_value();
	if(plugin->config.Hsel_lo < 0) plugin->config.Hsel_lo = 0;
	if(plugin->config.Hsel_lo > 360) plugin->config.Hsel_lo = 360;
	if(plugin->config.Hsel_hi - plugin->config.Hsel_lo > 360)
		plugin->config.Hsel_hi -= 360;
	if(plugin->config.Hsel_lo > plugin->config.Hsel_hi)
		plugin->config.Hsel_hi += 360;
	gui->Hsel_slider->update();
	return 1;
}

int BluebananaHSReadout1::value_event()
{
	plugin->config.Hsel_hi = get_value();
	if(plugin->config.Hsel_hi < 0) plugin->config.Hsel_hi = 0;
	if(plugin->config.Hsel_hi > 360) plugin->config.Hsel_hi = 360;
	if(plugin->config.Hsel_lo > plugin->config.Hsel_hi)
		plugin->config.Hsel_hi += 360;
	gui->Hsel_slider->hival = -1; /* force update to hival, not the hi readout */
	gui->Hsel_slider->update();
	return 1;
}

int BluebananaHSReadout2::value_event()
{
	float val = get_value();
	if(val < 0)val = 0;
	if(val > 360)val = 360;
	plugin->config.Hsel_over = val;
	gui->Hsel_slider->update();
	return 1;
}

// ---------------------------- saturation adjustment slider ----------------------------
class BluebananaSAReadout0 : public BB_Tumble
{
public:
	BluebananaSAReadout0(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, -100, 0, 100., 0, 1, w) {}
	int value_event();
};
class BluebananaSAReadout1 : public BB_Tumble
{
public:
	BluebananaSAReadout1(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 0, 200., 0, 1, w) {}
	int value_event();
};
class BluebananaSAReadout2 : public BB_Tumble
{
public:
	BluebananaSAReadout2(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, MIN_GAMMA, 0, MAX_GAMMA, 2, .01, w) {}
	int value_event();
};

class BluebananaSASlider : public BluebananaSliderChannel
{
public:
	BluebananaSASlider(BluebananaMain *plugin, BluebananaWindow *gui,
			int x, int y, int w, int h)
		: BluebananaSliderChannel(plugin, gui, x, y, w, h) { }

	virtual int handle_event()
	{
		plugin->config.Sadj_lo = loval;
		plugin->config.Sadj_hi = hival;
		plugin->config.Sadj_gamma = gamma;
		return 1;
	}

	void reset()
	{
		plugin->config.Sadj_lo = 0.;
		plugin->config.Sadj_hi = 100.;
		plugin->config.Sadj_gamma = 1.;
		update();
	}

	void update()
	{
		loval = plugin->config.Sadj_lo;
		hival = plugin->config.Sadj_hi;
		gamma = plugin->config.Sadj_gamma;

		highlight = plugin->config.active && plugin->config.Sadj_active;

		gui->Sadj_readout0->update(plugin->config.Sadj_lo);
		gui->Sadj_readout1->update(plugin->config.Sadj_hi);
		gui->Sadj_readout2->update(plugin->config.Sadj_gamma);
		gui->slider_labels[8]->set_color(highlight &&
			(plugin->config.Sadj_lo != 0 ||
			plugin->config.Sadj_hi != 100 ||
			plugin->config.Sadj_gamma != 1.) ?
			get_resources()->default_text_color : dimtextcolor);

		gui->enter_config_change();
		gui->commit_config_change();
	}
	void trough_color(float hdel, float &r, float &g, float &b)
	{
		r = g = b = 0;
	}
};

#define BBCLAMP(x, y, z) ((x)<(y) ? (y) : (x)>(z) ? (z) : (x))

int BluebananaSAReadout0::value_event()
{
	plugin->config.Sadj_lo = BBCLAMP(get_value(), -100, plugin->config.Sadj_hi);
	plugin->config.Sadj_lo = BBCLAMP(get_value(), -100, 100);
	gui->Sadj_slider->update();
	return 1;
}

int BluebananaSAReadout1::value_event()
{
	plugin->config.Sadj_hi = BBCLAMP(get_value(), plugin->config.Sadj_lo, 200);
	plugin->config.Sadj_hi = BBCLAMP(get_value(), 0, 200);
	gui->Sadj_slider->update();
	return 1;
}

int BluebananaSAReadout2::value_event()
{
	plugin->config.Sadj_gamma = BBCLAMP(get_value(), MIN_GAMMA, MAX_GAMMA);
	gui->Sadj_slider->update();
	return 1;
}

// ------------------------------- value adjustment slider -------------------------------
class BluebananaVAReadout0 : public BB_Tumble
{
public:
	BluebananaVAReadout0(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, -100., 0, 100., 0, 1, w) {}
	int value_event();
};

class BluebananaVAReadout1 : public BB_Tumble
{
public:
	BluebananaVAReadout1(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 0, 200., 0, 1, w) {}
	int value_event();
};

class BluebananaVAReadout2 : public BB_Tumble
{
public:
	BluebananaVAReadout2(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, MIN_GAMMA, 0, MAX_GAMMA, 2, .01, w) {}
	int value_event();
};

class BluebananaVASlider : public BluebananaSliderChannel
{
public:
	BluebananaVASlider(BluebananaMain *plugin, BluebananaWindow *gui,
			int x, int y, int w, int h)
		: BluebananaSliderChannel(plugin, gui, x, y, w, h) { }

	virtual int handle_event()
	{
		plugin->config.Vadj_lo = loval;
		plugin->config.Vadj_hi = hival;
		plugin->config.Vadj_gamma = gamma;
		return 1;
	}

	void reset()
	{
		plugin->config.Vadj_lo = 0;
		plugin->config.Vadj_hi = 100;
		plugin->config.Vadj_gamma = 1;
		update();
	}

	void update()
	{
		loval = plugin->config.Vadj_lo;
		hival = plugin->config.Vadj_hi;
		gamma = plugin->config.Vadj_gamma;

		highlight = plugin->config.active && plugin->config.Vadj_active;

		gui->Vadj_readout0->update(plugin->config.Vadj_lo);
		gui->Vadj_readout1->update(plugin->config.Vadj_hi);
		gui->Vadj_readout2->update(plugin->config.Vadj_gamma);
		gui->slider_labels[9]->set_color(highlight  &&
			(plugin->config.Vadj_lo != 0.f ||
			plugin->config.Vadj_hi != 100.f ||
			plugin->config.Vadj_gamma != 1.f) ?
			get_resources()->default_text_color : dimtextcolor);

		gui->enter_config_change();
		gui->commit_config_change();
	}

	void trough_color(float hdel, float &r, float &g, float &b)
	{
		r = g = b = 0;
	}
};

int BluebananaVAReadout0::value_event()
{
	plugin->config.Vadj_lo = BBCLAMP(get_value(), -100, plugin->config.Vadj_hi);
	plugin->config.Vadj_lo = BBCLAMP(get_value(), -100, 100);
	gui->Vadj_slider->update();
	return 1;
}

int BluebananaVAReadout1::value_event()
{
	plugin->config.Vadj_hi = BBCLAMP(get_value(), plugin->config.Vadj_lo, 200);
	plugin->config.Vadj_hi = BBCLAMP(get_value(), 0, 200);
	gui->Vadj_slider->update();
	return 1;
}

int BluebananaVAReadout2::value_event()
{
	plugin->config.Vadj_gamma = BBCLAMP(get_value(), MIN_GAMMA, MAX_GAMMA);
	gui->Vadj_slider->update();
	return 1;
}

// -------------------------------- red adjustment slider --------------------------------
class BluebananaRAReadout0 : public BB_Tumble
{
public:
	BluebananaRAReadout0(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, -100., 0, 100., 0, 1, w) {}
	int value_event();
};
class BluebananaRAReadout1 : public BB_Tumble
{
public:
	BluebananaRAReadout1(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 0, 200., 0, 1, w) {}
	int value_event();
};
class BluebananaRAReadout2 : public BB_Tumble
{
public:
	BluebananaRAReadout2(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, MIN_GAMMA, 0, MAX_GAMMA, 2, .01, w) {}
	int value_event();
};

class BluebananaRASlider : public BluebananaSliderChannel
{
public:
	BluebananaRASlider(BluebananaMain *plugin, BluebananaWindow *gui,
					   int x, int y, int w, int h)
		: BluebananaSliderChannel(plugin, gui, x, y, w, h) { }

	virtual int handle_event()
	{
		plugin->config.Radj_lo = loval;
		plugin->config.Radj_hi = hival;
		plugin->config.Radj_gamma = gamma;
		return 1;
	}

	void reset()
	{
		plugin->config.Radj_lo = 0;
		plugin->config.Radj_hi = 100;
		plugin->config.Radj_gamma = 1;
		update();
	}

	void update()
	{
		loval = plugin->config.Radj_lo;
		hival = plugin->config.Radj_hi;
		gamma = plugin->config.Radj_gamma;

		highlight = plugin->config.active && plugin->config.Radj_active;

		gui->Radj_readout0->update(plugin->config.Radj_lo);
		gui->Radj_readout1->update(plugin->config.Radj_hi);
		gui->Radj_readout2->update(plugin->config.Radj_gamma);
		gui->slider_labels[4]->set_color(highlight  &&
			(plugin->config.Radj_lo != 0 ||
			plugin->config.Radj_hi != 100 ||
			plugin->config.Radj_gamma != 1) ?
			get_resources()->default_text_color : dimtextcolor);

		gui->enter_config_change();
		gui->commit_config_change();
	}

	void trough_color(float hdel, float &r, float &g, float &b)
	{
		if(hdel < 0)
		{
			r = g = b = 0.;
		}
		else if(hdel <= 1.f)
		{
			r = hdel;
			g = b = 0.;
		}
		else
		{
			r = 1.;
			g = b = 0.;
		}
	}
};

int BluebananaRAReadout0::value_event()
{
	plugin->config.Radj_lo = BBCLAMP(get_value(), -100, plugin->config.Radj_hi);
	plugin->config.Radj_lo = BBCLAMP(get_value(), -100, 100);
	gui->Radj_slider->update();
	return 1;
}

int BluebananaRAReadout1::value_event()
{
	plugin->config.Radj_hi = BBCLAMP(get_value(), plugin->config.Radj_lo, 200);
	plugin->config.Radj_hi = BBCLAMP(get_value(), 0, 200);
	gui->Radj_slider->update();
	return 1;
}

int BluebananaRAReadout2::value_event()
{
	plugin->config.Radj_gamma = BBCLAMP(get_value(), MIN_GAMMA, MAX_GAMMA);
	gui->Radj_slider->update();
	return 1;
}

// ---------------------------- green adjustment slider ----------------------------
class BluebananaGAReadout0 : public BB_Tumble
{
public:
	BluebananaGAReadout0(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, -100., 0, 100., 0, 1, w) {}
	int value_event();
};
class BluebananaGAReadout1 : public BB_Tumble
{
public:
	BluebananaGAReadout1(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 0, 200., 0, 1, w) {}
	int value_event();
};
class BluebananaGAReadout2 : public BB_Tumble
{
public:
	BluebananaGAReadout2(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, MIN_GAMMA, 0, MAX_GAMMA, 2, .01, w) {}
	int value_event();
};

class BluebananaGASlider : public BluebananaSliderChannel
{
public:
	BluebananaGASlider(BluebananaMain *plugin, BluebananaWindow *gui,
			int x, int y, int w, int h)
		: BluebananaSliderChannel(plugin, gui, x, y, w, h) { }

	virtual int handle_event()
	{
		plugin->config.Gadj_lo = loval;
		plugin->config.Gadj_hi = hival;
		plugin->config.Gadj_gamma = gamma;
		return 1;
	}

	void reset()
	{
		plugin->config.Gadj_lo = 0;
		plugin->config.Gadj_hi = 100;
		plugin->config.Gadj_gamma = 1;
		update();
	}

	void update()
	{
		loval = plugin->config.Gadj_lo;
		hival = plugin->config.Gadj_hi;
		gamma = plugin->config.Gadj_gamma;

		highlight = plugin->config.active && plugin->config.Gadj_active;

		gui->Gadj_readout0->update(plugin->config.Gadj_lo);
		gui->Gadj_readout1->update(plugin->config.Gadj_hi);
		gui->Gadj_readout2->update(plugin->config.Gadj_gamma);
		gui->slider_labels[5]->set_color(highlight  &&
			(plugin->config.Gadj_lo != 0 ||
			plugin->config.Gadj_hi != 100 ||
			plugin->config.Gadj_gamma != 1) ?
			get_resources()->default_text_color : dimtextcolor);

		gui->enter_config_change();
		gui->commit_config_change();
	}

	void trough_color(float hdel, float &r, float &g, float &b)
	{
		if(hdel < 0)
		{
			r = g = b = 0.;
		}
		else if(hdel <= 1.f)
		{
			g = hdel;
			r = b = 0.;
		}
		else
		{
			g = 1.;
			r = b = 0.;
		}
	}
};

int BluebananaGAReadout0::value_event()
{
	plugin->config.Gadj_lo = BBCLAMP(get_value(), -100, plugin->config.Gadj_hi);
	plugin->config.Gadj_lo = BBCLAMP(get_value(), -100, 100);
	gui->Gadj_slider->update();
	return 1;
}

int BluebananaGAReadout1::value_event()
{
	plugin->config.Gadj_hi = BBCLAMP(get_value(), plugin->config.Gadj_lo, 200);
	plugin->config.Gadj_hi = BBCLAMP(get_value(), 0, 200);
	gui->Gadj_slider->update();
	return 1;
}

int BluebananaGAReadout2::value_event()
{
	plugin->config.Gadj_gamma = BBCLAMP(get_value(), MIN_GAMMA, MAX_GAMMA);
	gui->Gadj_slider->update();
	return 1;
}

// ------------------------------- blue adjustment slider -------------------------------
class BluebananaBAReadout0 : public BB_Tumble
{
public:
	BluebananaBAReadout0(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, -100., 0, 100., 0, 1, w) {}
	int value_event();
};

class BluebananaBAReadout1 : public BB_Tumble
{
public:
	BluebananaBAReadout1(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 0, 200., 0, 1, w) {}
	int value_event();
};

class BluebananaBAReadout2 : public BB_Tumble
{
public:
	BluebananaBAReadout2(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, MIN_GAMMA, 0, MAX_GAMMA, 2, .01, w) {}
	int value_event();
};

class BluebananaBASlider : public BluebananaSliderChannel
{
public:
	BluebananaBASlider(BluebananaMain *plugin, BluebananaWindow *gui,
			int x, int y, int w, int h)
		: BluebananaSliderChannel(plugin, gui, x, y, w, h) { }

	virtual int handle_event()
	{
		plugin->config.Badj_lo = loval;
		plugin->config.Badj_hi = hival;
		plugin->config.Badj_gamma = gamma;
		return 1;
	}

	void reset()
	{
		plugin->config.Badj_lo = 0;
		plugin->config.Badj_hi = 100;
		plugin->config.Badj_gamma = 1;
		update();
	}

	void update()
	{
		loval = plugin->config.Badj_lo;
		hival = plugin->config.Badj_hi;
		gamma = plugin->config.Badj_gamma;

		highlight = plugin->config.active && plugin->config.Badj_active;

		gui->Badj_readout0->update(plugin->config.Badj_lo);
		gui->Badj_readout1->update(plugin->config.Badj_hi);
		gui->Badj_readout2->update(plugin->config.Badj_gamma);
		gui->slider_labels[6]->set_color(highlight  &&
			(plugin->config.Badj_lo != 0 ||
			plugin->config.Badj_hi != 100 ||
			plugin->config.Badj_gamma != 1) ?
			get_resources()->default_text_color : dimtextcolor);

		gui->enter_config_change();
		gui->commit_config_change();
	}

	void trough_color(float hdel, float &r, float &g, float &b)
	{
		if(hdel < 0)
		{
			r = g = b = 0.;
		}
		else if(hdel <= 1.f)
		{
			b = hdel;
			g = r = 0.;
		}
		else
		{
			b = 1.;
			g = r = 0.;
		}
	}
};

int BluebananaBAReadout0::value_event()
{
	plugin->config.Badj_lo = BBCLAMP(get_value(), -100, plugin->config.Badj_hi);
	plugin->config.Badj_lo = BBCLAMP(get_value(), -100, 100);
	gui->Badj_slider->update();
	return 1;
}

int BluebananaBAReadout1::value_event()
{
	plugin->config.Badj_hi = BBCLAMP(get_value(), plugin->config.Badj_lo, 200);
	plugin->config.Badj_hi = BBCLAMP(get_value(), 0, 200);
	gui->Badj_slider->update();
	return 1;
}

int BluebananaBAReadout2::value_event()
{
	plugin->config.Badj_gamma = BBCLAMP(get_value(), MIN_GAMMA, MAX_GAMMA);
	gui->Badj_slider->update();
	return 1;
}

// ---------------------------------- opacity slider ---------------------------------
class BluebananaOAReadout : public BB_Tumble
{
public:
	BluebananaOAReadout(BluebananaMain *plugin, BluebananaWindow *gui, int w)
		: BB_Tumble(plugin, gui, 0., 0, 100., 0, 1, w) {}
	int value_event();
};

class BluebananaOASlider : public BluebananaSliderSingle
{
public:
	BluebananaOASlider(BluebananaMain *plugin, BluebananaWindow *gui,
			int x, int y, int w, int h)
		: BluebananaSliderSingle(plugin, gui, x, y, w, h, 0, 100) { }

	virtual int handle_event()
	{
		plugin->config.Oadj_val = val;
		return 1;
	}

	void reset()
	{
		plugin->config.Oadj_val = 100;
		update();
	}

	void update()
	{
		val = plugin->config.Oadj_val;
		highlight = plugin->config.active && plugin->config.Oadj_active;
		gui->Oadj_readout->update(plugin->config.Oadj_val);
		gui->slider_labels[10]->set_color(highlight  &&
			plugin->config.Oadj_val != 100 ?
				get_resources()->default_text_color : dimtextcolor);
		gui->enter_config_change();
		gui->commit_config_change();
	}

	void trough_color(float hdel, float vdel, float &r, float &g, float &b, float &a)
	{
		r = g = b = .8;
		a = 1 - cos(hdel * M_PI * .5);
	}
};

int BluebananaOAReadout::value_event()
{
	float val = get_value();
	plugin->config.Oadj_val = val;
	gui->Oadj_slider->update();
	return 1;
}

// ------------------------------------- picker buttons -----------------------------------------
class BluebananaHPicker : public BC_GenericButton
{
public:
	BluebananaHPicker(BluebananaWindow *gui, int w) : BC_GenericButton(-1, -1, w, _("Pick"))
	{
		this->gui = gui;
	}
	int handle_event()
	{
		gui->Hsel_slider->pick();
		return 1;
	}
	BluebananaWindow *gui;
};

class BluebananaSPicker : public BC_GenericButton
{
public:
	BluebananaSPicker(BluebananaWindow *gui, int w) : BC_GenericButton(-1, -1, w, _("Pick"))
	{
		this->gui = gui;
	}
	int handle_event()
	{
		gui->Ssel_slider->pick();
		return 1;
	}
	BluebananaWindow *gui;
};

class BluebananaVPicker : public BC_GenericButton
{
public:
	BluebananaVPicker(BluebananaWindow *gui, int w) : BC_GenericButton(-1, -1, w, _("Pick"))
	{
		this->gui = gui;
	}
	int handle_event()
	{
		gui->Vsel_slider->pick();
		return 1;
	}
	BluebananaWindow *gui;
};

// -------------------------------------- reset buttons -----------------------------------------

class BluebananaHAReset : public BC_GenericButton
{
public:
	BluebananaHAReset(BluebananaWindow *gui, int w) : BC_GenericButton(-1, -1, w, _("Reset"))
	{
		this->gui = gui;
	}
	int handle_event()
	{
		gui->Hadj_slider->reset();
		return 1;
	}
	BluebananaWindow *gui;
};

class BluebananaSAReset : public BC_GenericButton
{
public:
	BluebananaSAReset(BluebananaWindow *gui, int w) : BC_GenericButton(-1, -1, w, _("Reset"))
	{
		this->gui = gui;
	}
	int handle_event()
	{
		gui->Sadj_slider->reset();
		return 1;
	}
	BluebananaWindow *gui;
};

class BluebananaVAReset : public BC_GenericButton
{
public:
	BluebananaVAReset(BluebananaWindow *gui, int w) : BC_GenericButton(-1, -1, w, _("Reset"))
	{
		this->gui = gui;
	}
	int handle_event()
	{
		gui->Vadj_slider->reset();
		return 1;
	}
	BluebananaWindow *gui;
};

class BluebananaRAReset : public BC_GenericButton
{
public:
	BluebananaRAReset(BluebananaWindow *gui, int w) : BC_GenericButton(-1, -1, w, _("Reset"))
	{
		this->gui = gui;
	}
	int handle_event()
	{
		gui->Radj_slider->reset();
		return 1;
	}
	BluebananaWindow *gui;
};

class BluebananaGAReset : public BC_GenericButton
{
public:
	BluebananaGAReset(BluebananaWindow *gui, int w) : BC_GenericButton(-1, -1, w, _("Reset"))
	{
		this->gui = gui;
	}
	int handle_event()
	{
		gui->Gadj_slider->reset();
		return 1;
	}
	BluebananaWindow *gui;
};

class BluebananaBAReset : public BC_GenericButton
{
public:
	BluebananaBAReset(BluebananaWindow *gui, int w) : BC_GenericButton(-1, -1, w, _("Reset"))
	{
		this->gui = gui;
	}
	int handle_event()
	{
		gui->Badj_slider->reset();
		return 1;
	}
	BluebananaWindow *gui;
};

class BluebananaOAReset : public BC_GenericButton
{
public:
	BluebananaOAReset(BluebananaWindow *gui, int w) : BC_GenericButton(-1, -1, w, _("Reset"))
	{
		this->gui = gui;
	}
	int handle_event()
	{
		gui->Oadj_slider->reset();
		return 1;
	}
	BluebananaWindow *gui;
};

// ----------------------------------- slider active buttons ------------------------------------
BluebananaHActive::BluebananaHActive(BluebananaMain *plugin, BluebananaWindow *gui)
	: BC_CheckBox(-1, -1, &plugin->config.Hsel_active, "")
{
	this->plugin = plugin;
	this->gui = gui;
}

int BluebananaHActive::handle_event()
{
	plugin->config.Hsel_active = get_value();
	update();
	return 1;
}

void BluebananaHActive::update()
{
	this->BC_CheckBox::update(plugin->config.Hsel_active, 1);
	gui->Hsel_slider->update();
}

BluebananaSActive::BluebananaSActive(BluebananaMain *plugin, BluebananaWindow *gui)
	: BC_CheckBox(-1, -1, &plugin->config.Ssel_active, "")
{
	this->plugin = plugin;
	this->gui = gui;
}

int BluebananaSActive::handle_event()
{
	plugin->config.Ssel_active = get_value();
	update();
	return 1;
}

void BluebananaSActive::update()
{
	this->BC_CheckBox::update(plugin->config.Ssel_active, 1);
	gui->Ssel_slider->update();
}

BluebananaVActive::BluebananaVActive(BluebananaMain *plugin, BluebananaWindow *gui)
	: BC_CheckBox(-1, -1, &plugin->config.Vsel_active, "")
{
	this->plugin = plugin;
	this->gui = gui;
}

int BluebananaVActive::handle_event()
{
	plugin->config.Vsel_active = get_value();
	update();
	return 1;
}

void BluebananaVActive::update()
{
	this->BC_CheckBox::update(plugin->config.Vsel_active, 1);
	gui->Vsel_slider->update();
}

class BluebananaFActive : public BC_CheckBox
{
public:
	BluebananaFActive(BluebananaMain *plugin, BluebananaWindow *gui)
		: BC_CheckBox(-1, -1, &plugin->config.Fsel_active, "")
	{
		this->plugin = plugin;
		this->gui = gui;
	}

	virtual int handle_event()
	{
		plugin->config.Fsel_active = get_value();
		update();
		return 1;
	}

	void update()
	{
		this->BC_CheckBox::update(plugin->config.Fsel_active, 1);
		gui->Fsel_slider->update();
	}

	BluebananaMain *plugin;
	BluebananaWindow *gui;
};

class BluebananaHAActive : public BC_CheckBox
{
public:
	BluebananaHAActive(BluebananaMain *plugin, BluebananaWindow *gui)
		: BC_CheckBox(-1, -1, &plugin->config.Hadj_active, "")
	{
		this->plugin = plugin;
		this->gui = gui;
	}

	virtual int handle_event()
	{
		plugin->config.Hadj_active = get_value();
		update();
		return 1;
	}

	void update()
	{
		this->BC_CheckBox::update(plugin->config.Hadj_active, 1);
		gui->Hadj_slider->update();
	}
	BluebananaMain *plugin;
	BluebananaWindow *gui;
};

class BluebananaSAActive : public BC_CheckBox
{
public:
	BluebananaSAActive(BluebananaMain *plugin, BluebananaWindow *gui)
		: BC_CheckBox(-1, -1, &plugin->config.Sadj_active, "")
	{
		this->plugin = plugin;
		this->gui = gui;
	}

	virtual int handle_event()
	{
		plugin->config.Sadj_active = get_value();
		update();
		return 1;
	}

	void update()
	{
		this->BC_CheckBox::update(plugin->config.Sadj_active, 1);
		gui->Sadj_slider->update();
	}

	BluebananaMain *plugin;
	BluebananaWindow *gui;
};

class BluebananaVAActive : public BC_CheckBox
{
public:
	BluebananaVAActive(BluebananaMain *plugin, BluebananaWindow *gui)
		: BC_CheckBox(-1, -1, &plugin->config.Vadj_active, "")
	{
		this->plugin = plugin;
		this->gui = gui;
	}
	virtual int handle_event()
	{
		plugin->config.Vadj_active = get_value();
		update();
		return 1;
	}

	void update()
	{
		this->BC_CheckBox::update(plugin->config.Vadj_active, 1);
		gui->Vadj_slider->update();
	}

	BluebananaMain *plugin;
	BluebananaWindow *gui;
};

class BluebananaRAActive : public BC_CheckBox
{
public:
	BluebananaRAActive(BluebananaMain *plugin, BluebananaWindow *gui)
		: BC_CheckBox(-1, -1, &plugin->config.Radj_active, "")
	{
		this->plugin = plugin;
		this->gui = gui;
	}

	virtual int handle_event()
	{
		plugin->config.Radj_active = get_value();
		update();
		return 1;
	}

	void update()
	{
		this->BC_CheckBox::update(plugin->config.Radj_active, 1);
		gui->Radj_slider->update();
	}
	BluebananaMain *plugin;
	BluebananaWindow *gui;
};

class BluebananaGAActive : public BC_CheckBox
{
public:
	BluebananaGAActive(BluebananaMain *plugin, BluebananaWindow *gui)
		: BC_CheckBox(-1, -1, &plugin->config.Gadj_active, "")
	{
		this->plugin = plugin;
		this->gui = gui;
	}

	virtual int handle_event()
	{
		plugin->config.Gadj_active = get_value();
		update();
		return 1;
	}

	void update()
	{
		this->BC_CheckBox::update(plugin->config.Gadj_active, 1);
		gui->Gadj_slider->update();
	}

	BluebananaMain *plugin;
	BluebananaWindow *gui;
};

class BluebananaBAActive : public BC_CheckBox
{
public:
	BluebananaBAActive(BluebananaMain *plugin, BluebananaWindow *gui)
		: BC_CheckBox(-1, -1, &plugin->config.Badj_active, "")
	{
		this->plugin = plugin;
		this->gui = gui;
	}

	virtual int handle_event()
	{
		plugin->config.Badj_active = get_value();
		update();
		return 1;
	}

	void update()
	{
		this->BC_CheckBox::update(plugin->config.Badj_active, 1);
		gui->Badj_slider->update();
	}

	BluebananaMain *plugin;
	BluebananaWindow *gui;
};

class BluebananaOAActive : public BC_CheckBox
{
public:
	BluebananaOAActive(BluebananaMain *plugin, BluebananaWindow *gui)
		: BC_CheckBox(-1, -1, &plugin->config.Oadj_active, "")
	{
		this->plugin = plugin;
		this->gui = gui;
	}

	virtual int handle_event()
	{
		plugin->config.Oadj_active = get_value();
		update();
		return 1;
	}

	void update()
	{
		this->BC_CheckBox::update(plugin->config.Oadj_active, 1);
		gui->Oadj_slider->update();
	}

	BluebananaMain *plugin;
	BluebananaWindow *gui;
};

// -------------------------------------------- Erode --------------------------------------------
class BluebananaErode : public BC_CheckBox
{
public:
	BluebananaErode(BluebananaMain *plugin, BluebananaWindow *gui)
		: BC_CheckBox(-1, -1, &plugin->config.Fsel_erode, "")
	{
		this->plugin = plugin;
		this->gui = gui;
	}

	virtual int handle_event()
	{
		plugin->config.Fsel_erode = get_value();
		update();
		return 1;
	}

	void update()
	{
		this->BC_CheckBox::update(plugin->config.Fsel_erode, 1);
		gui->Fsel_slider->update();
	}

	BluebananaMain *plugin;
	BluebananaWindow *gui;
};


// -------------------------------------- Invert Selection ---------------------------------------
class BluebananaIS : public BC_CheckBox
{
public:
	BluebananaIS(BluebananaMain *plugin, BluebananaWindow *gui)
		: BC_CheckBox(-1, -1, &plugin->config.invert_selection, "")
	{
		this->plugin = plugin;
		this->gui = gui;
	}

	virtual int handle_event()
	{
		plugin->config.invert_selection = get_value();
		update();
		return 1;
	}

	void update()
	{
		this->BC_CheckBox::update(plugin->config.invert_selection, 1);
		gui->enter_config_change();
		gui->commit_config_change();
	}

	BluebananaMain *plugin;
	BluebananaWindow *gui;
};


// -------------------------------------------- Mark --------------------------------------------
class BluebananaMark : public BC_CheckBox
{
public:
	BluebananaMark(BluebananaMain *plugin, BluebananaWindow *gui)
		: BC_CheckBox(-1, -1, 0, "")
	{
		this->plugin = plugin;
		this->gui = gui;
	}

	virtual int handle_event()
	{
		plugin->config.mark = get_value();
		gui->enter_config_change();
		gui->commit_config_change();
		return 1;
	}

	void update ()
	{
		BC_CheckBox::update(plugin->config.mark, 1);
	}

	BluebananaMain *plugin;
	BluebananaWindow *gui;
};

// ------------------------------------------- Active -------------------------------------------
class BluebananaActive : public BC_CheckBox
{
public:
	BluebananaActive(BluebananaMain *plugin, BluebananaWindow *gui)
		: BC_CheckBox(-1, -1, &plugin->config.active, "")
	{
		this->plugin = plugin;
		this->gui = gui;
		active = -1;
	}

	virtual int handle_event()
	{
		active = get_value();
		update();
		return 1;
	}

	void update()
	{
		if(active != plugin->config.active)
		{
			if(active >= 0)
				plugin->config.active = active;
			this->BC_CheckBox::update(plugin->config.active, 1);
			gui->enter_config_change();
			gui->Hadj_slider->update();
			gui->Sadj_slider->update();
			gui->Vadj_slider->update();
			gui->Radj_slider->update();
			gui->Gadj_slider->update();
			gui->Badj_slider->update();
			gui->Oadj_slider->update();
			gui->commit_config_change();
		}
	}

	BluebananaMain *plugin;
	BluebananaWindow *gui;
	int active;
};

// ---------------------------------------- Capture mask ---------------------------------------
class BluebananaUnmask : public BC_CheckBox
{
public:
	BluebananaUnmask(BluebananaMain *plugin, BluebananaWindow *gui, int padx)
		: BC_CheckBox(-1, -1, &plugin->config.capture_mask, "")
	{
		this->plugin = plugin;
		this->gui = gui;
		this->padx = padx;
		this->label = new BC_Title(-1, -1, _(" End Mask"));
		this->x = -1;
		this->y = -1;
		gui->add_subwindow(this->label);
		gui->add_subwindow(this);
		hidden = -1;
	}

	virtual int handle_event()
	{
		plugin->config.capture_mask = get_value();
		update();
		gui->enter_config_change();
		gui->commit_config_change();
		return 1;
	}

	int get_h()
	{
		return label->get_h();
	}

	int get_w()
	{
		return BC_CheckBox::get_w() + label->get_w() + padx * 4;
	}

	void reposition_window(int x, int y)
	{
		int h = label->get_h();
		this->x = x;
		this->y = y;
		label->reposition_window(x + padx, y);
		BC_CheckBox::reposition_window(x + padx * 2 + label->get_w(), y + (h - BC_CheckBox::get_h()) / 2);
		update();
	}

	void update()
	{
		int w = get_w();
		int h = get_h();
		int f = 0;
		int hideme = !plugin->config.use_mask;
		switch(plugin->colormodel)
		{
		case BC_RGB888:
		case BC_RGB_FLOAT:
		case BC_YUV888:
		case BC_RGB161616:
		case BC_YUV161616:
			hideme = 1;
			break;
		}

		if(hideme && hidden != 1)
		{
			hide_window();
			label->hide_window();
			gui->set_color(get_resources()->get_bg_color());
			gui->draw_box(x, y, w, h);
			gui->set_color(get_resources()->default_text_color);
			gui->draw_line(x, y + h / 2, x + w, y + h / 2);
			hidden = 1;
			f = 1;
		}

		if(!hideme && hidden != 0)
		{
			gui->set_color(get_resources()->get_bg_color());
			gui->draw_box(x, y, w, h);
			show_window();
			label->show_window();
			hidden = 0;
			f = 1;
		}

		if(plugin->config.capture_mask != get_value())
			this->BC_CheckBox::update(plugin->config.capture_mask, 1);
		if(f)
			gui->flash(x, y, w, h);
	}

	BluebananaMain *plugin;
	BluebananaWindow *gui;
	BC_Title *label;
	int x, y, padx, hidden;
};

// ------------------------------------------ Use mask ----------------------------------------
class BluebananaA2Sel : public BC_CheckBox
{
public:
	BluebananaA2Sel(BluebananaMain *plugin, BluebananaWindow *gui, int padx)
		: BC_CheckBox(-1, -1, &plugin->config.use_mask, "")
	{
		this->plugin = plugin;
		this->gui = gui;
		this->padx = padx;
		this->label = new BC_Title(-1, -1, _(" Mask Selection"));
		this->x = -1;
		this->y = -1;
		gui->add_subwindow(this->label);
		gui->add_subwindow(this);
		hidden = -1;
	}

	virtual int handle_event()
	{
		plugin->config.use_mask = get_value();
		update();
		gui->enter_config_change();
		gui->commit_config_change();
		return 1;
	}

	int get_h()
	{
		return label->get_h();
	}

	int get_w()
	{
		return BC_CheckBox::get_w() + label->get_w() + padx * 4;
	}

	void reposition_window(int x, int y)
	{
		int h = label->get_h();
		this->x = x;
		this->y = y;
		label->reposition_window(x + padx, y);
		BC_CheckBox::reposition_window(x + padx * 2 + label->get_w(), y + (h - BC_CheckBox::get_h()) / 2);
		update();
	}

	void update()
	{
		int w = get_w();
		int h = get_h();
		int f = 0;

		if(gui->capture_mask)
			gui->capture_mask->update();

		switch(plugin->colormodel)
		{
		case BC_RGBA16161616:
		case BC_AYUV16161616:
			if(hidden != 0)
			{
				gui->set_color(get_resources()->get_bg_color());
				gui->draw_box(x, y, w, h);
				show_window();
				label->show_window();
				hidden = 0;
				f = 1;
			}
			break;
		case -1:
			// not initialized yet
			return;
		default:
			fprintf(stderr, "Unknown colormodel in BluebananaA2Sel:update()\n");
			break;
		}
		if(plugin->config.use_mask != get_value())
			this->BC_CheckBox::update(plugin->config.use_mask, 1);
		if(f)
			gui->flash(x, y, w, h);
	}

	BluebananaMain *plugin;
	BluebananaWindow *gui;
	BC_Title *label;
	int x, y, padx, hidden;
};

// --------------------------------------- Main GUI window --------------------------------------
BluebananaWindow::BluebananaWindow(BluebananaMain *plugin, int x, int y)
 : PluginWindow(plugin, x, y, 1000, 1000)
{
	int xmargin = 20, ymargin = 10;
	float row_padding = .1;
	float column_padding = .3;
	int padx = 0;

	int i;
	int row_h = 0, row_adv = 0;
	int label_x = 0;
	int label_w = 0;
	int slider_left_x = 0, slider_w = 0;
	int picker_w = 0;
	int reset_w = 0;
	int tumbler_w = 0, tumbler_ww = 0, tumbler_h = 0;
	int tumbler_col1_x = 0, tumbler_col2_x = 0, tumbler_col2_w = 0;

	do_render = 0;
	windowx = x;
	windowy = y;
	this->plugin = plugin;
	config_refcount = 1; // suppress pushing config during startup
	config_change = 0;
	config_produce = 0;
	config_consume = 0;
	config_pending = 0;

	Hsel_slider = NULL;
	Ssel_slider = NULL;
	Vsel_slider = NULL;
	Fsel_slider = NULL;
	Hadj_slider = NULL;
	Sadj_slider = NULL;
	Vadj_slider = NULL;
	Radj_slider = NULL;
	Gadj_slider = NULL;
	Badj_slider = NULL;
	Oadj_slider = NULL;

	use_mask = 0;
	capture_mask = 0;

	y = ymargin;

	BluebananaHAReset *hareset = NULL;

	config_refcount = 0;
	enter_config_change();

	// window headline
	{
		BC_Title *l = new BC_Title(xmargin, y, _("Color Selection"));
		BC_Title *l2 = new BC_Title(-1, -1, _(" Mark Selected Areas"));

		add_subwindow(mark = new BluebananaMark(plugin, this));
		add_subwindow(l);
		add_subwindow(l2);
		padx = l->get_h() * column_padding;
		label_x = xmargin + l->get_w();

		int x0 = get_w() - xmargin - mark->get_w();
		mark->reposition_window(x0, y - (mark->get_h() - l->get_h()) / 2);
		x0 -= padx + l2->get_w();
		l2->reposition_window(x0, y);
		x0 -= padx;

		set_color(get_resources()->default_text_color);
		draw_line(label_x + padx, (int)(y + l->get_h()*.5), x0, (int)(y + l->get_h()*.5));

		y += l->get_h() * (row_padding + 1.);
	}

	const char *labels[11] = {_("hue"), _("saturation"), _("value"), _("fill"), _("red"), _("green"), _("blue"), _("hue"), _("saturation"), _("value"), _("fade")};
	for(i = 0; i < 11; i++)
	{
		add_subwindow(slider_labels[i] = new BC_Title(-1, -1, labels[i]));
		if(slider_labels[i]->get_w() > label_w)label_w = slider_labels[i]->get_w();
	}

	int tumbler_text_ww = MAX(get_text_width(MEDIUMFONT, "-000"), get_text_width(MEDIUMFONT, "5.00")) + 8;
	int tumbler_text_w = get_text_width(MEDIUMFONT, "50") + 8;
	if(tumbler_text_w * 3 < tumbler_text_ww * 2)
	{
		tumbler_text_ww = (tumbler_text_ww * 2 + 2) / 3 * 3 / 2;
		tumbler_text_w = tumbler_text_ww * 2 / 3;
	}

	erode_label = new BC_Title(xmargin, y, _("pre-erode"));
	BluebananaErode *erode = new BluebananaErode(plugin, this);
	add_subwindow(erode_label);
	add_subwindow(erode);

	for(i = 0; i < 11; i++)
	{
		BC_GenericButton *p = NULL;
		BluebananaSlider *s = NULL;
		BB_Tumble *t0 = NULL, *t1 = NULL, *t2 = NULL;
		BC_Toggle *a = NULL;
		BC_Title *l = slider_labels[i];

		switch(i)
		{
		case 0:
			add_subwindow(t0 = Hsel_readout0 = new BluebananaHSReadout0(plugin, this, tumbler_text_ww));
			add_subwindow(t1 = Hsel_readout1 = new BluebananaHSReadout1(plugin, this, tumbler_text_ww));
			add_subwindow(t2 = Hsel_readout2 = new BluebananaHSReadout2(plugin, this, tumbler_text_ww));
			add_subwindow(a = Hsel_active = new BluebananaHActive(plugin, this));

			// need a narrow and a wide wide tumbler
			add_subwindow(Fsel_readout0 = new BluebananaFSReadout0(plugin, this, tumbler_text_w));
			add_subwindow(Fsel_readout1 = new BluebananaFSReadout1(plugin, this, tumbler_text_w));
			add_subwindow(Fsel_readout2 = new BluebananaFSReadout2(plugin, this, tumbler_text_w));
			add_subwindow(Fsel_readout3 = new BluebananaFSReadout3(plugin, this, tumbler_text_ww));
			tumbler_w = Fsel_readout0->get_w();
			tumbler_ww = t0->get_w();
			tumbler_h = t0->get_h();

			// need a reset button's width
			reset_w = BC_GenericButton::calculate_w(this, _("Reset"));
			picker_w = BC_GenericButton::calculate_w(this, _("Pick"));

			// determine row spacing 
			row_h = 30; // minimum widget height allowance for the row 
					//(really, min height for the slider)
			if(row_h < a->get_h()) row_h = a->get_h();
			if(row_h < l->get_h()) row_h = l->get_h();
			if(row_h < BC_GenericButton::calculate_h())row_h = BC_GenericButton::calculate_h();
			if(row_h < t2->get_h()) row_h = t2->get_h();
			row_adv = row_h * (1. + row_padding);

			// determine horizontal element positioning; two main setups
			// setup 1: three tumblers + button
			tumbler_col2_w = MAX(reset_w, picker_w);

			// setup 2: four tumblers + erode
			{
				int threew = tumbler_ww * 3 + padx * 4 + tumbler_col2_w;
				int fourw = tumbler_w * 3 + tumbler_ww + padx * 5 + erode->get_w() + erode_label->get_w();
				if(fourw > threew) tumbler_col2_w += fourw - threew;
			}

			tumbler_col2_x = get_w() - xmargin - tumbler_col2_w;
			tumbler_col1_x = tumbler_col2_x - tumbler_ww * 3 - padx * 5;
			slider_x = label_x + padx;
			slider_w = (tumbler_col1_x - slider_x - padx * 3);

			// make sure the label x doesn't cause any labels to go off the
			// left of the pane
			{
				int lx = label_x - padx - a->get_w();

				if (lx - label_w < xmargin)
				{
					slider_x += ((xmargin + label_w) - lx);
					label_x += ((xmargin + label_w) - lx);
					slider_w -= ((xmargin + label_w) - lx);
				}
			}

			y += row_adv / 3; // extra half row spacing under headline
			s = Hsel_slider = new BluebananaHSSlider(plugin, this, slider_x, y, slider_w, row_h);
			add_subwindow(p = new BluebananaHPicker(this, tumbler_col2_w));

			// Move the upper alpha <->selection config buttons into place
			{
				int x0 = slider_x + slider_w + padx * 2;
				invert_selection = new BluebananaIS(plugin, this);
				BC_Title *l = new BC_Title(xmargin, y, _(" Invert Selection"));

				add_subwindow(l);
				add_subwindow(invert_selection);
				int w0 = padx + l->get_w() + padx + invert_selection->get_w() + padx * 2;
				set_color(get_resources()->get_bg_color());
				draw_box(x0 - w0, ymargin, w0, ymargin + l->get_h());
				x0 -= padx * 2;
				x0 -= invert_selection->get_w();
				invert_selection->reposition_window(x0, ymargin + (l->get_h() - invert_selection->get_h()) / 2);
				x0 -= padx;
				x0 -= l->get_w();
				l->reposition_window(x0, ymargin);
				x0 -= padx;
				x0 -= padx * 5;

				use_mask = new BluebananaA2Sel(plugin, this, padx);
				x0 -= use_mask->get_w();
				use_mask->reposition_window(x0, ymargin);
				capture_mask = new BluebananaUnmask(plugin, this, padx);
				x0 -= padx * 5 + capture_mask->get_w();
				capture_mask->reposition_window(x0, ymargin);
			}

			break;

		case 1:
			add_subwindow(t0 = Ssel_readout0 = new BluebananaSSReadout0(plugin, this, tumbler_text_ww));
			add_subwindow(t1 = Ssel_readout1 = new BluebananaSSReadout1(plugin, this, tumbler_text_ww));
			add_subwindow(t2 = Ssel_readout2 = new BluebananaSSReadout2(plugin, this, tumbler_text_ww));
			add_subwindow(a = Ssel_active = new BluebananaSActive(plugin, this));
			add_subwindow(p = new BluebananaSPicker(this, tumbler_col2_w));
			s = Ssel_slider = new BluebananaSSSlider(plugin, this, slider_x, y, slider_w, row_h);
			break;

		case 2:
			add_subwindow(t0 = Vsel_readout0 = new BluebananaVSReadout0(plugin, this, tumbler_text_ww));
			add_subwindow(t1 = Vsel_readout1 = new BluebananaVSReadout1(plugin, this, tumbler_text_ww));
			add_subwindow(t2 = Vsel_readout2 = new BluebananaVSReadout2(plugin, this, tumbler_text_ww));
			add_subwindow(a = Vsel_active = new BluebananaVActive(plugin, this));
			add_subwindow(p = new BluebananaVPicker(this, tumbler_col2_w));
			s = Vsel_slider = new BluebananaVSSlider(plugin, this, slider_x, y, slider_w, row_h);
			break;

		case 3:
			add_subwindow(a = Fsel_active = new BluebananaFActive(plugin, this));
			s = Fsel_slider = new BluebananaFSSlider(plugin, this, slider_x, y, slider_w, row_h);

			Fsel_readout0->reposition_window(tumbler_col1_x, y + (row_h - tumbler_h) / 2 + 1);
			Fsel_readout1->reposition_window(tumbler_col1_x + tumbler_w, y + (row_h - tumbler_h) / 2 + 1);
			Fsel_readout2->reposition_window(tumbler_col1_x + tumbler_w * 2, y + (row_h - tumbler_h) / 2 + 1);
			Fsel_readout3->reposition_window(tumbler_col1_x + tumbler_w * 3 + padx * 2,
						y + (row_h - tumbler_h) / 2 + 1);
			{
				int x = get_w() - xmargin - erode->get_w();
				erode->reposition_window(x, y + (row_h - erode->get_h()) / 2);
				erode_label->reposition_window(x - erode_label->get_w() - padx, y + (row_h - erode_label->get_h()) / 2);
			}
			break;
		case 4:
			add_subwindow(t0 = Radj_readout0 = new BluebananaRAReadout0(plugin, this, tumbler_text_ww));
			add_subwindow(t1 = Radj_readout1 = new BluebananaRAReadout1(plugin, this, tumbler_text_ww));
			add_subwindow(t2 = Radj_readout2 = new BluebananaRAReadout2(plugin, this, tumbler_text_ww));
			add_subwindow(a = Radj_active = new BluebananaRAActive(plugin, this));
			add_subwindow(p = new BluebananaRAReset(this, tumbler_col2_w));
			s = Radj_slider = new BluebananaRASlider(plugin, this, slider_x, y, slider_w, row_h);
			break;

		case 5:
			add_subwindow(t0 = Gadj_readout0 = new BluebananaGAReadout0(plugin, this, tumbler_text_ww));
			add_subwindow(t1 = Gadj_readout1 = new BluebananaGAReadout1(plugin, this, tumbler_text_ww));
			add_subwindow(t2 = Gadj_readout2 = new BluebananaGAReadout2(plugin, this, tumbler_text_ww));
			add_subwindow(a = Gadj_active = new BluebananaGAActive(plugin, this));
			add_subwindow(p = new BluebananaGAReset(this, tumbler_col2_w));
			s = Gadj_slider = new BluebananaGASlider(plugin, this, slider_x, y, slider_w, row_h);
			break;

		case 6:
			add_subwindow(t0 = Badj_readout0 = new BluebananaBAReadout0(plugin, this, tumbler_text_ww));
			add_subwindow(t1 = Badj_readout1 = new BluebananaBAReadout1(plugin, this, tumbler_text_ww));
			add_subwindow(t2 = Badj_readout2 = new BluebananaBAReadout2(plugin, this, tumbler_text_ww));
			add_subwindow(a = Badj_active = new BluebananaBAActive(plugin, this));
			add_subwindow(p = new BluebananaBAReset(this, tumbler_col2_w));
			s = Badj_slider = new BluebananaBASlider(plugin, this, slider_x, y, slider_w, row_h);
			break;

		case 7:
			add_subwindow(t0 = Hadj_readout = new BluebananaHAReadout(plugin, this, tumbler_text_ww));
			add_subwindow(a = Hadj_active = new BluebananaHAActive(plugin, this));
			add_subwindow(p = new BluebananaHAReset(this, tumbler_col2_w));
			s = Hadj_slider = new BluebananaHASlider(plugin, this, slider_x, y, slider_w, row_h);
			break;

		case 8:
			add_subwindow(t0 = Sadj_readout0 = new BluebananaSAReadout0(plugin, this, tumbler_text_ww));
			add_subwindow(t1 = Sadj_readout1 = new BluebananaSAReadout1(plugin, this, tumbler_text_ww));
			add_subwindow(t2 = Sadj_readout2 = new BluebananaSAReadout2(plugin, this, tumbler_text_ww));

			add_subwindow(a = Sadj_active = new BluebananaSAActive(plugin, this));
			add_subwindow(p = new BluebananaSAReset(this, tumbler_col2_w));
			s = Sadj_slider = new BluebananaSASlider(plugin, this, slider_x, y, slider_w, row_h);
			break;

		case 9:
			add_subwindow(t0 = Vadj_readout0 = new BluebananaVAReadout0(plugin, this, tumbler_text_ww));
			add_subwindow(t1 = Vadj_readout1 = new BluebananaVAReadout1(plugin, this, tumbler_text_ww));
			add_subwindow(t2 = Vadj_readout2 = new BluebananaVAReadout2(plugin, this, tumbler_text_ww));
			add_subwindow(a = Vadj_active = new BluebananaVAActive(plugin, this));
			add_subwindow(p = new BluebananaVAReset(this, tumbler_col2_w));
			s = Vadj_slider = new BluebananaVASlider(plugin, this, slider_x, y, slider_w, row_h);
			break;
		case 10:
			add_subwindow(t0 = Oadj_readout = new BluebananaOAReadout(plugin, this, tumbler_text_ww));
			add_subwindow(a = Oadj_active = new BluebananaOAActive(plugin, this));
			add_subwindow(p = new BluebananaOAReset(this, tumbler_col2_w));
			s = Oadj_slider = new BluebananaOASlider(plugin, this, slider_x, y, slider_w, row_h);
			break;
		}
		if(s)
			add_subwindow(s);

		if(a)
		{
			a->reposition_window(label_x - padx - a->get_w(), y + (row_h - a->get_h()) / 2 + 1);
			if(l)
				l->reposition(label_x - l->get_w() - padx * 2 - a->get_w(), y + (row_h - l->get_h()) / 2 + 1);
		}

		if(p)
		{
			p->BC_SubWindow::reposition_window(tumbler_col2_x, y + (row_h - p->get_h()) / 2 + 1,
				MAX(tumbler_col2_w, picker_w), p->get_h());

			// work around bug; the reposition step does not fully redraw the button
			p->draw_face();
		}

		if(t0) t0->reposition_window(tumbler_col1_x,
			y + (row_h - tumbler_h) / 2 + 1);
		if(t1) t1->reposition_window(tumbler_col1_x + tumbler_ww,
			y + (row_h - tumbler_h) / 2 + 1);
		if(t2) t2->reposition_window(tumbler_col1_x + tumbler_ww * 2 + padx * 2 + (tumbler_ww - t2->get_w()) / 2,
			y + (row_h - tumbler_h) / 2 + 1);
		s->update();

		y += row_adv;
		if(i == 3)
		{
			y += row_adv / 3;

			BC_Title *l = new BC_Title(xmargin, y, _("Color Adjustment"));
			BC_Title *l2 = new BC_Title(-1, -1, _(" Filter Active"));

			add_subwindow(l);
			add_subwindow(l2);
			add_subwindow(active = new BluebananaActive(plugin, this));

			int x0 = get_w() - xmargin - mark->get_w();

			active->reposition_window(x0, y - (active->get_h() - l->get_h()) / 2);
			x0 -= padx + l2->get_w();
			l2->reposition_window(x0, y);
			x0 -= padx * 2;
			set_color(get_resources()->default_text_color);
			draw_line(xmargin + l->get_w() + padx, y + l->get_h()*.5, x0, y + l->get_h()*.5);

			y += l->get_h() * (row_padding + 1.);
			y += row_adv / 3;
		}

		if(i == 6 || i == 9)
		{
			y += row_adv / 4;

			set_color((s->dimtextcolor + get_resources()->default_text_color) / 2);
			draw_line(slider_x + 20, y + l->get_h()*.5, tumbler_col2_x - 30, y + l->get_h()*.5);

			y += l->get_h() * (row_padding + 1.);
			y += row_adv / 4;
		}
	}
	y += row_adv / 2 - 4;
	plugin->update_lookups(0);
	do_render = 1;

	resize_window(get_w(), y);

	reposition_window(windowx, windowy, get_w(), y);
	leave_config_change(); // also forces render
	update();
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

// adds one to config push refcount
// updates any internal state immediately
void BluebananaWindow::enter_config_change()
{
	config_refcount++;
	if(!config_change && !plugin->update_cache.equivalent(plugin->config))
	{
		config_change = 1;
	}
	plugin->update_lookups(0);
}

// decrements one from config push refcount.  If refcount drops to
// zero, pushes new config up to application

// also compresses events; waits 184ms for new events before pushing
// configuration changes
void BluebananaWindow::commit_config_change()
{
	if(--config_refcount == 0)
	{
		if(config_change)
		{
			config_change = 0;
			config_produce++;
			config_pending = 1;
			set_repeat(97);
		}
		render();
	}
}

// decrements one from config push refcount.  Does not push config up
// to application when refcount drops to zero (used to wrap update
// requests coming from the application, not user-initiated state
// changes)
void BluebananaWindow::leave_config_change()
{
	config_change = 0;
	plugin->update_cache.copy_from(plugin->config);
	if(--config_refcount == 0)
	{
		render();
	}
}

void BluebananaWindow::flush_config_change()
{
	unset_repeat(97);
	if(config_pending)
	{
		config_pending = 0;
		plugin->update_cache.copy_from(plugin->config);
		plugin->send_configure_change();
	}
	config_consume = config_produce;
}

void BluebananaWindow::repeat_event(int d)
{
	if(d == 97)
	{
		if(config_consume == config_produce)
			flush_config_change();
		config_consume = config_produce;
	}
}

// engine -> gui update; don't allow any EDL pushes 
void BluebananaWindow::update()
{
	// called to suppress configuration pushes
	enter_config_change();

	// full configuration recompute and redraw
	Hsel_slider->update();
	Ssel_slider->update();
	Vsel_slider->update();
	Fsel_slider->update();
	Hadj_slider->update();
	Sadj_slider->update();
	Vadj_slider->update();
	Radj_slider->update();
	Gadj_slider->update();
	Badj_slider->update();
	Oadj_slider->update();

	active->update();
	mark->update();
	use_mask->update();
	capture_mask->update();
	invert_selection->update();

	Hsel_active->update();
	Ssel_active->update();
	Vsel_active->update();
	Fsel_active->update();

	Hadj_active->update();
	Sadj_active->update();
	Vadj_active->update();
	Radj_active->update();
	Gadj_active->update();
	Badj_active->update();
	Oadj_active->update();
	update_histograms();

	// called to release configuration without pushing
	leave_config_change();
}

void BluebananaWindow::render()
{
	if(do_render)
	{
		Hsel_slider->render();
		Ssel_slider->render();
		Vsel_slider->render();
		Fsel_slider->render();
		Hadj_slider->render();
		Sadj_slider->render();
		Vadj_slider->render();
		Radj_slider->render();
		Gadj_slider->render();
		Badj_slider->render();
		Oadj_slider->render();
	}
}

void BluebananaWindow::update_histograms()
{
	int w = plugin->frame->get_w();
	int h = plugin->frame->get_h();

	if(Radj_slider)Radj_slider->update_histogram(plugin->red_histogram, 0, 0, 0, w * h);
	if(Gadj_slider)Gadj_slider->update_histogram(plugin->green_histogram, 0, 0, 0, w * h);
	if(Badj_slider)Badj_slider->update_histogram(plugin->blue_histogram, 0, 0, 0, w * h);

	if(Hadj_slider)
		Hadj_slider->update_histogram(plugin->hue_histogram,
			plugin->hue_histogram_red,
			plugin->hue_histogram_green,
			plugin->hue_histogram_blue, w * h);

	if(Sadj_slider)
		Sadj_slider->update_histogram(plugin->sat_histogram,
			plugin->sat_histogram_red,
			plugin->sat_histogram_green,
			plugin->sat_histogram_blue, w * h);

	if(Vadj_slider)
		Vadj_slider->update_histogram(plugin->value_histogram,
			plugin->value_histogram_red,
			plugin->value_histogram_green,
			plugin->value_histogram_blue, w * h);
}
