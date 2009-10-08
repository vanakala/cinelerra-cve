
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

#ifndef VATTACHMENTPOINT_H
#define VATTACHMENTPOINT_H


#include "attachmentpoint.h"


class VAttachmentPoint : public AttachmentPoint
{
public:
	VAttachmentPoint(RenderEngine *renderengine, Plugin *plugin);
	~VAttachmentPoint();
	
	void delete_buffer_vector();
	void new_buffer_vector(int width, int height, int colormodel);
	void render(VFrame *output, 
		int buffer_number,
		int64_t start_position,
		double frame_rate,
		int debug_render,
		int use_opengl = 0);
	void dispatch_plugin_server(int buffer_number, 
		int64_t current_position, 
		int64_t fragment_size);
	int get_buffer_size();

	VFrame **buffer_vector;
};

#endif
