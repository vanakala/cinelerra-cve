
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
#include "picon_png.h"
#include "vframe.h"


REGISTER_PLUGIN


CrossfadeMain::CrossfadeMain(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

CrossfadeMain::~CrossfadeMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void CrossfadeMain::process_realtime(AFrame *out, AFrame *in)
{
	double intercept = source_pts / total_len_pts;
	double slope = (double)1 / round(total_len_pts * out->samplerate);
	double *incoming = in->buffer;
	double *outgoing = out->buffer;
	int size = out->length;

	for(int i = 0; i < size; i++)
	{
		incoming[i] = outgoing[i] * ((double)1 - (slope * i + intercept)) + 
			incoming[i] * (slope * i + intercept);
	}
}
