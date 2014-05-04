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

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bluebanana.h"
#include "bluebananaconfig.h"

static float halfsin2(float x)
{
	if(x < -M_PI / 2)return 0;
	if(x > M_PI / 2)return 1;
	return .5 + .5 * sin(x);
}

static float scale_gamma(float hdel, float lo, float hi, float gamma)
{
	lo /= 100.;
	hi /= 100.;

	// apply gamma
	if(hdel < 0)
	{
		// this is contrived, so do something that looks good
		// poor man's tangent
		hdel *= pow(.001, 1 / gamma) * 1000;
	}
	else
	{
		hdel = pow(hdel, 1 / gamma);
	}

	// apply range scaling
	return hdel * (hi - lo) + lo;

}

static int select_setup(char *pattern, int change)
{
	int n = 0;
	if(change)
	{
		float gsq = change * change + 1.1;
		int x = -1;
		int y = abs(change);

		while(1)
		{
			while(x * x + y * y < gsq)
			{
				x--;
				if(n == FSrange * 4)
				{
					fprintf(stderr, "Internal error; pattern array overflow\n");
					return n;
				}
				pattern[n++] = (change > 0 ? 'H' : 'h');
			}
			if(!y)
				break;
			y--;
			if(n == FSrange * 4)
			{
				fprintf(stderr, "Internal error; pattern array overflow\n");
				return n;
			}
			pattern[n++] = (change > 0 ? 'V' : 'v');
		}
	}

	return n;
}

