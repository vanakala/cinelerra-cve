#include "clip.h"
#include "colormodels.h"
#include "defaults.h"
#include "filexml.h"
#include "diffkey.h"
#include "diffkeywindow.h"
#include "picon_png.h"
#include "language.h"

#include <stdint.h>
#include <string.h>
#include "plugincolors.h"

REGISTER_PLUGIN(DiffKeyMain)

//#define NO_PROCESS
//#define NO_PARAM_CODE
//#define NO_GUI_CODE

#ifdef NO_GUI_CODE
	#define NO_SAVE_DEFAULTS
	#define NO_LOAD_DEFAULTS
	#define NO_SAVE_DATA
	#define NO_LOAD_DATA
#endif

#ifdef NO_PARAM_CODE
	#define NO_PARAM_COPY
	#define NO_PARAM_COMPARE
	#define NO_PARAM_INTERPOLATE
#endif

int grab_key_frame = 0;
int add_key_frame = 0;

DiffKeyConfig::DiffKeyConfig()
{
	#ifdef DEBUG
	printf("\nDiffKeyConfig::DiffKeyConfig");
	#endif

	hue_imp = 25;
	sat_imp = 25;
	val_imp = 25;
	r_imp = 25;
	g_imp = 25;
	b_imp = 25;
	vis_thresh = 25;
	trans_thresh = 25;
	desat_thresh = 25;
	hue_on = 0;
	sat_on = 0;
	val_on = 0;
	r_on = 0;
	g_on = 0;
	b_on = 1;
	vis_on = 0;
	trans_on = 0;
	desat_on = 0;
	show_mask = 0;

}

void DiffKeyConfig::copy_from(DiffKeyConfig &that)
{
#ifndef NO_PARAM_COPY
	#ifdef DEBUG
	printf("\nDiffKeyConfig::copy_from");
	#endif

	hue_imp = that.hue_imp;
	sat_imp = that.sat_imp;
	val_imp = that.val_imp;
	r_imp = that.r_imp;
	g_imp = that.g_imp;
	b_imp = that.b_imp;
	vis_thresh = that.vis_thresh;
	trans_thresh = that.trans_thresh;
	desat_thresh = that.desat_thresh;
	hue_on = that.hue_on;
	sat_on = that.sat_on;
	val_on = that.val_on;
	r_on = that.r_on;
	g_on = that.g_on;
	b_on = that.b_on;
	vis_on = that.vis_on;
	trans_on = that.trans_on;
	desat_on = that.desat_on;
	show_mask = that.show_mask;
#endif
}

int DiffKeyConfig::equivalent(DiffKeyConfig &that)
{
#ifndef NO_PARAM_COMPARE
	#ifdef DEBUG
	printf("\nDiffKeyConfig::equivalent");
	#endif
	return hue_imp == that.hue_imp &&
		sat_imp == that.sat_imp &&
		val_imp == that.val_imp &&
		r_imp == that.r_imp &&
		g_imp == that.g_imp &&
		b_imp == that.b_imp &&
		vis_thresh == that.vis_thresh &&
		trans_thresh == that.trans_thresh &&
		desat_thresh == that.desat_thresh &&
		hue_on == that.hue_on &&
		sat_on == that.sat_on &&
		val_on == that.val_on &&
		r_on == that.r_on &&
		g_on == that.g_on &&
		b_on == that.b_on &&
		vis_on == that.vis_on &&
		trans_on == that.trans_on &&
		desat_on == that.desat_on &&
		show_mask == that.show_mask;
#endif
}

void DiffKeyConfig::interpolate(DiffKeyConfig &prev, 
	DiffKeyConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	#ifndef NO_PARAM_INTERPOLATE
	#ifdef DEBUG
	printf("\nDiffKeyConfig::interpolate");
	#endif
	
	ftype next_scale = (ftype)(current_frame - prev_frame) / (next_frame - prev_frame);
	ftype prev_scale = (ftype)(next_frame - current_frame) / (next_frame - prev_frame);

	this->hue_imp = (prev.hue_imp * prev_scale + next.hue_imp * next_scale);
	this->sat_imp = (prev.sat_imp * prev_scale + next.sat_imp * next_scale);
	this->val_imp = (prev.val_imp * prev_scale + next.val_imp * next_scale);
	this->r_imp = (prev.r_imp * prev_scale + next.r_imp * next_scale);
	this->g_imp = (prev.g_imp * prev_scale + next.g_imp * next_scale);
	this->b_imp = (prev.b_imp * prev_scale + next.b_imp * next_scale);
	this->vis_thresh = (prev.vis_thresh * prev_scale + next.vis_thresh * next_scale);
	this->trans_thresh = (prev.trans_thresh * prev_scale + next.trans_thresh * next_scale);
	this->desat_thresh = (prev.desat_thresh * prev_scale + next.desat_thresh * next_scale);
	this->hue_on = prev.hue_on;
	this->sat_on = prev.sat_on;
	this->val_on = prev.val_on;
	this->r_on = prev.r_on;
	this->g_on = prev.g_on;
	this->b_on = prev.b_on;
	this->vis_on = prev.vis_on;
	this->trans_on = prev.trans_on;
	this->desat_on = prev.desat_on;
	this->show_mask = prev.show_mask;
#endif
}














