
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

#ifndef AATTACHMENTPOINT_H
#define AATTACHMENTPOINT_H


#include "attachmentpoint.h"

class AAttachmentPoint : public AttachmentPoint
{
public:
	AAttachmentPoint(RenderEngine *renderengine, Plugin *plugin);
	~AAttachmentPoint();
	
	void delete_buffer_vector();
	void new_buffer_vector(int total, int size);
	void render(double *output, 
		int buffer_number,
		int64_t start_position, 
		int64_t len,
		int64_t sample_rate);
	void dispatch_plugin_server(int buffer_number, 
		long current_position, 
		long fragment_size);
	int get_buffer_size();

// Storage for multichannel plugins
	double **buffer_vector;
	int buffer_allocation;
};

#endif