void BluebananaMain::update_lookups(int serverside)
{
	// compare config contents with current lookup table state.  Update
	// as needed.

	int i;

	// Alpha lookup table for hue selection
	if(lookup_cache.Hsel_lo != config.Hsel_lo ||
			lookup_cache.Hsel_hi != config.Hsel_hi ||
			lookup_cache.Hsel_over != config.Hsel_over)
	{

		float scale = M_PI / (config.Hsel_over + .0000001);
		for(i = 0; i < SELECT_LOOKUP_SIZE + 2; i++)
		{
			// this is a circular scale; the overlap wraps around to the opposite side
			float hue = (float)i / SELECT_LOOKUP_SIZE * 360.;
			float l0 = (hue - config.Hsel_lo - 360.f) * scale;
			float l1 = (hue - config.Hsel_lo) * scale;
			float l2 = (hue - config.Hsel_lo + 360.f) * scale;
			float l3 = (hue - config.Hsel_lo + 720.f) * scale;
			float h0 = (hue - config.Hsel_hi - 360.f) * scale;
			float h1 = (hue - config.Hsel_hi) * scale;
			float h2 = (hue - config.Hsel_hi + 360.f) * scale;
			float h3 = (hue - config.Hsel_hi + 720.f) * scale;

			float a0 = halfsin2(l0) * (1. - halfsin2(h0));
			float a1 = halfsin2(l1) * (1. - halfsin2(h1));
			float a2 = halfsin2(l2) * (1. - halfsin2(h2));
			float a3 = halfsin2(l3) * (1. - halfsin2(h3));

			hue_select_alpha_lookup[i] = a0 + a1 + a2 + a3;
		}
	}

	// Alpha lookup table for saturation selection
	if(lookup_cache.Ssel_lo != config.Ssel_lo ||
			lookup_cache.Ssel_hi != config.Ssel_hi ||
			lookup_cache.Ssel_over != config.Ssel_over)
	{
		float scale = M_PI / (config.Ssel_over + .0000001);
		for(i = 0; i < SELECT_LOOKUP_SIZE + 2; i++)
		{
			// this is a bounded scale; the overlap reflects off the boundary
			float sat = (float)i / SELECT_LOOKUP_SIZE * 100.;
			float l0 = (-sat - config.Ssel_lo) * scale;
			float h0 = (-sat - config.Ssel_hi) * scale;
			float l1 = (sat - config.Ssel_lo) * scale;
			float h1 = (sat - config.Ssel_hi) * scale;
			float l2 = ((200. - sat) - config.Ssel_lo) * scale;
			float h2 = ((200. - sat) - config.Ssel_hi) * scale;
			sat_select_alpha_lookup[i] =
				halfsin2(l0) * (1. - halfsin2(h0)) +
				halfsin2(l1) * (1. - halfsin2(h1)) +
				halfsin2(l2) * (1. - halfsin2(h2));
		}
	}

	// Alpha lookup table for value selection
	if(lookup_cache.Vsel_lo != config.Vsel_lo ||
			lookup_cache.Vsel_hi != config.Vsel_hi ||
			lookup_cache.Vsel_over != config.Vsel_over)
	{
		float scale = M_PI / (config.Vsel_over + .0000001);
		for(i = 0; i < SELECT_LOOKUP_SIZE + 2; i++)
		{
			// this is a bounded scale; the overlap reflects off the boundary
			float val = (float)i / SELECT_LOOKUP_SIZE * 100.;
			float l0 = (-val - config.Vsel_lo) * scale;
			float h0 = (-val - config.Vsel_hi) * scale;
			float l1 = (val - config.Vsel_lo) * scale;
			float h1 = (val - config.Vsel_hi) * scale;
			float l2 = ((200. - val) - config.Vsel_lo) * scale;
			float h2 = ((200. - val) - config.Vsel_hi) * scale;
			val_select_alpha_lookup[i] =
				halfsin2(l0) * (1. - halfsin2(h0)) +
				halfsin2(l1) * (1. - halfsin2(h1)) +
				halfsin2(l2) * (1. - halfsin2(h2));
		}
	}

	if(serverside)
	{
		// lookup table for saturation adjustment
		if(lookup_cache.Sadj_lo != config.Sadj_lo ||
				lookup_cache.Sadj_hi != config.Sadj_hi ||
				lookup_cache.Sadj_gamma != config.Sadj_gamma)
		{
			for(i = 0; i < ADJUST_LOOKUP_SIZE + 2; i++)
			{
				sat_adj_lookup[i] = scale_gamma ((float)i / ADJUST_LOOKUP_SIZE*MAX_ADJ_HEADROOM,
						config.Sadj_lo, config.Sadj_hi, config.Sadj_gamma);
			}
			sat_adj_toe_slope = -scale_gamma (-1, config.Sadj_lo, config.Sadj_hi, config.Sadj_gamma);
		}

		// lookup table for value adjustment
		if(lookup_cache.Vadj_lo != config.Vadj_lo ||
				lookup_cache.Vadj_hi != config.Vadj_hi ||
				lookup_cache.Vadj_gamma != config.Vadj_gamma)
		{
			if(config.Vadj_gamma < 1.0)
			{
				// scaling mode for 'darkening' gamma
				for(i = 0; i < ADJUST_LOOKUP_SIZE + 2; i++)
				{
					float V0 = (float)i / ADJUST_LOOKUP_SIZE * MAX_ADJ_HEADROOM;
					float V1 = scale_gamma((float)i / ADJUST_LOOKUP_SIZE * MAX_ADJ_HEADROOM,
						0, config.Vadj_hi - config.Vadj_lo,
						config.Vadj_gamma);
					val_adj_lookup[i] = V0 ? V1 / V0 : 0;
				}
				val_adj_toe_slope = -scale_gamma (-1, 0, config.Vadj_hi - config.Vadj_lo, config.Vadj_gamma);
			}
			else
			{
				// shifting mode for 'brightening' gamma
				for(i = 0; i < ADJUST_LOOKUP_SIZE + 2; i++)
				{
					float V0 = (float)i / ADJUST_LOOKUP_SIZE * MAX_ADJ_HEADROOM;
					float V1 = scale_gamma((float)i / ADJUST_LOOKUP_SIZE * MAX_ADJ_HEADROOM,
						config.Vadj_lo, config.Vadj_lo + 100, config.Vadj_gamma);
					val_adj_lookup[i] = V1 - V0;
				}
				val_adj_toe_slope = -scale_gamma(-1, config.Vadj_lo, config.Vadj_lo+100., config.Vadj_gamma);
			}
		}

		// lookup table for red adjustment
		if(lookup_cache.Radj_lo != config.Radj_lo ||
				lookup_cache.Radj_hi != config.Radj_hi ||
				lookup_cache.Radj_gamma != config.Radj_gamma)
		{
			for(i = 0; i < ADJUST_LOOKUP_SIZE + 2; i++)
				red_adj_lookup[i] = scale_gamma((float)i / ADJUST_LOOKUP_SIZE * MAX_ADJ_HEADROOM,
					config.Radj_lo,config.Radj_hi, config.Radj_gamma);
			red_adj_toe_slope = -scale_gamma(-1, config.Radj_lo, config.Radj_hi, config.Radj_gamma);
		}

		// lookup table for green adjustment
		if(lookup_cache.Gadj_lo != config.Gadj_lo ||
				lookup_cache.Gadj_hi != config.Gadj_hi ||
				lookup_cache.Gadj_gamma != config.Gadj_gamma)
		{
			for(i = 0; i < ADJUST_LOOKUP_SIZE + 2; i++)
				green_adj_lookup[i] = scale_gamma((float)i / ADJUST_LOOKUP_SIZE * MAX_ADJ_HEADROOM,
					config.Gadj_lo,config.Gadj_hi, config.Gadj_gamma);
			green_adj_toe_slope = -scale_gamma (-1, config.Gadj_lo, config.Gadj_hi, config.Gadj_gamma);
		}

		// lookup table for blue adjustment
		if(lookup_cache.Badj_lo != config.Badj_lo ||
				lookup_cache.Badj_hi != config.Badj_hi ||
				lookup_cache.Badj_gamma != config.Badj_gamma)
		{
			for(i = 0; i < ADJUST_LOOKUP_SIZE + 2; i++)
				blue_adj_lookup[i] = scale_gamma((float)i / ADJUST_LOOKUP_SIZE * MAX_ADJ_HEADROOM,
					config.Badj_lo, config.Badj_hi, config.Badj_gamma);
			blue_adj_toe_slope = -scale_gamma(-1, config.Badj_lo, config.Badj_hi, config.Badj_gamma);
		}
	}

	if(lookup_cache.Fsel_erode != config.Fsel_erode ||
			lookup_cache.Fsel_lo != config.Fsel_lo ||
			lookup_cache.Fsel_mid != config.Fsel_mid ||
			lookup_cache.Fsel_hi != config.Fsel_hi)
	{
		select_one_n = 0;
		select_two_n = 0;
		select_three_n = 0;

		if(config.Fsel_erode)
		{
			// shrink-first mode
			select_one_n = select_setup(select_one, config.Fsel_lo);
			if(config.Fsel_hi)
			{
				select_two_n = select_setup(select_two, config.Fsel_hi - config.Fsel_lo);
				select_three_n = select_setup(select_three, config.Fsel_mid - config.Fsel_hi);
			}
			else
			{
				select_two_n = select_setup(select_two,config.Fsel_mid-config.Fsel_lo);
			}
		}
		else
		{
			// grow-first mode
			select_one_n = select_setup(select_one, config.Fsel_hi);
			if(config.Fsel_lo)
			{
				select_two_n = select_setup(select_two, config.Fsel_lo - config.Fsel_hi);
				select_three_n = select_setup(select_three, config.Fsel_mid - config.Fsel_lo);
			}
			else
			{
				select_two_n = select_setup(select_two, config.Fsel_mid - config.Fsel_hi);
			}
		}
	}

	lookup_cache.copy_from(config);
}

