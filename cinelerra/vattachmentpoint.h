#ifndef VATTACHMENTPOINT_H
#define VATTACHMENTPOINT_H


#include "attachmentpoint.h"


class VAttachmentPoint : public AttachmentPoint
{
public:
	VAttachmentPoint(RenderEngine *renderengine, Plugin *plugin);
	~VAttachmentPoint();
	
	void delete_buffer_vectors();
	void new_buffer_vectors();
	void render(VFrame *video_in, VFrame *video_out, long current_position);
	void dispatch_plugin_server(int buffer_number, 
		long current_position, 
		long fragment_size);
	int get_buffer_size();

	VFrame **buffer_in, **buffer_out;
};

#endif