DiffKeyMain::DiffKeyMain(PluginServer *server)
 : PluginVClient(server)
{
#ifdef DEBUG
	printf("\nConstructing DiffKeyMain....");
	printf("\n");
#endif
	PLUGIN_CONSTRUCTOR_MACRO
	key_frame = 0;
}

DiffKeyMain::~DiffKeyMain()
{
#ifdef DEBUG
	printf("\nDestroying DiffKeyMain....");
	printf("\n");
#endif
	if (key_frame)
		delete key_frame;
	PLUGIN_DESTRUCTOR_MACRO
}

char* DiffKeyMain::plugin_title() { return N_("DiffKey"); }
int DiffKeyMain::is_realtime() { return 1; }
	




#define DIFF_KEY_MACRO(type, maximum, use_yuv) \
{ \
	type **input_rows, **output_rows, **key_rows; \
	type *input_row, *output_row, *key_row; \
	type *oc1, *oc2, *oc3, *oc4; \
	max = (ftype)maximum; \
	input_rows = ((type**)input_ptr->get_rows()); \
	output_rows = ((type**)output_ptr->get_rows()); \
	key_rows = ((type**)key_frame->get_rows()); \
	 \
	 \
	 \
	for(i = 0; i < h; i++) \
	{ \
		input_row = input_rows[i]; \
		output_row = output_rows[i]; \
		key_row = key_rows[i]; \
		for(k = 0; k < w; k++) \
		{ \
			in_r = (ftype)*input_row / max; \
			in_g = (ftype)*(input_row + 1) / max; \
			in_b = (ftype)*(input_row + 2) / max; \
			 \
			key_r = (ftype)*key_row / max; \
			key_g = (ftype)*(key_row + 1) / max; \
			key_b = (ftype)*(key_row + 2) / max; \
			 \
			if(use_yuv) \
			{ \
				HSV::yuv_to_hsv((int)input_row,  \
					(int)(input_row + 1),  \
					(int)(input_row + 2),  \
					in_hue,  \
					in_sat,  \
					in_val,  \
					(int)maximum); \
				HSV::yuv_to_hsv((int)key_row,  \
					(int)(key_row + 1),  \
					(int)(key_row + 2),  \
					key_hue,  \
					key_sat,  \
					key_val,  \
					(int)maximum); \
			} \
			else \
			{ \
				HSV::rgb_to_hsv(in_r,  \
					in_g,  \
					in_b,  \
					in_hue, in_sat, in_val); \
				HSV::rgb_to_hsv(key_r,  \
					key_g,  \
					key_b,  \
					key_hue, key_sat, key_val); \
			} \
			 \
			difference = alpha = 0; \
			if (config.hue_on) DO_DIFFERENCE (in_hue, key_hue, hue_imp);\
			if (config.sat_on) DO_DIFFERENCE (in_sat, key_sat, sat_imp);\
			if (config.val_on) DO_DIFFERENCE (in_val, key_val, val_imp);\
			if (config.r_on) DO_DIFFERENCE (in_r, key_r, r_imp);\
			if (config.g_on) DO_DIFFERENCE (in_g, key_g, g_imp);\
			if (config.b_on) DO_DIFFERENCE (in_b, key_b, b_imp);\
			 \
			if (config.vis_on) alpha *= vis_thresh; \
			if (alpha > 1) \
				alpha = 1; \
			 \
			alpha = 1 - alpha; \
			 \
			if (config.trans_on) alpha *= trans_thresh; \
			if (alpha > 1) \
				alpha = 1; \
			 \
			 \
			 \
			 \
			if (config.show_mask) \
			{ \
			alpha = 1 - alpha; \
				*output_row = \
				*(output_row + 1) = \
				*(output_row + 2) = \
				(type)(alpha * max); \
				*(output_row + 3) = maximum; \
			} \
			 \
			else \
			{ \
			 \
			if (config.desat_on) { \
				DO_DESATURATE (in_r, key_r, out_r);\
				DO_DESATURATE (in_g, key_g, out_g);\
				DO_DESATURATE (in_b, key_b, out_b);\
				*output_row = (type)(out_r * max); \
				*(output_row + 1) = (type)(out_g * max); \
				*(output_row + 2) = (type)(out_b * max); \
			} \
			else { \
				*output_row = *input_row; \
				*(output_row + 1) = *(input_row + 1); \
				*(output_row + 2) = *(input_row + 2); \
			} \
			alpha = 1 - alpha; \
				*(output_row + 3) = (type)(alpha * max); \
				 \
				 \
			} \
			 \
			output_row += 4; \
			input_row += 4; \
			key_row += 4; \
			 \
		} \
	} \
}



