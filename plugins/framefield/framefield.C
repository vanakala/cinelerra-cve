#include "bcdisplayinfo.h"
#include "defaults.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <string.h>
#include <stdint.h>


#define TOP_FIELD_FIRST 0
#define BOTTOM_FIELD_FIRST 1

class FrameField;
class FrameFieldWindow;







class FrameFieldConfig
{
public:
	FrameFieldConfig();
	int equivalent(FrameFieldConfig &src);
	int field_dominance;
	int avg;
};




class FrameFieldTop : public BC_Radial
{
public:
	FrameFieldTop(FrameField *plugin, FrameFieldWindow *gui, int x, int y);
	int handle_event();
	FrameField *plugin;
	FrameFieldWindow *gui;
};


class FrameFieldBottom : public BC_Radial
{
public:
	FrameFieldBottom(FrameField *plugin, FrameFieldWindow *gui, int x, int y);
	int handle_event();
	FrameField *plugin;
	FrameFieldWindow *gui;
};


class FrameFieldDouble : public BC_CheckBox
{
public:
	FrameFieldDouble(FrameField *plugin, FrameFieldWindow *gui, int x, int y);
	int handle_event();
	FrameField *plugin;
	FrameFieldWindow *gui;
};

class FrameFieldShift : public BC_CheckBox
{
public:
	FrameFieldShift(FrameField *plugin, FrameFieldWindow *gui, int x, int y);
	int handle_event();
	FrameField *plugin;
	FrameFieldWindow *gui;
};

class FrameFieldAvg : public BC_CheckBox
{
public:
	FrameFieldAvg(FrameField *plugin, FrameFieldWindow *gui, int x, int y);
	int handle_event();
	FrameField *plugin;
	FrameFieldWindow *gui;
};

class FrameFieldWindow : public BC_Window
{
public:
	FrameFieldWindow(FrameField *plugin, int x, int y);
	void create_objects();
	int close_event();
	FrameField *plugin;
	FrameFieldTop *top;
	FrameFieldBottom *bottom;
	FrameFieldAvg *avg;
};


PLUGIN_THREAD_HEADER(FrameField, FrameFieldThread, FrameFieldWindow)



class FrameField : public PluginVClient
{
public:
	FrameField(PluginServer *server);
	~FrameField();

	PLUGIN_CLASS_MEMBERS(FrameFieldConfig, FrameFieldThread);

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();


	void average_rows(int offset, VFrame *frame);

// Last frame requested
	int64_t last_frame;
// Field needed
	int64_t field_number;
// Frame needed
	int64_t current_frame_number;
// Frame stored
	int64_t src_frame_number;
	VFrame *src_frame;
};









REGISTER_PLUGIN(FrameField)



FrameFieldConfig::FrameFieldConfig()
{
	field_dominance = TOP_FIELD_FIRST;
	avg = 1;
}

int FrameFieldConfig::equivalent(FrameFieldConfig &src)
{
	return src.field_dominance == field_dominance &&
		src.avg == avg;
}








