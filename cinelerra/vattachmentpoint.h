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
		int debug_render);
	void dispatch_plugin_server(int buffer_number, 
		int64_t current_position, 
		int64_t fragment_size);
	int get_buffer_size();

	VFrame **buffer_vector;
};

#endif
