#include "flash.h"
#include "edl.inc"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"
#include <stdint.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

PluginClient* new_plugin(PluginServer *server)
{
	return new FlashMain(server);
}





FlashMain::FlashMain(PluginServer *server)
 : PluginVClient(server)
{
}

FlashMain::~FlashMain()
{
}

char* FlashMain::plugin_title() { return _("Flash"); }
int FlashMain::is_video() { return 1; }
int FlashMain::is_transition() { return 1; }
int FlashMain::uses_gui() { return 0; }

NEW_PICON_MACRO(FlashMain)


#define FLASH(type, components, max, chroma_offset) \
{ \
	int foreground = (int)(fraction * max + 0.5); \
	int chroma_foreground = foreground; \
	if(chroma_offset) chroma_foreground = foreground * chroma_offset / max; \
	int transparency = max - foreground; \
/* printf("FLASH %d %d %d\n", foreground, transparency, is_before); */\
	for(int i = 0; i < h; i++) \
	{ \
		type *in_row = (type*)incoming->get_rows()[i]; \
		type *out_row = (type*)outgoing->get_rows()[i]; \
		if(is_before) \
		{ \
			for(int j = 0; j < w; j++) \
			{ \
				*out_row = foreground + transparency * *out_row / max; \
				out_row++; \
				*out_row = chroma_foreground + transparency * *out_row / max; \
				out_row++; \
				*out_row = chroma_foreground + transparency * *out_row / max; \
				out_row++; \
				if(components == 4) \
				{ \
					*out_row = foreground + transparency * *out_row / max; \
					out_row++; \
				} \
			} \
		} \
		else \
		{ \
			for(int j = 0; j < w; j++) \
			{ \
				*out_row = foreground + transparency * *in_row / max; \
				out_row++; \
				in_row++; \
				*out_row = chroma_foreground + transparency * *in_row / max; \
				out_row++; \
				in_row++; \
				*out_row = chroma_foreground + transparency * *in_row / max; \
				out_row++; \
				in_row++; \
				if(components == 4) \
				{ \
					*out_row = foreground + transparency * *in_row / max; \
					out_row++; \
					in_row++; \
				} \
			} \
		} \
	} \
}

int FlashMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	int half = PluginClient::get_total_len() / 2;
	int position = half - labs(PluginClient::get_source_position() - half);
	float fraction = (float)position / half;
	int is_before = PluginClient::get_source_position() < half;
	int w = incoming->get_w();
	int h = incoming->get_h();

	switch(incoming->get_color_model())
	{
		case BC_RGB888:
			FLASH(unsigned char, 3, 0xff, 0x0);
			break;
		case BC_RGBA8888:
			FLASH(unsigned char, 4, 0xff, 0x0);
			break;
		case BC_YUV888:
			FLASH(unsigned char, 3, 0xff, 0x80);
			break;
		case BC_YUVA8888:
			FLASH(unsigned char, 4, 0xff, 0x80);
			break;
		case BC_RGB161616:
			FLASH(uint16_t, 3, 0xffff, 0x0);
			break;
		case BC_RGBA16161616:
			FLASH(uint16_t, 4, 0xffff, 0x0);
			break;
		case BC_YUV161616:
			FLASH(uint16_t, 3, 0xffff, 0x8000);
			break;
		case BC_YUVA16161616:
			FLASH(uint16_t, 4, 0xffff, 0x8000);
			break;
	}

	return 0;
}