#define DO_DIFFERENCE(input, key, scale) \
{ \
	difference = input - key; \
	if (difference < 0) \
		difference *= -1; \
	alpha += difference * scale; \
}


#define DO_DESATURATE(input, key, output) \
{ \
	difference = key * alpha * desat_thresh; \
	output = input - difference; \
	if (output < 0) \
		output = 0; \
}



int DiffKeyMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
#ifndef NO_PROCESS
	load_configuration();
	ftype in_hue, in_sat, in_val, in_r, in_g, in_b;
	ftype out_hue, out_sat, out_val, out_r, out_g, out_b;
	ftype key_hue, key_sat, key_val, key_r, key_g, key_b;
	ftype difference;
	ftype alpha;
	ftype max;
	ftype hue_imp = ((ftype)config.hue_imp / 3200);
	ftype sat_imp = ((ftype)config.sat_imp / 25);
	ftype val_imp = ((ftype)config.val_imp / 25);
	ftype r_imp = ((ftype)config.r_imp / 25);
	ftype g_imp = ((ftype)config.g_imp / 25);
	ftype b_imp = ((ftype)config.b_imp / 25);
	ftype vis_thresh = ((ftype)config.vis_thresh / 50); // was / 50 + 1
	ftype trans_thresh = ((ftype)config.trans_thresh / 50 + 1);
	ftype desat_thresh = ((ftype)config.desat_thresh / 100);
	ftype trans_up = 1 - trans_thresh;

	int i, j, k, l;
	int w = input_ptr->get_w();
	int h = input_ptr->get_h();
	int colormodel = input_ptr->get_color_model();









	
#ifdef DEBUG
	printf("\nAbout to process....");
	printf("\nConfig values:");
	printf("\nhue_imp: %f   sat_imp: %f   val_imp: %f", config.hue_imp, config.sat_imp, config.val_imp);
	printf("\nr_imp: %f   g_imp: %f   b_imp: %f", config.r_imp, config.g_imp, config.b_imp);
	printf("\nvis_thresh: %f   trans_thresh: %f   show_mask: %i", config.vis_thresh, config.trans_thresh, config.show_mask);
	printf("\nProcess values:");
	printf("\nhue_imp: %f   sat_imp: %f   val_imp: %f", hue_imp, sat_imp, val_imp);
	printf("\nr_imp: %f   g_imp: %f   b_imp: %f", r_imp, g_imp, b_imp);
	printf("\nvis_thresh: %f   trans_thresh: %f   trans_up: %f", vis_thresh, trans_thresh, trans_up);
	printf("\n");
	printf("\n");
	printf("\n");
#endif

	if (grab_key_frame || !key_frame)
	{
		#ifdef DEBUG
			printf("\nTrying to grab keyframe...");
			printf("\n");
		#endif
		grab_key_frame = 0;
		if (key_frame)
			delete key_frame;
		key_frame = new VFrame(0, 
			input_ptr->get_w(),
			input_ptr->get_h(),
			PluginVClient::project_color_model);
		key_frame->copy_from(input_ptr);
		#ifdef DEBUG
			printf("\nKey frame grabbed.");
			printf("\n");
		#endif
	}

	switch(colormodel)
	{
		case BC_RGBA8888:
			DIFF_KEY_MACRO(unsigned char, 255, 0);
			break;
		case BC_YUVA8888:
			DIFF_KEY_MACRO(unsigned char, 255, 1);
			break;
		case BC_RGBA16161616:
			DIFF_KEY_MACRO(uint16_t, 65535, 0);
			break;
		case BC_YUVA16161616:
			DIFF_KEY_MACRO(uint16_t, 65535, 1);
			break;
		case BC_RGB888: // We need an alpha channel to do anything useful
		case BC_YUV888:
		case BC_RGB161616:
		case BC_YUV161616:
			output_ptr->copy_from(input_ptr);
			break;
	}

	#ifdef DEBUG
		printf("\nFrame processed.\n");
		printf("\n");
		printf("\n");
	#endif
