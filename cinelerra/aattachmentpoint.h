#ifndef AATTACHMENTPOINT_H
#define AATTACHMENTPOINT_H


#include "attachmentpoint.h"


class AAttachmentPoint : public AttachmentPoint
{
public:
	AAttachmentPoint(RenderEngine *renderengine, Plugin *plugin);
	~AAttachmentPoint();
	
	void delete_buffer_vectors();
	void new_buffer_vectors();
	void render(double *audio_in, 
		double *audio_out, 
		long fragment_size, 
		long current_position);
	void dispatch_plugin_server(int buffer_number, 
		long current_position, 
		long fragment_size);
	int get_buffer_size();

	double **buffer_in, **buffer_out;
};

#endif
