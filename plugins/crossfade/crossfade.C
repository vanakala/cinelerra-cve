
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

#include "crossfade.h"
#include "edl.inc"
#include "language.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"




REGISTER_PLUGIN(CrossfadeMain)




CrossfadeMain::CrossfadeMain(PluginServer *server)
 : PluginAClient(server)
{
}

CrossfadeMain::~CrossfadeMain()
{
}

const char* CrossfadeMain::plugin_title() { return N_("Crossfade"); }
int CrossfadeMain::is_transition() { return 1; }
int CrossfadeMain::uses_gui() { return 0; }

NEW_PICON_MACRO(CrossfadeMain)


int CrossfadeMain::process_realtime(int64_t size, 
	double *outgoing, 
	double *incoming)
{
	double intercept = (double)PluginClient::get_source_position() / 
		PluginClient::get_total_len();
	double slope = (double)1 / PluginClient::get_total_len();

//printf("CrossfadeMain::process_realtime %f %f\n", intercept, slope);
	for(int i = 0; i < size; i++)
	{
		incoming[i] = outgoing[i] * ((double)1 - (slope * i + intercept)) + 
			incoming[i] * (slope * i + intercept);
	}

	return 0;
}
