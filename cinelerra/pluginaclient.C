
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

#include "bcsignals.h"
#include "edl.h"
#include "edlsession.h"
#include "pluginaclient.h"
#include "pluginserver.h"
#include "renderengine.inc"
#include "track.h"
#include "trackrender.h"

#include <string.h>

PluginAClient::PluginAClient(PluginServer *server)
 : PluginClient(server)
{
}

int PluginAClient::is_audio()
{
	return 1;
}

int PluginAClient::plugin_process_loop(AFrame **aframes)
{
	if(is_multichannel())
		return process_loop(aframes);
	else
		return process_loop(aframes[0]);
}

int PluginAClient::get_project_samplerate()
{
	return edlsession->sample_rate;
}
