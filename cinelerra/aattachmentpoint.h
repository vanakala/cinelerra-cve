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
