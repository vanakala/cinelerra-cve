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
#include <unistd.h>

#include "bluebanana.h"
#include "bluebananaconfig.h"
#include "loadbalance.h"
#include "bluebananacolor.c" // need the inlines

BluebananaPackage::BluebananaPackage(BluebananaEngine *engine)
 : LoadPackage()
{
	this->engine = engine;
}

BluebananaUnit::BluebananaUnit(BluebananaEngine *server, BluebananaMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}

void BluebananaUnit::process_package(LoadPackage *package)
{
	BluebananaPackage *pkg = (BluebananaPackage*)package;
	BluebananaEngine *engine = (BluebananaEngine*)pkg->engine;

	VFrame *frame = engine->data;
	int w = frame->get_w();
	int h = frame->get_h();
	int ant = plugin->ants_counter;
	int gui_open = plugin->gui_open();
	int show_ants = plugin->config.mark && gui_open;
	int i, j;
	int active = plugin->config.active;
	int use_mask = plugin->config.use_mask;
	int capture_mask = plugin->config.capture_mask;
	int invert_selection = plugin->config.invert_selection;

	float *Hl = (plugin->config.Hsel_active &&
		(plugin->config.Hsel_lo != 0 ||
		plugin->config.Hsel_hi != 360)) ? plugin->hue_select_alpha_lookup : NULL;
	float *Sl = (plugin->config.Ssel_active &&
		(plugin->config.Ssel_lo != 0  ||
		plugin->config.Ssel_hi != 100)) ? plugin->sat_select_alpha_lookup : NULL;
	float *Vl = (plugin->config.Vsel_active &&
		(plugin->config.Vsel_lo != 0 ||
		plugin->config.Vsel_hi != 100)) ? plugin->val_select_alpha_lookup : NULL;

	float Hal = plugin->config.Hadj_active ? plugin->config.Hadj_val / 60.f : 0.f;

	float *Sal = (plugin->config.Sadj_active &&
		(plugin->config.Sadj_lo != 0 ||
		plugin->config.Sadj_hi != 100 ||
		plugin->config.Sadj_gamma != 1)) ? plugin->sat_adj_lookup : NULL;
	float *Val = (plugin->config.Vadj_active &&
		(plugin->config.Vadj_lo != 0 ||
		plugin->config.Vadj_hi != 100 ||
		plugin->config.Vadj_gamma != 1)) ? plugin->val_adj_lookup : NULL;
	float *Ral = (plugin->config.Radj_active &&
		(plugin->config.Radj_lo != 0 ||
		plugin->config.Radj_hi != 100 ||
		plugin->config.Radj_gamma != 1)) ? plugin->red_adj_lookup : NULL;
	float *Gal = (plugin->config.Gadj_active &&
		(plugin->config.Gadj_lo != 0 ||
		plugin->config.Gadj_hi != 100 ||
		plugin->config.Gadj_gamma != 1)) ? plugin->green_adj_lookup : NULL;
	float *Bal = (plugin->config.Badj_active &&
		(plugin->config.Badj_lo != 0 ||
		plugin->config.Badj_hi != 100 ||
		plugin->config.Badj_gamma != 1)) ? plugin->blue_adj_lookup : NULL;

	float Sas = plugin->sat_adj_toe_slope;
	float Vas = plugin->val_adj_toe_slope;
	float Ras = plugin->red_adj_toe_slope;
	float Gas = plugin->green_adj_toe_slope;
	float Bas = plugin->blue_adj_toe_slope;

	float Aal = plugin->config.Oadj_active ? plugin->config.Oadj_val * .01 : 1.f;

	float Vscale = (plugin->config.Vadj_hi - plugin->config.Vadj_lo) / 100.f;
	float Vshift = plugin->config.Vadj_lo / 100.f;
	float Vgamma = plugin->config.Vadj_gamma;

	int doRGB = Ral || Gal || Bal;

	int doHSV = (Hal != 0) || Sal || Val;

	int doSEL = ( Hl || Sl || Vl ) && (active || show_ants);

	int shaping = plugin->config.Fsel_active && doSEL &&
		(plugin->config.Fsel_lo || plugin->config.Fsel_hi || plugin->config.Fsel_over);

	int byte_advance = 0;
	int have_alpha = 1;

#define SPLIT 128

	int tasks = engine->get_total_packages() * 16;
	int taski, rowi, coli;

	/* do as much work entirely local to thread memory as possible */
	unsigned char row_fragment[SPLIT * 16];
	float *selection_fullframe = NULL;
	float  selection[SPLIT];

	float Rvec[SPLIT];
	float Gvec[SPLIT];
	float Bvec[SPLIT];

	float Avec[SPLIT];
	float Hvec[SPLIT];
	float Svec[SPLIT];
	float Vvec[SPLIT];

	float *Rhist = pkg->Rhist;
	float *Ghist = pkg->Ghist;
	float *Bhist = pkg->Bhist;
	float *Hhist = pkg->Hhist;
	float *Shist = pkg->Shist;
	float *Vhist = pkg->Vhist;
	float Htotal = 0.f;
	float Hweight = 0.f;

	float *Hhr = pkg->Hhr;
	float *Hhg = pkg->Hhg;
	float *Hhb = pkg->Hhb;
	float *Shr = pkg->Shr;
	float *Shg = pkg->Shg;
	float *Shb = pkg->Shb;
	float *Vhr = pkg->Vhr;
	float *Vhg = pkg->Vhg;
	float *Vhb = pkg->Vhb;

	memset(Rhist, 0, sizeof(pkg->Rhist));
	memset(Ghist, 0, sizeof(pkg->Ghist));
	memset(Bhist, 0, sizeof(pkg->Bhist));
	memset(Hhist, 0, sizeof(pkg->Hhist));
	memset(Shist, 0, sizeof(pkg->Shist));
	memset(Vhist, 0, sizeof(pkg->Vhist));

	memset(Hhr, 0, sizeof(pkg->Hhr));
	memset(Hhg, 0, sizeof(pkg->Hhg));
	memset(Hhb, 0, sizeof(pkg->Hhb));
	memset(Shr, 0, sizeof(pkg->Shr));
	memset(Shg, 0, sizeof(pkg->Shg));
	memset(Shb, 0, sizeof(pkg->Shb));
	memset(Vhr, 0, sizeof(pkg->Vhr));
	memset(Vhg, 0, sizeof(pkg->Vhg));
	memset(Vhb, 0, sizeof(pkg->Vhb));

	// If we're doing fill shaping, we need to compute base selection
	//  for the entire frame before shaping.

	if(shaping)
	{
		engine->set_task(tasks * 2, "shaping_even");
		while ((taski = engine->next_task()) >= 0)
		{

			// operate on discontinuous, interleaved sections of the source
			//   buffer in two passes.  Although we could take extra steps to
			//   make cache contention across cores completely impossible, the
			//   extra locking and central join required isn't worth it.  It's
			//   preferable to make contention merely highly unlikely.

			int start_row, end_row;
			if(taski < tasks)
			{
				start_row = (taski * 2) * h / (tasks * 2);
				end_row = (taski * 2 + 1) * h / (tasks * 2);
			}
			else
			{
				start_row = ((taski - tasks) * 2 + 1) * h / (tasks * 2);
				end_row = ((taski - tasks) * 2 + 2) * h / (tasks * 2);
			}

			for(rowi = start_row; rowi < end_row; rowi++)
			{
				unsigned char *row = frame->get_row_ptr(rowi);

				for(coli = 0; coli < w; coli += SPLIT)
				{

					int todo = (w - coli > SPLIT) ? SPLIT : (w - coli);
					float *A = engine->selection_workA + w * rowi + coli;

					switch(frame->get_color_model())
					{
					case BC_RGB888:
						rgb8_to_RGB((unsigned char *)row, Rvec, Gvec, Bvec, todo);
						row += todo * 3;
						break;
					case BC_RGBA8888:
						rgba8_to_RGBA((unsigned char *)row, Rvec, Gvec, Bvec, Avec, todo);
						row += todo * 4;
						break;
					case BC_RGB_FLOAT:
						rgbF_to_RGB((float *)row, Rvec, Gvec, Bvec, todo);
						row += todo * 12;
						break;
					case BC_RGBA_FLOAT:
						rgbaF_to_RGBA((float *)row, Rvec, Gvec, Bvec, Avec, todo);
						row += todo * 16;
						break;
					case BC_YUV888:
						yuv8_to_RGB((unsigned char *)row, Rvec, Gvec, Bvec, todo);
						row += todo * 3;
						break;
					case BC_YUVA8888:
						yuva8_to_RGBA((unsigned char *)row, Rvec, Gvec, Bvec, Avec, todo);
						row += todo * 4;
						break;
					case BC_RGBA16161616:
						rgba16_to_RGBA((uint16_t *)row, Rvec, Gvec, Bvec, Avec, todo);
						row += todo * 8;
						break;
					case BC_AYUV16161616:
						ayuv16_to_RGBA((uint16_t *)row, Rvec, Gvec, Bvec, Avec, todo);
						row += todo * 8;
						break;
					}

					for(j = 0; j < todo; j++)
						RGB_to_HSpV(Rvec[j], Gvec[j], Bvec[j], Hvec[j], Svec[j], Vvec[j]);

					if(Hl)
						for(j = 0; j < todo; j++)
							selection[j] = sel_lookup(Hvec[j] * .166666667f, Hl);
					else
						for(j = 0; j < todo; j++)
							selection[j] = 1.f;

					if(Sl)
						for(j = 0; j < todo; j++)
							selection[j] *= sel_lookup(Svec[j], Sl);

					if(Vl)
						for(j = 0; j < todo; j++)
							selection[j] *= sel_lookup(Vvec[j], Vl);

					// lock the memcpy to prevent pessimal cache coherency
					//   interleave across cores.
					pthread_mutex_lock(&engine->copylock);
					memcpy(A, selection, sizeof(*selection) * todo);
					pthread_mutex_unlock(&engine->copylock);
				}
			}
		}

		// Perform fill shaping on the selection
		selection_fullframe = plugin->fill_selection(engine->selection_workA,
			engine->selection_workB, w, h, engine);
	}

	// row-by-row color modification and histogram feedback
	engine->set_task(tasks * 2, "modification_even");
	while((taski = engine->next_task()) >= 0)
	{

		// operate on discontinuous, interleaved sections of the source
		//   buffer in two passes.  Although we could take extra steps to
		//   make cache contention across cores completely impossible, the
		//   extra locking and central join required isn't worth it.  It's
		//   preferable to make contention merely highly unlikely.

		int start_row, end_row;
		if(taski < tasks)
		{
			start_row = (taski * 2) * h / (tasks * 2);
			end_row = (taski * 2 + 1) * h / (tasks * 2);
		}
		else
		{
			start_row = ((taski - tasks) * 2 + 1) * h / (tasks * 2);
			end_row = ((taski - tasks) * 2 + 2) * h / (tasks * 2);
		}

		for(rowi = start_row; rowi < end_row; rowi++)
		{
			unsigned char *row = frame->get_row_ptr(rowi);

			for(int coli=0; coli < w; coli += SPLIT)
			{
				int todo = (w - coli > SPLIT) ? SPLIT : (w - coli);
				int have_selection = 0;

				/* convert from pipeline color format */
				if(active || show_ants || (use_mask && capture_mask && have_alpha))
				{

					switch(frame->get_color_model())
					{
					case BC_RGB888:
						pthread_mutex_lock(&engine->copylock);
						memcpy(row_fragment, row, todo * 3);
						pthread_mutex_unlock(&engine->copylock);
						rgb8_to_RGB(row_fragment, Rvec, Gvec, Bvec, todo);
						byte_advance = todo * 3;
						have_alpha = 0;
						break;

					case BC_RGBA8888:
						pthread_mutex_lock(&engine->copylock);
						memcpy(row_fragment, row, todo * 4);
						pthread_mutex_unlock(&engine->copylock);
						rgba8_to_RGBA(row_fragment, Rvec, Gvec, Bvec, Avec, todo);
						byte_advance = todo * 4;
						have_alpha = 1;
						break;

					case BC_RGBA16161616:
						pthread_mutex_lock(&engine->copylock);
						memcpy(row_fragment, row, todo * 8);
						pthread_mutex_unlock(&engine->copylock);
						rgba16_to_RGBA((uint16_t*)row_fragment, Rvec, Gvec, Bvec, Avec, todo);
						byte_advance = todo * 8;
						have_alpha = 1;
						break;

					case BC_RGB_FLOAT:
						pthread_mutex_lock(&engine->copylock);
						memcpy(row_fragment, row, todo * 12);
						pthread_mutex_unlock(&engine->copylock);
						rgbF_to_RGB((float *)row_fragment, Rvec, Gvec, Bvec, todo);
						byte_advance = todo * 12;
						have_alpha = 0;
						break;

					case BC_RGBA_FLOAT:
						pthread_mutex_lock(&engine->copylock);
						memcpy(row_fragment, row, todo * 16);
						pthread_mutex_unlock(&engine->copylock);
						rgbaF_to_RGBA((float *)row_fragment, Rvec, Gvec, Bvec, Avec, todo);
						byte_advance = todo * 16;
						have_alpha = 1;
						break;

					case BC_YUV888:
						pthread_mutex_lock(&engine->copylock);
						memcpy(row_fragment, row, todo * 3);
						pthread_mutex_unlock(&engine->copylock);
						yuv8_to_RGB(row_fragment, Rvec, Gvec, Bvec, todo);
						byte_advance = todo * 3;
						have_alpha = 0;
						break;

					case BC_YUVA8888:
						pthread_mutex_lock(&engine->copylock);
						memcpy(row_fragment, row, todo * 4);
						pthread_mutex_unlock(&engine->copylock);
						yuva8_to_RGBA(row, Rvec, Gvec, Bvec, Avec, todo);
						byte_advance = todo * 4;
						have_alpha = 1;
						break;

					case BC_AYUV16161616:
						pthread_mutex_lock(&engine->copylock);
						memcpy(row_fragment, row, todo * 8);
						pthread_mutex_unlock(&engine->copylock);
						ayuv16_to_RGBA((uint16_t*)row_fragment, Rvec, Gvec, Bvec, Avec, todo);
						byte_advance = todo * 8;
						have_alpha = 1;
						break;
					}

					if(doSEL)
						// generate initial HSV values [if selection active]
						for(j = 0; j < todo; j++)
							RGB_to_HSpV(Rvec[j], Gvec[j], Bvec[j], Hvec[j], Svec[j], Vvec[j]);

					float selection_test = todo;

					if(selection_fullframe)
					{
						// get the full-frame selection data we need into thread-local storage
						// the full-frame data is read-only at this point, no need to lock
						float *sf = selection_fullframe + rowi * w + coli;
						selection_test = 0.f;
						for(j = 0; j < todo; j++)
							selection_test += selection[j] = sf[j];
						have_selection = 1;
					}
					else
					{
						// selection computation when no full-frame shaping
						if(Hl)
						{
							selection_test = 0.f;
							for(j = 0; j < todo; j++)
								selection_test += selection[j] = sel_lookup(Hvec[j] * .166666667f, Hl);
							have_selection = 1;
						}

						if(Sl)
						{
							if(have_selection)
							{
								if(selection_test > SELECT_THRESH)
								{
									selection_test = 0.f;
									for(j = 0; j < todo; j++)
										selection_test += selection[j] *= sel_lookup(Svec[j], Sl);
								}
							}
							else
							{
								selection_test = 0.f;
								for(j = 0; j < todo; j++)
									selection_test += selection[j] = sel_lookup(Svec[j], Sl);
								have_selection = 1;
							}
						}

						if(Vl)
						{
							if(have_selection)
							{
								if(selection_test > SELECT_THRESH)
								{
									selection_test = 0.f;
									for(j = 0; j < todo; j++)
										selection_test += selection[j] *= sel_lookup(Vvec[j], Vl);
								}
							}
							else
							{
								selection_test = 0.f;
								for(j = 0; j < todo; j++)
									selection_test += selection[j] = sel_lookup(Vvec[j], Vl);
								have_selection = 1;
							}
						}
					}

					// selection modification according to config
					if(use_mask && have_alpha)
					{
						if(!have_selection)
						{
							// selection consists only of mask
							selection_test = 0.;
							for(j = 0; j < todo; j++)
								selection_test += selection[j] = Avec[j];
							have_selection = 1;
						}
						else
						{
							if(invert_selection)
							{
								if(selection_test < SELECT_THRESH)
								{
									// fully selected after invert, clip to mask
									selection_test = 0.f;
									for(j = 0; j < todo; j++)
										selection_test += selection[j] = Avec[j];
								}
								else if (selection_test >= todo - SELECT_THRESH)
								{
									// fully deselected after invert
									selection_test = 0.;
								}
								else
								{
									// partial selection after invert, clip to mask
									selection_test = 0.f;
									for(j = 0; j < todo; j++)
										selection_test += selection[j] = Avec[j] * (1.f - selection[j]);
								}
							}
							else
							{
								if(selection_test < SELECT_THRESH)
								{
									// fully deselected
								}
								else if (selection_test >= todo - SELECT_THRESH)
								{
									// fully selected, clip to mask
									selection_test = 0.f;
									for(j = 0; j < todo; j++)
										selection_test += selection[j] = Avec[j];
								}
								else
								{
									// partial selection, clip to mask
									selection_test = 0.f;
									for(j = 0; j < todo; j++)
										selection_test += selection[j] *= Avec[j];
								}
							}
						}
						if(selection_test < SELECT_THRESH)
						{
							// skip processing this fragment
							// we're using a mask; if the mask is set to capture, we
							//   need to restore alpha before skipping
							if(capture_mask)
							{
								switch(frame->get_color_model())
								{
								case BC_RGBA8888:
									unmask_rgba8(row_fragment, todo);
									break;
								case BC_RGBA_FLOAT:
									unmask_rgbaF((float *)row_fragment, todo);
									break;
								case BC_YUVA8888:
									unmask_yuva8(row_fragment, todo);
									break;
								case BC_AYUV16161616:
									unmask_ayuv16((uint16_t*)row_fragment, todo);
									break;
								case BC_RGBA16161616:
									unmask_rgba16((uint16_t*)row_fragment, todo);
									break;
								}
								pthread_mutex_lock(&engine->copylock);
								memcpy(row, row_fragment, byte_advance);
								pthread_mutex_unlock(&engine->copylock);
							}

							row += byte_advance;
							continue;
						}
						if(selection_test > todo - SELECT_THRESH)
							have_selection = 0; // fully selected
					}
					else
					{
						if(have_selection)
						{
							if(invert_selection)
							{
								if(selection_test < SELECT_THRESH)
								{
									// fully selected after inversion
									selection_test = todo;
								}
								else if(selection_test >= todo - SELECT_THRESH)
								{
									// fully deselected after invert, skip fragment
									row += byte_advance;
									continue;
								}
								else
								{
									// partial selection
									selection_test = 0.f;
									for(j = 0; j < todo; j++)
										selection_test += selection[j] = (1.f - selection[j]);
								}
							}
							else
							{
								if(selection_test < SELECT_THRESH)
								{
									// fully deselected, skip fragment
									row += byte_advance;
									continue;
								}
								else if (selection_test >= todo-SELECT_THRESH)
								{
									// fully selected
									selection_test=todo;
								}
								else
								{
									// partial selection; already calculated
								}
							}
							if(selection_test < SELECT_THRESH)
							{
								row += byte_advance;
								continue; // inactive fragment
							}
							if(selection_test > todo - SELECT_THRESH)
								have_selection = 0; // fully selected
						}
						else
						{
							if(invert_selection)
							{
								row += byte_advance;
								continue;
							}
						}
					}
				}

				if(active)
				{
					// red adjust
					if(Ral)
						if(have_selection)
						{
							for(j = 0; j < todo; j++)
								if(selection[j] > SELECT_THRESH) Rvec[j] = adj_lookup(Rvec[j], Ral, Ras);
						}
						else
						{
							for(j = 0; j < todo; j++)
								Rvec[j] = adj_lookup(Rvec[j], Ral, Ras);
						}
					// red histogram
					if(gui_open)
					{
						if(have_selection)
						{
							for(j = 0; j < todo; j++)
								if(selection[j] > SELECT_THRESH)
									Rhist[(int)CLAMP((Rvec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE)] += selection[j];
						}
						else
						{
							for(j = 0; j < todo; j++)
								Rhist[(int)CLAMP((Rvec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE)] += 1.f;
						}
					}

					// green adjust
					if(Gal)
						if(have_selection)
						{
							for(j = 0; j < todo; j++)
								if(selection[j] > SELECT_THRESH)
									Gvec[j] = adj_lookup(Gvec[j], Gal, Gas);
						}
						else
						{
							for(j = 0; j < todo; j++)
								Gvec[j] = adj_lookup(Gvec[j], Gal, Gas);
						}
					// green histogram
					if(gui_open)
					{
						if(have_selection)
						{
							for(j = 0; j < todo; j++)
								if(selection[j] > SELECT_THRESH)
									Ghist[(int)CLAMP((Gvec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE)] += selection[j];
						}
						else
						{
							for(j = 0; j < todo; j++)
								Ghist[(int)CLAMP((Gvec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE)] += 1.f;
						}
					}

					// blue adjust
					if(Bal)
						if(have_selection)
						{
							for(j = 0; j < todo; j++)
								if(selection[j] > SELECT_THRESH) Bvec[j] = adj_lookup(Bvec[j], Bal, Bas);
						}
						else
						{
							for(j = 0; j < todo; j++)
								Bvec[j] = adj_lookup(Bvec[j], Bal, Bas);
						}
					// blue histogram
					if(gui_open)
					{
						if(have_selection)
						{
							for(j = 0; j < todo; j++)
								if(selection[j] > SELECT_THRESH)
									Bhist[(int)CLAMP((Bvec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE)] += selection[j];
						}
						else
						{
							for(j = 0; j < todo; j++)
								Bhist[(int)CLAMP((Bvec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE)] += 1.f;
						}
					}

					// compute HSV values or update values from earlier selection work if they've changed
					if( (!doSEL || doRGB) && // not yet computed, or since modified
							(doHSV || gui_open) ) // needed for HSV mod and/or histograms
						for(j = 0; j < todo; j++)
							RGB_to_HSpV(Rvec[j], Gvec[j], Bvec[j], Hvec[j], Svec[j], Vvec[j]);

					if(doHSV)
					{
						// H modification
						// don't bother checking selection, H adj is lightweight
						if(Hal)
						{
							for(j = 0; j < todo; j++)
							{
								Hvec[j] += Hal;
								if(Hvec[j] < 0.f) Hvec[j] += 6.f;
								if(Hvec[j] >= 6.f) Hvec[j] -= 6.f;
							}
						}
						// H histogram
						// this is pre-shift RGB data; shift hue later to save an HSV->RGB transform
						if(gui_open)
						{
							if(have_selection)
							{
								for(j = 0; j < todo; j++)
								{
									if(selection[j] > SELECT_THRESH)
									{
										float weight = selection[j] * Svec[j];
										int bin = CLAMP(Hvec[j] * HUESCALE, 0, HISTSIZE);
										Htotal += selection[j];
										Hweight += weight;
										Hhist[bin] += weight;
										Hhr[bin >> HRGBSHIFT] += Rvec[j] * weight;
										Hhg[bin >> HRGBSHIFT] += Gvec[j] * weight;
										Hhb[bin >> HRGBSHIFT] += Bvec[j] * weight;
									}
								}
							}
							else
							{
								for(j = 0; j < todo; j++)
								{
									float weight = Svec[j];
									int bin = CLAMP(Hvec[j] * HUESCALE, 0, HISTSIZE);
									Htotal += 1.f;
									Hweight += weight;
									Hhist[bin] += weight;
									Hhr[bin >> HRGBSHIFT] += Rvec[j] * weight;
									Hhg[bin >> HRGBSHIFT] += Gvec[j] * weight;
									Hhb[bin >> HRGBSHIFT] += Bvec[j] * weight;
								}
							}
						}

						// S modification
						if(Sal)
							if(have_selection)
							{
								for(j = 0; j < todo; j++)
									if(selection[j] > SELECT_THRESH)
										Svec[j] = adj_lookup(Svec[j], Sal, Sas);
							}
							else
							{
								for(j = 0; j < todo; j++)
									Svec[j] = adj_lookup(Svec[j], Sal, Sas);
							}

						// This is unrolled a few times below...
						//   Although we're using HSV, we don't want hue/saturation
						//   changes to have a strong effect on apparent brightness.
						//   Apply a correction to V (not Y!) based on luma change.
						// Calculate new RGB values at same time 
						if(Hal || Sal)
						{
							if(have_selection)
							{
								for(j = 0; j < todo; j++)
								{
									if(selection[j] > SELECT_THRESH)
									{
										HSpV_correct_RGB(Hvec[j], Svec[j], Vvec[j], Rvec[j], Gvec[j], Bvec[j]);
										// run S histogram at the same time as we've got
										//   the RGB data
										if(gui_open)
										{
											int bin = CLAMP((Svec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE);
											Shist[bin] += selection[j];
											Shr[bin >> HRGBSHIFT] += Rvec[j] * selection[j];
											Shg[bin >> HRGBSHIFT] += Gvec[j] * selection[j];
											Shb[bin >> HRGBSHIFT] += Bvec[j] * selection[j];
										}
									}
								}
							}
							else
							{
								for(j = 0; j < todo; j++)
								{
									HSpV_correct_RGB(Hvec[j], Svec[j], Vvec[j], Rvec[j], Gvec[j], Bvec[j]);
									if(gui_open)
									{
										int bin = CLAMP((Svec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE);
										Shist[bin] += 1.f;
										Shr[bin >> HRGBSHIFT] += Rvec[j];
										Shg[bin >> HRGBSHIFT] += Gvec[j];
										Shb[bin >> HRGBSHIFT] += Bvec[j];
									}
								}
							}
						}
						else
						{
							if(gui_open)
							{
								if(have_selection)
								{
									for(j = 0; j < todo; j++)
									{
										if(selection[j] > SELECT_THRESH)
										{
											int bin = CLAMP((Svec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE);
											Shist[bin] += selection[j];
											Shr[bin >> HRGBSHIFT] += Rvec[j] * selection[j];
											Shg[bin >> HRGBSHIFT] += Gvec[j] * selection[j];
											Shb[bin >> HRGBSHIFT] += Bvec[j] * selection[j];
										}
									}
								}
								else
								{
									for(j = 0; j < todo; j++)
									{
										int bin = CLAMP((Svec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE);
										Shist[bin] += 1.f;
										Shr[bin >> HRGBSHIFT] += Rvec[j];
										Shg[bin >> HRGBSHIFT] += Gvec[j];
										Shb[bin >> HRGBSHIFT] += Bvec[j];
									}
								}
							}
						}

						// V modification
						/* This one is a bit unlike the others; Value gamma
						   behavior is conditional.  When < 1.0 (darkening the
						   picture) gamma directly operates on Value as normally
						   expected (thus scaling R G and B about zero).  When
						   gamma > 1.0 (lightening the picture) it operates on the
						   Value scale's zero point rather than the Value.  Both
						   are done to get the most natural behavior out of
						   saturation as gamma changes.  The only drawback: One
						   can no longer reverse a gamma operation by applying the
						   inverse gamma in a subsequent step. */
						if(Val)
						{
							if(have_selection)
							{
								if(Vgamma < 1.0)
								{
									// scale mode
									for(j = 0; j < todo; j++)
										if(selection[j] > SELECT_THRESH)
										{
											float scale = adj_lookup(Vvec[j], Val, Vas);
											Vvec[j] = Vvec[j] * scale + Vshift;
											Rvec[j] = Rvec[j] * scale + Vshift;
											Gvec[j] = Gvec[j] * scale + Vshift;
											Bvec[j] = Bvec[j] * scale + Vshift;
										}
								}
								else
								{
									// shift mode
									for(j = 0; j < todo; j++)
										if(selection[j] > SELECT_THRESH)
										{
											float shift = adj_lookup(Vvec[j], Val, Vas);
											Vvec[j] = Vvec[j] * Vscale + shift;
											Rvec[j] = Rvec[j] * Vscale + shift;
											Gvec[j] = Gvec[j] * Vscale + shift;
											Bvec[j] = Bvec[j] * Vscale + shift;
										}
								}
							}
							else
							{
								if(Vgamma < 1.0)
								{
									// scale mode
									for(j = 0; j < todo; j++)
									{
										float scale = adj_lookup(Vvec[j], Val, Vas);
										Vvec[j] = Vvec[j] * scale + Vshift;
										Rvec[j] = Rvec[j] * scale + Vshift;
										Gvec[j] = Gvec[j] * scale + Vshift;
										Bvec[j] = Bvec[j] * scale + Vshift;
									}
								}
								else
								{
									// shift mode
									for(j = 0; j < todo; j++)
									{
										float shift = adj_lookup(Vvec[j], Val, Vas);
										Vvec[j] = Vvec[j] * Vscale + shift;
										Rvec[j] = Rvec[j] * Vscale + shift;
										Gvec[j] = Gvec[j] * Vscale + shift;
										Bvec[j] = Bvec[j] * Vscale + shift;
									}
								}
							}
						}

						// V histogram
						if(gui_open)
						{
							if(have_selection)
							{
								for(j = 0; j < todo; j++)
								{
									if(selection[j]>SELECT_THRESH)
									{
										int bin = CLAMP((Vvec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE);
										Vhist[bin] += selection[j];
										Vhr[bin >> HRGBSHIFT] += Rvec[j] * selection[j];
										Vhg[bin >> HRGBSHIFT] += Gvec[j] * selection[j];
										Vhb[bin >> HRGBSHIFT] += Bvec[j] * selection[j];
									}
								}
							}
							else
							{
								for(j = 0; j < todo; j++)
								{
									int bin = CLAMP((Vvec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE);
									Vhist[bin] += 1.f;
									Vhr[bin >> HRGBSHIFT] += Rvec[j];
									Vhg[bin >> HRGBSHIFT] += Gvec[j];
									Vhb[bin >> HRGBSHIFT] += Bvec[j];
								}
							}
						}
					}
					else if (gui_open)
					{
						if(have_selection)
						{
							// H histogram
							for(j = 0; j < todo; j++)
							{
								if(selection[j] > SELECT_THRESH)
								{
									float weight = selection[j] * Svec[j];
									int bin = CLAMP(Hvec[j] * HUESCALE, 0, HISTSIZE);
									Htotal += selection[j];
									Hweight += weight;
									Hhist[bin] += weight;
									Hhr[bin >> HRGBSHIFT] += Rvec[j] * weight;
									Hhg[bin >> HRGBSHIFT] += Gvec[j] * weight;
									Hhb[bin >> HRGBSHIFT] += Bvec[j] * weight;
								}
							}
							// S histogram
							for(j = 0; j < todo; j++)
							{
								if(selection[j] > SELECT_THRESH)
								{
									int bin = CLAMP((Svec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE);
									Shist[bin] += selection[j];
									Shr[bin >> HRGBSHIFT] += Rvec[j] * selection[j];
									Shg[bin >> HRGBSHIFT] += Gvec[j] * selection[j];
									Shb[bin >> HRGBSHIFT] += Bvec[j] * selection[j];
								}
							}
							// V histogram
							for(j = 0; j < todo; j++)
							{
								if(selection[j] > SELECT_THRESH)
								{
									int bin = CLAMP((Vvec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE);
									Vhist[bin] += selection[j];
									Vhr[bin >> HRGBSHIFT] += Rvec[j] * selection[j];
									Vhg[bin >> HRGBSHIFT] += Gvec[j] * selection[j];
									Vhb[bin >> HRGBSHIFT] += Bvec[j] * selection[j];
								}
							}
						}
						else
						{

							// H histogram
							for(j = 0; j < todo; j++)
							{
								float weight = Svec[j];
								int bin = CLAMP(Hvec[j] * HUESCALE, 0, HISTSIZE);
								Htotal += 1.f;
								Hweight += weight;
								Hhist[bin] += weight;
								Hhr[bin >> HRGBSHIFT] += Rvec[j] * weight;
								Hhg[bin >> HRGBSHIFT] += Gvec[j] * weight;
								Hhb[bin >> HRGBSHIFT] += Bvec[j] * weight;
							}
							// S histogram
							for(j = 0; j < todo; j++)
							{
								int bin = CLAMP((Svec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE);
								Shist[bin] += 1.f;
								Shr[bin >> HRGBSHIFT] += Rvec[j];
								Shg[bin >> HRGBSHIFT] += Gvec[j];
								Shb[bin >> HRGBSHIFT] += Bvec[j];
							}
							// V histogram
							for(j = 0; j < todo; j++)
							{
								int bin = CLAMP((Vvec[j] + HISTOFF) * HISTSCALE, 0, HISTSIZE);
								Vhist[bin] += 1.f;
								Vhr[bin >> HRGBSHIFT] += Rvec[j];
								Vhg[bin >> HRGBSHIFT] += Gvec[j];
								Vhb[bin >> HRGBSHIFT] += Bvec[j];
							}
						}
					}

					// layer back into pipeline color format; master fader applies here
					switch(frame->get_color_model())
					{
					case BC_RGB888:
						RGB_to_rgb8(Rvec, Gvec, Bvec, have_selection ? selection : 0, Aal, row_fragment, todo, 3);
						break;
					case BC_RGBA8888:
						RGB_to_rgb8(Rvec, Gvec, Bvec, have_selection ? selection : 0, Aal, row_fragment, todo, 4);
						break;
					case BC_RGBA16161616:
						RGB_to_rgb16(Rvec, Gvec, Bvec, have_selection ? selection : 0, Aal, (uint16_t*)row_fragment, todo, 4);
						break;
					case BC_RGB_FLOAT:
						RGB_to_rgbF(Rvec, Gvec, Bvec, have_selection ? selection : 0, Aal, (float *)row_fragment, todo, 3);
						break;
					case BC_RGBA_FLOAT:
						RGB_to_rgbF(Rvec, Gvec, Bvec, have_selection ? selection : 0, Aal, (float *)row_fragment, todo, 4);
						break;
					case BC_YUV888:
						RGB_to_yuv8(Rvec, Gvec, Bvec, have_selection ? selection : 0, Aal, row_fragment, todo, 3);
						break;
					case BC_YUVA8888:
						RGB_to_yuv8(Rvec, Gvec, Bvec, have_selection ? selection : 0, Aal, row_fragment, todo, 4);
						break;
					case BC_AYUV16161616:
						RGB_to_ayuv16(Rvec, Gvec, Bvec, have_selection ? selection : 0, Aal, (uint16_t*)row_fragment, todo);
						break;
					}
				}

				// ants
				if(show_ants)
				{
					for(j = 0; j < todo; j++)
					{
						float A = have_selection ? selection[j] : 1.f;
						if(A > .99999f)
						{
							if((ant + rowi - j) & 0x4)
							{
								// magenta
								Rvec[j] = 1;
								Gvec[j] = 0;
								Bvec[j] = 1;
							}
							else
							{
								// green
								Rvec[j] = 0;
								Gvec[j] = 1;
								Bvec[j] = 0;
							}
						}
						else
						{
							if((ant + rowi + j) & 0x4)
							{
								// black
								Rvec[j] = 0;
								Gvec[j] = 0;
								Bvec[j] = 0;
							}
							else
							{
								// white
								Rvec[j] = 1;
								Gvec[j] = 1;
								Bvec[j] = 1;
							}
						}
					}

					// re-layer back into pipeline color format
					switch(frame->get_color_model())
					{
					case BC_RGB888:
						RGB_to_rgb8(Rvec, Gvec, Bvec, have_selection ? selection : 0, 1.f, row_fragment, todo, 3);
						break;
					case BC_RGBA8888:
						RGB_to_rgb8(Rvec, Gvec, Bvec, have_selection ? selection : 0, 1.f, row_fragment, todo, 4);
						break;
					case BC_RGBA16161616:
						RGB_to_rgb16(Rvec, Gvec, Bvec, have_selection ? selection : 0, 1.f, (uint16_t*)row_fragment, todo, 4);
						break;
					case BC_RGB_FLOAT:
						RGB_to_rgbF(Rvec, Gvec, Bvec, have_selection ? selection : 0, 1.f, (float *)row_fragment, todo, 3);
						break;
					case BC_RGBA_FLOAT:
						RGB_to_rgbF(Rvec, Gvec, Bvec, have_selection ? selection : 0, 1.f, (float *)row_fragment, todo, 4);
						break;
					case BC_YUV888:
						RGB_to_yuv8(Rvec, Gvec, Bvec, have_selection ? selection : 0, 1.f, row_fragment, todo, 3);
						break;
					case BC_YUVA8888:
						RGB_to_yuv8(Rvec, Gvec, Bvec, have_selection ? selection : 0, 1.f, row_fragment, todo, 4);
						break;
					case BC_AYUV16161616:
						RGB_to_ayuv16(Rvec, Gvec, Bvec, have_selection ? selection : 0, 1.f, (uint16_t*)row_fragment, todo);
						break;
					}
				}

				if(active || show_ants || (use_mask && capture_mask))
				{

					if(use_mask && capture_mask)
					{
						switch(frame->get_color_model())
						{
						case BC_RGBA8888:
							unmask_rgba8(row_fragment, todo);
							break;
						case BC_RGBA16161616:
							unmask_rgba16((uint16_t*)row_fragment, todo);
							break;
						case BC_RGBA_FLOAT:
							unmask_rgbaF((float *)row_fragment, todo);
							break;
						case BC_YUVA8888:
							unmask_yuva8(row_fragment, todo);
							break;
						case BC_AYUV16161616:
							unmask_ayuv16((uint16_t*)row_fragment, todo);
							break;
						}
					}

					// lock to prevent interleaved cache coherency collisions
					//   across cores. Two memcpys touching the same cache line
					//   will cause the wings to fall off the processor.

					pthread_mutex_lock(&engine->copylock);
					memcpy(row, row_fragment, byte_advance);
					pthread_mutex_unlock(&engine->copylock);
					row += byte_advance;

				}
			}
		}
	}

	pthread_mutex_lock(&engine->copylock);

	plugin->hue_total += Htotal;
	plugin->hue_weight += Hweight;

	for(int i = 0; i < HISTSIZE; i++)
	{
		plugin->red_histogram[i] += Rhist[i];
		plugin->green_histogram[i] += Ghist[i];
		plugin->blue_histogram[i] += Bhist[i];

		plugin->hue_histogram[i] += Hhist[i];
		plugin->sat_histogram[i] += Shist[i];

		plugin->value_histogram[i] += Vhist[i];
	}

	for(int i = 0; i < (HISTSIZE >> HRGBSHIFT); i++)
	{
		plugin->hue_histogram_red[i] += Hhr[i];
		plugin->hue_histogram_green[i] += Hhg[i];
		plugin->hue_histogram_blue[i] += Hhb[i];

		plugin->sat_histogram_red[i] += Shr[i];
		plugin->sat_histogram_green[i] += Shg[i];
		plugin->sat_histogram_blue[i] += Shb[i];

		plugin->value_histogram_red[i] += Vhr[i];
		plugin->value_histogram_green[i] += Vhg[i];
		plugin->value_histogram_blue[i] += Vhb[i];
	}
	pthread_mutex_unlock(&engine->copylock);
}

BluebananaEngine::BluebananaEngine(BluebananaMain *plugin, int total_clients,
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
	selection_workA = 0;
	selection_workB = 0;
	task_init_serial = 0;
	pthread_mutex_init(&copylock, NULL);
	pthread_mutex_init(&tasklock, NULL);
	pthread_cond_init(&taskcond, NULL);
}

BluebananaEngine::~BluebananaEngine()
{
	pthread_cond_destroy(&taskcond);
	pthread_mutex_destroy(&tasklock);
	pthread_mutex_destroy(&copylock);
	if(selection_workA) delete[] selection_workA;
	if(selection_workB) delete[] selection_workB;
}

LoadClient* BluebananaEngine::new_client()
{
	return new BluebananaUnit(this, plugin);
}

LoadPackage* BluebananaEngine::new_package()
{
	return new BluebananaPackage(this);
}

static float lt(float *data, float pos, int n)
{
	if(pos < 0) pos += n;
	if(pos >= n) pos -= n;
	int i = (int)pos;
	float del = pos - i;
	return data[i] * (1.f - del) + data[i + 1] * del;
}

static float lt_shift(float *data, float pos, int n, int over)
{
	float s0 = 0, s1 = 0;
	if(pos < 0) pos += n;
	if(pos >= n) pos -= n;
	int i = (int)pos;
	float del = pos - i;
	i *= over;
	data += i;
	for(int j = 0; j < over; j++)
		s0 += *data++;
	for(int j = 0; j < over; j++)
		s1 += *data++;

	float temp = (s0 * (1.f - del) + s1 * del);
	if(temp > 0)
		return 1. / temp;
	else
		return 0;
}

void BluebananaEngine::process_packages(VFrame *data)
{
	int w = data->get_w();
	int h = data->get_h();
	this->data = data;
	task_init_state = 0;
	task_n = 0;
	task_finish_count = 0;

	// If we're doing any spatial modification of the selection, we'll
	//   need to operate on a temporary selection array that covers the
	//   complete frame
	if(plugin->config.Fsel_active)
	{
		if(plugin->config.Fsel_lo ||
			plugin->config.Fsel_mid ||
			plugin->config.Fsel_hi ||
			plugin->config.Fsel_over)
		{
			selection_workA = new float[w * h];
			selection_workB = new float[w * h];
		}
	}

	memset(plugin->red_histogram, 0, sizeof(plugin->red_histogram));
	memset(plugin->green_histogram, 0, sizeof(plugin->green_histogram));
	memset(plugin->blue_histogram, 0, sizeof(plugin->blue_histogram));
	memset(plugin->hue_histogram, 0, sizeof(plugin->hue_histogram));
	plugin->hue_total = 0.f;
	plugin->hue_weight = 0.f;
	memset(plugin->sat_histogram, 0, sizeof(plugin->sat_histogram));
	memset(plugin->value_histogram, 0, sizeof(plugin->value_histogram));

	memset(plugin->hue_histogram_red, 0, sizeof(plugin->hue_histogram_red));
	memset(plugin->hue_histogram_green, 0, sizeof(plugin->hue_histogram_green));
	memset(plugin->hue_histogram_blue, 0, sizeof(plugin->hue_histogram_blue));

	memset(plugin->sat_histogram_red, 0, sizeof(plugin->sat_histogram_red));
	memset(plugin->sat_histogram_green, 0, sizeof(plugin->sat_histogram_green));
	memset(plugin->sat_histogram_blue, 0, sizeof(plugin->sat_histogram_blue));

	memset(plugin->value_histogram_red, 0, sizeof(plugin->value_histogram_red));
	memset(plugin->value_histogram_green, 0, sizeof(plugin->value_histogram_green));
	memset(plugin->value_histogram_blue, 0, sizeof(plugin->value_histogram_blue));

	LoadServer::process_packages();

	// The Hue histogram needs to be adjusted/shifted
	if(plugin->config.active && plugin->hue_weight)
	{
		int i, j;
		float scale = plugin->hue_total / plugin->hue_weight;
		float Hshift = plugin->config.Hsel_active ? (plugin->config.Hsel_lo + plugin->config.Hsel_hi) / 720.f - .5f : 0.f;
		float Hal = plugin->config.Hadj_active ? plugin->config.Hadj_val / 60.f : 0.f;
		float hist[HISTSIZE + (1 << HRGBSHIFT)];
		float red[(HISTSIZE >> HRGBSHIFT) + 1];
		float green[(HISTSIZE >> HRGBSHIFT) + 1];
		float blue[(HISTSIZE >> HRGBSHIFT) + 1];

		for(i = 0; i < (1 << HRGBSHIFT); i++)
		{
			plugin->hue_histogram[i] += plugin->hue_histogram[HISTSIZE + i];
			plugin->hue_histogram[HISTSIZE + i] = plugin->hue_histogram[i];
		}
		plugin->hue_histogram_red[0] += plugin->hue_histogram_red[HISTSIZE >> HRGBSHIFT];
		plugin->hue_histogram_red[HISTSIZE >> HRGBSHIFT] = plugin->hue_histogram_red[0];
		plugin->hue_histogram_green[0] += plugin->hue_histogram_green[HISTSIZE >> HRGBSHIFT];
		plugin->hue_histogram_green[HISTSIZE >> HRGBSHIFT] = plugin->hue_histogram_green[0];
		plugin->hue_histogram_blue[0] += plugin->hue_histogram_blue[HISTSIZE >> HRGBSHIFT];
		plugin->hue_histogram_blue[HISTSIZE >> HRGBSHIFT] = plugin->hue_histogram_blue[0];

		for(i = 0; i < (HISTSIZE >> HRGBSHIFT); i++)
		{
			float pos = i + Hshift * (HISTSIZE >> HRGBSHIFT);
			if(pos < 0) pos += (HISTSIZE >> HRGBSHIFT);
			if(pos >= (HISTSIZE >> HRGBSHIFT)) pos -= (HISTSIZE >> HRGBSHIFT);

			float div = lt_shift(plugin->hue_histogram, i + Hshift * (HISTSIZE >> HRGBSHIFT),
					(HISTSIZE >> HRGBSHIFT), (1 << HRGBSHIFT));
			red[i] = lt(plugin->hue_histogram_red, i + Hshift * (HISTSIZE >> HRGBSHIFT),
					(HISTSIZE >> HRGBSHIFT)) * div;
			green[i] = lt(plugin->hue_histogram_green, i + Hshift * (HISTSIZE >> HRGBSHIFT),
					(HISTSIZE >> HRGBSHIFT)) * div;
			blue[i] = lt(plugin->hue_histogram_blue, i + Hshift * (HISTSIZE >> HRGBSHIFT),
					(HISTSIZE >> HRGBSHIFT)) * div;
		}

		for(int i = 0; i < HISTSIZE; i++)
			hist[i] = lt(plugin->hue_histogram, i + Hshift * HISTSIZE, HISTSIZE) * scale;

		memcpy(hist + HISTSIZE, hist, sizeof(*hist) * (1 << HRGBSHIFT));
		memcpy(plugin->hue_histogram, hist, sizeof(hist));

		red[HISTSIZE >> HRGBSHIFT] = red[0];
		green[HISTSIZE >> HRGBSHIFT] = green[0];
		blue[HISTSIZE >> HRGBSHIFT] = blue[0];

		for(i = 0, j = 0; i <= (HISTSIZE >> HRGBSHIFT); i++)
		{
			float sum = 0.f, H, S, V;
			for(int k = 0; k < (1 << HRGBSHIFT); j++,k++)
				sum += hist[j];

			RGB_to_HSpV(red[i], green[i], blue[i], H, S, V);
			H += Hal;
			if(H < 0) H += 6;
			if(H >= 6) H -= 6;
			HSpV_to_RGB(H, S, V, red[i], green[i], blue[i]);

			plugin->hue_histogram_red[i] = red[i] * sum;
			plugin->hue_histogram_green[i] = green[i] * sum;
			plugin->hue_histogram_blue[i] =  blue[i] * sum;
		}
	}

	if(selection_workA)
	{
		delete[] selection_workA;
		selection_workA = 0;
	}
	if(selection_workB)
	{
		delete[] selection_workB;
		selection_workB = 0;
	}
}

// create a coordinated work-division list and join point
void BluebananaEngine::set_task(int n, const char *task)
{
	pthread_mutex_lock(&this->tasklock);
	if(task_init_state == 0)
	{
		task_n = n;
		task_finish_count = get_total_packages();
		task_init_state = 1;
		task_init_serial++;
	}
	pthread_mutex_unlock(&this->tasklock);
}

// fetch the next task ticket.  If the task is complete, wait here for
//   all package threads to complete before returning <0
int BluebananaEngine::next_task()
{
	pthread_mutex_lock(&tasklock);
	if(task_n)
	{
		int ret = --task_n;
		pthread_mutex_unlock(&tasklock);
		return ret;
	}
	else
	{
		task_finish_count--;
		if(task_finish_count == 0)
		{
			task_init_state = 0;
			pthread_cond_broadcast(&taskcond);
		}
		else
		{
			int serial = task_init_serial;
			while(task_finish_count && serial == task_init_serial)
			{
				pthread_cond_wait(&taskcond, &tasklock);
			}
		}
		pthread_mutex_unlock(&tasklock);
		return -1;
	}
}

// same as above, but waits for join without fetching a task slot
void BluebananaEngine::wait_task()
{
	pthread_mutex_lock(&tasklock);
	task_finish_count--;
	if(task_finish_count == 0)
	{
		task_init_state = 0;
		pthread_cond_broadcast(&taskcond);
	}
	else
	{
		int serial = task_init_serial;
		while(task_finish_count && serial == task_init_serial)
		{
			pthread_cond_wait(&taskcond, &tasklock);
		}
	}
	pthread_mutex_unlock(&tasklock);
}