#endif
	return 0;
}
























SHOW_GUI_MACRO(DiffKeyMain, DiffKeyThread)

RAISE_WINDOW_MACRO(DiffKeyMain)

SET_STRING_MACRO(DiffKeyMain)

NEW_PICON_MACRO(DiffKeyMain)

LOAD_CONFIGURATION_MACRO(DiffKeyMain, DiffKeyConfig)

void DiffKeyMain::update_gui()
{
#ifndef NO_GUI_CODE
	#ifdef DEBUG
	printf("\nDiffKeyMain::update_gui");
	#endif
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->hue_imp->update((int)config.hue_imp);
		thread->window->sat_imp->update((int)config.sat_imp);
		thread->window->val_imp->update((int)config.val_imp);
		thread->window->r_imp->update((int)config.r_imp);
		thread->window->g_imp->update((int)config.g_imp);
		thread->window->b_imp->update((int)config.b_imp);
		thread->window->vis_thresh->update((int)config.vis_thresh);
		thread->window->trans_thresh->update((int)config.trans_thresh);
		thread->window->desat_thresh->update((int)config.desat_thresh);
		thread->window->hue_on->update((int)config.hue_on);
		thread->window->sat_on->update((int)config.sat_on);
		thread->window->val_on->update((int)config.val_on);
		thread->window->r_on->update((int)config.r_on);
		thread->window->g_on->update((int)config.g_on);
		thread->window->b_on->update((int)config.b_on);
		thread->window->vis_on->update((int)config.vis_on);
		thread->window->trans_on->update((int)config.trans_on);
		thread->window->desat_on->update((int)config.desat_on);
		thread->window->show_mask->update((int)config.show_mask);
		thread->window->unlock_window();
	}
#endif
}

void DiffKeyMain::save_data(KeyFrame *keyframe)
{
#ifndef NO_SAVE_DATA
	#ifdef DEBUG
	printf("\nDiffKeyMain::save_data");
	#endif
	FileXML output;
// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("DIFF_KEY0.01");
	output.tag.set_property("HUE_IMPORTANCE", config.hue_imp);
	output.tag.set_property("SATURATION_IMPORTANCE", config.sat_imp);
	output.tag.set_property("VALUE_IMPORTANCE", config.val_imp);
	output.tag.set_property("RED_IMPORTANCE", config.r_imp);
	output.tag.set_property("GREEN_IMPORTANCE", config.g_imp);
	output.tag.set_property("BLUE_IMPORTANCE", config.b_imp);
	output.tag.set_property("VISIBILITY_THRESHOLD", config.vis_thresh);
	output.tag.set_property("TRANSPARENCY_THRESHOLD", config.trans_thresh);
	output.tag.set_property("DESATURATION_THRESHOLD", config.desat_thresh);

	output.append_tag();
	if(config.hue_on)
	{
		output.tag.set_title("HUE_ON");
		output.append_tag();
	}

	if(config.sat_on)
	{
		output.tag.set_title("SATURATION_ON");
		output.append_tag();
	}

	if(config.val_on)
	{
		output.tag.set_title("VALUE_ON");
		output.append_tag();
	}

	if(config.r_on)
	{
		output.tag.set_title("RED_ON");
		output.append_tag();
	}

	if(config.g_on)
	{
		output.tag.set_title("GREEN_ON");
		output.append_tag();
	}

	if(config.b_on)
	{
		output.tag.set_title("BLUE_ON");
		output.append_tag();
	}

	if(config.vis_on)
	{
		output.tag.set_title("VISIBILITY_ON");
		output.append_tag();
	}

	if(config.trans_on)
	{
		output.tag.set_title("TRANSPARENCY_ON");
		output.append_tag();
	}

	if(config.desat_on)
	{
		output.tag.set_title("DESATURATION_ON");
		output.append_tag();
	}

	if(config.show_mask)
	{
		output.tag.set_title("SHOW_MASK");
		output.append_tag();
	}

	output.terminate_string();
// data is now in *text
#endif
}