FrameFieldWindow::FrameFieldWindow(FrameField *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	210, 
	160, 
	200, 
	160, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

void FrameFieldWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(top = new FrameFieldTop(plugin, this, x, y));
	y += 30;
	add_subwindow(bottom = new FrameFieldBottom(plugin, this, x, y));
	y += 30;
	add_subwindow(avg = new FrameFieldAvg(plugin, this, x, y));
	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(FrameFieldWindow)












FrameFieldTop::FrameFieldTop(FrameField *plugin, 
	FrameFieldWindow *gui, 
	int x, 
	int y)
 : BC_Radial(x, 
	y, 
	plugin->config.field_dominance == TOP_FIELD_FIRST,
	_("Top field first"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FrameFieldTop::handle_event()
{
	plugin->config.field_dominance = TOP_FIELD_FIRST;
	gui->bottom->update(0);
	plugin->send_configure_change();
	return 1;
}





FrameFieldBottom::FrameFieldBottom(FrameField *plugin, 
	FrameFieldWindow *gui, 
	int x, 
	int y)
 : BC_Radial(x, 
	y, 
	plugin->config.field_dominance == BOTTOM_FIELD_FIRST,
	_("Bottom field first"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FrameFieldBottom::handle_event()
{
	plugin->config.field_dominance = BOTTOM_FIELD_FIRST;
	gui->top->update(0);
	plugin->send_configure_change();
	return 1;
}





FrameFieldAvg::FrameFieldAvg(FrameField *plugin, 
	FrameFieldWindow *gui, 
	int x, 
	int y)
 : BC_CheckBox(x, 
	y, 
	plugin->config.avg,
	_("Average empty rows"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FrameFieldAvg::handle_event()
{
	plugin->config.avg = get_value();
	plugin->send_configure_change();
	return 1;
}



PLUGIN_THREAD_OBJECT(FrameField, FrameFieldThread, FrameFieldWindow)
















FrameField::FrameField(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	field_number = 0;
	src_frame = 0;
	src_frame_number = -1;
	last_frame = -1;
}


FrameField::~FrameField()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(src_frame) delete src_frame;
}


// 0 - current frame field 0, prev frame field 1
// 1 - current frame field 0, current frame field 1, copy current to prev
// 2 - current frame field 0, prev frame field 1

int FrameField::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	int row_size = VFrame::calculate_bytes_per_pixel(frame->get_color_model()) * 
		frame->get_w();
	int start_row;

	if(src_frame &&
		src_frame->get_color_model() != frame->get_color_model())
	{
		delete src_frame;
		src_frame = 0;
	}
	if(!src_frame)
	{
		src_frame = new VFrame(0, 
			frame->get_w(), 
			frame->get_h(), 
			frame->get_color_model());
	}

	unsigned char **src_rows = src_frame->get_rows();
	unsigned char **output_rows = frame->get_rows();

// Calculate current field based on absolute position so the algorithm isn't
// relative to where playback started.
	field_number = get_source_position() % 2;
	current_frame_number = start_position / 2;

// Import source frame at half frame rate
	if(current_frame_number != src_frame_number ||
// If same frame was requested, assume it was a configuration change and reprocess.
		start_position == last_frame)
	{
		read_frame(src_frame, 
			0, 
			current_frame_number, 
			frame_rate / 2);
		src_frame_number = current_frame_number;
	}


// Even field
	if(field_number == 0)
	{
		if(config.field_dominance == TOP_FIELD_FIRST) 
		{
			for(int i = 0; i < frame->get_h() - 1; i += 2)
			{
// Copy even lines of src to both lines of output
				memcpy(output_rows[i], 
					src_rows[i], 
					row_size);
				if(!config.avg) 
					memcpy(output_rows[i + 1], 
						src_rows[i], 
						row_size);
			}

// Average empty rows
			if(config.avg) average_rows(0, frame);
		}
		else
		{
			for(int i = 0; i < frame->get_h() - 1; i += 2)
			{
// Copy odd lines of current to both lines of output with shift up.
				memcpy(output_rows[i + 1], 
					src_rows[i + 1], 
					row_size);
				if(i < frame->get_h() - 2 && !config.avg)
					memcpy(output_rows[i + 2], 
						src_rows[i + 1], 
						row_size);
			}

// Average empty rows
			if(config.avg) average_rows(1, frame);
		}
	}
	else
// Odd field
	{
		if(config.field_dominance == TOP_FIELD_FIRST)
		{
			for(int i = 0; i < frame->get_h() - 1; i += 2)
			{
// Copy odd lines of src to both lines of output
				memcpy(output_rows[i + 1], 
					src_rows[i + 1], 
					row_size);
				if(i < frame->get_h() - 2 && !config.avg)
					memcpy(output_rows[i + 2], 
						src_rows[i + 1], 
						row_size);
			}

// Average empty rows
			if(config.avg) average_rows(1, frame);
		}
		else
		{
			for(int i = 0; i < frame->get_h() - 1; i += 2)
			{
// Copy even lines of src to both lines of output.
				memcpy(output_rows[i], 
					src_rows[i], 
					row_size);
				if(!config.avg) 
					memcpy(output_rows[i + 1], 
						src_rows[i], 
						row_size);
			}

// Average empty rows
			if(config.avg) average_rows(0, frame);
		}
	}

	last_frame = start_position;
	return 0;
}

// Averaging 2 pixels
#define AVERAGE(type, temp_type, components, offset) \
{ \
	type **rows = (type**)frame->get_rows(); \
	int w = frame->get_w(); \
	int h = frame->get_h(); \
	int row_size = components * w; \
	for(int i = offset; i < h - 3; i += 2) \
	{ \
		type *row1 = rows[i]; \
		type *row2 = rows[i + 1]; \
		type *row3 = rows[i + 2]; \
		for(int j = 0; j < row_size; j++) \
		{ \
			temp_type sum = (temp_type)*row1++ + (temp_type)*row3++; \
			*row2++ = (sum / 2); \
		} \
	} \
}

// Averaging 6 pixels
#define AVERAGE_BAK(type, components, offset) \
{ \
	type **rows = (type**)frame->get_rows(); \
	int w = frame->get_w(); \
	int h = frame->get_h(); \
	int row_size = w; \
	for(int i = offset; i < h - 3; i += 2) \
	{ \
		type *row1 = rows[i]; \
		type *row2 = rows[i + 1]; \
		type *row3 = rows[i + 2]; \
		int64_t sum; \
		int64_t pixel1[4], pixel2[4], pixel3[4]; \
		int64_t pixel4[4], pixel5[4], pixel6[4]; \
 \
/* First pixel */ \
		for(int j = 0; j < components; j++) \
		{ \
			pixel1[j] = *row1++; \
			pixel4[j] = *row3++; \
			*row2++ = (pixel1[j] + pixel4[j]) >> 1; \
		} \
 \
		for(int j = 2; j < row_size; j++) \
		{ \
			for(int k = 0; k < components; k++) \
			{ \
				pixel2[k] = *row1++; \
				pixel5[k] = *row3++; \
			} \
 \
			for(int k = 0; k < components; k++) \
			{ \
				pixel3[k] = *row1; \
				pixel6[k] = *row3; \
				*row2++ = (pixel1[k] + \
					pixel2[k] + \
					pixel3[k] + \
					pixel4[k] + \
					pixel5[k] + \
					pixel6[k]) / 6; \
				pixel1[k] = pixel2[k]; \
				pixel4[k] = pixel5[k]; \
			} \
		} \
 \
/* Last pixel */ \
		for(int j = 0; j < components; j++) \
		{ \
			*row2++ = (pixel3[j] + pixel6[j]) >> 1; \
		} \
	} \
}

void FrameField::average_rows(int offset, VFrame *frame)
{
//printf("FrameField::average_rows 1 %d\n", offset);
	switch(frame->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			AVERAGE(unsigned char, int64_t, 3, offset);
			break;
		case BC_RGB_FLOAT:
			AVERAGE(float, float, 3, offset);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			AVERAGE(unsigned char, int64_t, 4, offset);
			break;
		case BC_RGBA_FLOAT:
			AVERAGE(float, float, 4, offset);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			AVERAGE(uint16_t, int64_t, 3, offset);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			AVERAGE(uint16_t, int64_t, 4, offset);
			break;
	}
}



char* FrameField::plugin_title() { return N_("Frames to fields"); }
int FrameField::is_realtime() { return 1; }

NEW_PICON_MACRO(FrameField) 

SHOW_GUI_MACRO(FrameField, FrameFieldThread)

RAISE_WINDOW_MACRO(FrameField)

SET_STRING_MACRO(FrameField);

int FrameField::load_configuration()
{
	KeyFrame *prev_keyframe;
	FrameFieldConfig old_config = config;

	prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(prev_keyframe);

	return !old_config.equivalent(config);
}

int FrameField::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sframefield.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.field_dominance = defaults->get("DOMINANCE", config.field_dominance);
	config.avg = defaults->get("AVG", config.avg);
	return 0;
}

int FrameField::save_defaults()
{
	defaults->update("DOMINANCE", config.field_dominance);
	defaults->update("AVG", config.avg);
	defaults->save();
	return 0;
}

void FrameField::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("FRAME_FIELD");
	output.tag.set_property("DOMINANCE", config.field_dominance);
	output.tag.set_property("AVG", config.avg);
	output.append_tag();
	output.terminate_string();
}

void FrameField::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("FRAME_FIELD"))
		{
			config.field_dominance = input.tag.get_property("DOMINANCE", config.field_dominance);
			config.avg = input.tag.get_property("AVG", config.avg);
		}
	}
}

void FrameField::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window();
			thread->window->top->update(config.field_dominance == TOP_FIELD_FIRST);
			thread->window->bottom->update(config.field_dominance == BOTTOM_FIELD_FIRST);
			thread->window->unlock_window();
		}
	}
}





