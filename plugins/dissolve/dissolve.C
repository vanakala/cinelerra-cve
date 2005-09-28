#include "dissolve.h"
#include "edl.inc"
#include "language.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"

PluginClient* new_plugin(PluginServer *server)
{
	return new DissolveMain(server);
}





DissolveMain::DissolveMain(PluginServer *server)
 : PluginVClient(server)
{
	overlayer = 0;
}

DissolveMain::~DissolveMain()
{
	delete overlayer;
}

char* DissolveMain::plugin_title() { return N_("Dissolve"); }
int DissolveMain::is_video() { return 1; }
int DissolveMain::is_transition() { return 1; }
int DissolveMain::uses_gui() { return 0; }

NEW_PICON_MACRO(DissolveMain)


int DissolveMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	float fade = (float)PluginClient::get_source_position() / 
			PluginClient::get_total_len();

	if(!overlayer) overlayer = new OverlayFrame(get_project_smp() + 1);


// There is a problem when dissolving from a big picture to a small picture.
// In order to make it dissolve correctly, we have to manually decrese alpha of big picture.
	switch (outgoing->get_color_model())
	{
		case BC_RGBA8888:
		case BC_YUVA8888:
		{
			uint8_t** data_rows = (uint8_t **)outgoing->get_rows();
			int w = outgoing->get_w();
			int h = outgoing->get_h(); 
			for(int i = 0; i < h; i++) 
			{ 
				uint8_t* alpha_chan = data_rows[i] + 3; 
				for(int j = 0; j < w; j++) 
				{
					*alpha_chan = (uint8_t) (*alpha_chan * (1-fade));
					alpha_chan+=4;
				} 
			}
			break;
		}
		case BC_YUVA16161616:
		{
			uint16_t** data_rows = (uint16_t **)outgoing->get_rows();
			int w = outgoing->get_w();
			int h = outgoing->get_h(); 
			for(int i = 0; i < h; i++) 
			{ 
				uint16_t* alpha_chan = data_rows[i] + 3; // 3 since this is uint16_t
				for(int j = 0; j < w; j++) 
				{
					*alpha_chan = (uint16_t)(*alpha_chan * (1-fade));
					alpha_chan += 8;
				} 
			}
			break;
		}
		case BC_RGBA_FLOAT:
		{
			float** data_rows = (float **)outgoing->get_rows();
			int w = outgoing->get_w();
			int h = outgoing->get_h(); 
			for(int i = 0; i < h; i++) 
			{ 
				float* alpha_chan = data_rows[i] + 3; // 3 since this is floats 
				for(int j = 0; j < w; j++) 
				{
					*alpha_chan = *alpha_chan * (1-fade);
					alpha_chan += sizeof(float);
				} 
			}
			break;
		}
		default:
			break;
	}


	overlayer->overlay(outgoing, 
		incoming, 
		0, 
		0, 
		incoming->get_w(),
		incoming->get_h(),
		0,
		0,
		incoming->get_w(),
		incoming->get_h(),
		fade,
		TRANSFER_NORMAL,
		NEAREST_NEIGHBOR);

	return 0;
}