void DiffKeyMain::read_data(KeyFrame *keyframe)
{
#ifndef NO_READ_DATA
	#ifdef DEBUG
	printf("\nDiffKeyMain::read_data");
	#endif
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	config.show_mask = 
	config.hue_on = 
	config.sat_on = 
	config.val_on = 
	config.r_on = 
	config.g_on = 
	config.b_on = 
	config.vis_on = 
	config.trans_on = 
	config.desat_on = 
	0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("DIFF_KEY0.01"))
			{
				config.hue_imp = input.tag.get_property("HUE_IMPORTANCE", config.hue_imp);
				config.sat_imp = input.tag.get_property("SATURATION_IMPORTANCE", config.sat_imp);
				config.val_imp = input.tag.get_property("VALUE_IMPORTANCE", config.val_imp);
				config.r_imp = input.tag.get_property("RED_IMPORTANCE", config.r_imp);
				config.g_imp = input.tag.get_property("GREEN_IMPORTANCE", config.g_imp);
				config.b_imp = input.tag.get_property("BLUE_IMPORTANCE", config.b_imp);
				config.trans_thresh = input.tag.get_property("TRANSPARENCY_THRESHOLD", config.trans_thresh);
				config.vis_thresh = input.tag.get_property("VISIBILITY_THRESHOLD", config.vis_thresh);
				config.desat_thresh = input.tag.get_property("DESATURATION_THRESHOLD", config.desat_thresh);
			}

			if(input.tag.title_is("HUE_ON"))
				config.hue_on = 1;
			
			if(input.tag.title_is("SATURATION_ON"))
				config.sat_on = 1;
			
			if(input.tag.title_is("VALUE_ON"))
				config.val_on = 1;
			
			if(input.tag.title_is("RED_ON"))
				config.r_on = 1;
			
			if(input.tag.title_is("GREEN_ON"))
				config.g_on = 1;
			
			if(input.tag.title_is("BLUE_ON"))
				config.b_on = 1;
			
			if(input.tag.title_is("VISIBILITY_ON"))
				config.vis_on = 1;
			
			if(input.tag.title_is("TRANSPARENCY_ON"))
				config.trans_on = 1;
			
			if(input.tag.title_is("DESATURATION_ON"))
				config.desat_on = 1;
			
			if(input.tag.title_is("SHOW_MASK"))
				config.show_mask = 1;
		}
	}
#endif
}

int DiffKeyMain::load_defaults()
{
#ifndef NO_LOAD_DEFAULTS
	#ifdef DEBUG
	printf("\nDiffKeyMain::load_defaults");
	#endif

	/*Use directory for saving key frame to disk later*/
	
	char directory[BCTEXTLEN], string[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sdiffkey.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.hue_imp = defaults->get("HUE_IMPORTANCE", config.hue_imp);
	config.sat_imp = defaults->get("SATURATION_IMPORTANCE", config.sat_imp);
	config.val_imp = defaults->get("VALUE_IMPORTANCE", config.val_imp);
	config.r_imp = defaults->get("RED_IMPORTANCE", config.r_imp);
	config.g_imp = defaults->get("GREEN_IMPORTANCE", config.g_imp);
	config.b_imp = defaults->get("BLUE_IMPORTANCE", config.b_imp);
	config.vis_thresh = defaults->get("VISIBILITY_THRESHOLD", config.vis_thresh);
	config.trans_thresh = defaults->get("TRANSPARENCY_THRESHOLD", config.trans_thresh);
	config.desat_thresh = defaults->get("DESATURATION_THRESHOLD", config.desat_thresh);
	config.show_mask = defaults->get("SHOW_MASK", config.show_mask);
	return 0;
#endif
}

int DiffKeyMain::save_defaults()
{
#ifndef NO_SAVE_DEFAULTS
	#ifdef DEBUG
	printf("\nDiffKeyMain::save_defaults");
	#endif
	defaults->update("HUE_IMPORTANCE", config.hue_imp);
	defaults->update("SATURATION_IMPORTANCE", config.sat_imp);
	defaults->update("VALUE_IMPORTANCE", config.val_imp);
	defaults->update("RED_IMPORTANCE", config.r_imp);
	defaults->update("GREEN_IMPORTANCE", config.g_imp);
	defaults->update("BLUE_IMPORTANCE", config.b_imp);
	defaults->update("VISIBILITY_THRESHOLD", config.vis_thresh);
	defaults->update("TRANSPARENCY_THRESHOLD", config.trans_thresh);
	defaults->update("DESATURATION_THRESHOLD", config.desat_thresh);
	defaults->update("SHOW_MASK", config.show_mask);
	defaults->save();
	return 0;
#endif
}
