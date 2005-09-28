#ifndef VIRTUALVNODE_H
#define VIRTUALVNODE_H

#include "fadeengine.inc"
#include "maskengine.inc"
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
		VirtualNode *parent_module);

	~VirtualVNode();

// expansions
	VirtualNode* create_module(Plugin *real_plugin, 
							Module *real_module, 
							Track *track);
	VirtualNode* create_plugin(Plugin *real_plugin);
	void arm_attachmentpoint();

// Called by VirtualVConsole::process_buffer to process exit nodes.
// start_position - end of frame if reverse.  start of frame if forward.
// frame_rate - rate start_position is relative to
	int render(VFrame *output_temp, 
		int64_t start_position,
		double frame_rate);

// Read data from what comes before this node.
	int read_data(VFrame *output_temp,
		int64_t start_position,
		double frame_rate);

private:
	int render_as_module(VFrame **video_out, 
		VFrame *output_temp,
		int64_t start_position,
		double frame_rate);
	void render_as_plugin(VFrame *output_temp, 
		int64_t start_position,
		double frame_rate);

	int render_projector(VFrame *input,
			VFrame **output,
			int64_t start_position,
			double frame_rate);  // Start of input fragment in project if forward.  End of input fragment if reverse.

	int render_fade(VFrame *output,        // start of output fragment
			int64_t start_position,  // start of input fragment in project if forward / end of input fragment if reverse
			double frame_rate, 
			Autos *autos,
			int direction);


	FadeEngine *fader;
	MaskEngine *masker;
};


#endif