// [0..360]
float BluebananaMain::hue_select_alpha(float hue)
{
	if(config.Hsel_active)
	{
		float huewarp = hue * (SELECT_LOOKUP_SIZE * .002777778f);
		float hueint = floor(huewarp);
		float del = huewarp - hueint;
		return hue_select_alpha_lookup[(int)hueint] * (1.f - del) + hue_select_alpha_lookup[(int)hueint + 1] * del;
	}
	else
		return 1.f;
}

// [0..1]
float BluebananaMain::sat_select_alpha(float sat)
{
	if(config.Ssel_active)
	{
		float satwarp = sat * SELECT_LOOKUP_SIZE;
		float satint = floor(satwarp);
		float del = satwarp - satint;
		return sat_select_alpha_lookup[(int)satint] * (1.f - del) + sat_select_alpha_lookup[(int)satint + 1] * del;
	}
	else
		return 1.f;
}

// [0..1]
float BluebananaMain::val_select_alpha(float val)
{
	if(config.Vsel_active)
	{
		float valwarp = val * SELECT_LOOKUP_SIZE;
		float valint = floor(valwarp);
		float del = valwarp - valint;
		return val_select_alpha_lookup[(int)valint] * (1.f - del) + val_select_alpha_lookup[(int)valint + 1] * del;
	}
	else
		return 1.f;
}

// [0..1] HSV selection lookup must be sanitized as S and V values can
// exceed [0..1].  Clamping is OK, as out of range values will still
// be selected at range limits.
float sel_lookup(float val, float *lookup)
{
	float bin;
	float del = modff(CLAMP(val, 0.f, 1.f) * SELECT_LOOKUP_SCALE, &bin);
	return lookup[(int)bin] * (1.f - del) + lookup[(int)bin + 1] * del;
}

// [0..4] == lookup range
// below 0, lookups are linear
// clamp above 4
float adj_lookup(float val, float *lookup, float toe_slope)
{
	if(val < 0)
	{
		return val * toe_slope + lookup[0];
	}
	else
	{
		if(val > 4.f) val = 4.f;
		float bin;
		float del = modff(val * ADJUST_LOOKUP_SCALE, &bin);
		return lookup[(int)bin] * (1.f - del) + lookup[(int)bin + 1] * del;
	}
}
