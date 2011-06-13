
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

#include "language.h"
#include "picon_png.h"
#include "pluginaclient.h"
#include "vframe.h"


class InvertAudioEffect : public PluginAClient
{
public:
	InvertAudioEffect(PluginServer *server)
	 : PluginAClient(server) {};
	~InvertAudioEffect(){};

	PLUGIN_CLASS_MEMBERS_TRANSITION

	int is_realtime(){ return 1; };
	int has_pts_api(){ return 1; };
	int uses_gui(){ return 0; };

	void process_frame_realtime(AFrame *input_ptr, AFrame *output);
};


REGISTER_PLUGIN(InvertAudioEffect)

NEW_PICON_MACRO(InvertAudioEffect)

const char* InvertAudioEffect::plugin_title()
{
	return  N_("Invert Audio");
};

void InvertAudioEffect::process_frame_realtime(AFrame *input, AFrame *output)
{
	int size = input->length;

	if(input != output)
		output->copy_of(input);

	for(int i = 0; i < size; i++)
		output->buffer[i] = -input->buffer[i];
};
