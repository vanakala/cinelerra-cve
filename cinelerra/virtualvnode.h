#ifndef VIRTUALVNODE_H
#define VIRTUALVNODE_H

#include "bezierauto.inc"
#include "fadeengine.inc"
#include "maskengine.inc"
#include "overlayframe.inc"
#include "plugin.inc"
#include "renderengine.inc"
#include "vframe.inc"
#include "virtualnode.h"
#include "vrender.inc"

class VirtualVNode : public VirtualNode
{
public:
// construct as a module or a plugin
	VirtualVNode(RenderEngine *renderengine, 
		VirtualConsole *vconsole, 
		Module *real_module, 
		Plugin *real_plugin,
		Track *track, 
		VirtualNode *parent_module, 
		VFrame *input,
		VFrame *output,
		int input_is_master,
		int output_is_master,
		int in,
		int out);

	~VirtualVNode();

// expansions
	void new_output_buffer();
	void new_input_buffer();
	VirtualNode* create_module(Plugin *real_plugin, 
							Module *real_module, 
							Track *track);
	VirtualNode* create_plugin(Plugin *real_plugin);
	void arm_attachmentpoint();

	int render(VFrame **video_out, int64_t input_position);


// Pointers to data, whether drive read buffers or temp buffers
	VFrame *buffer_in;
	VFrame *buffer_out;  


private:
	int render_as_module(VFrame **video_out, int64_t input_position);
	void render_as_plugin(int64_t input_position);

	int render_projector(VFrame *input,
			VFrame **output,
			int64_t real_position);  // Start of input fragment in project if forward.  End of input fragment if reverse.

	int render_fade(VFrame *input,         // start of input fragment
			VFrame *output,        // start of output fragment
			int64_t input_position,  // start of input fragment in project if forward / end of input fragment if reverse
			Autos *autos);

// overlay on the frame with scaling
// Alpha values are from 0 to VMAX
	int transfer_from(VFrame *frame_out, 
		VFrame *frame_in, 
		float in_x1, 
		float in_y1, 
		float in_x2, 
		float in_y2, 
		float out_x1, 
		float out_y1, 
		float out_x2, 
		float out_y2, 
		float alpha, 
		int mode);

	VFrame* get_module_input();
	VFrame* get_module_output();
	OverlayFrame *overlayer;
	FadeEngine *fader;
	MaskEngine *masker;
};


#endif
