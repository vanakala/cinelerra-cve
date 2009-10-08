
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "transportque.inc"
#include "vframe.h"

#include <string.h>
#include <stdint.h>


#define TOP_FIELD_FIRST 0
#define BOTTOM_FIELD_FIRST 1

class FrameField;
class FrameFieldWindow;



// 601 to RGB expansion is provided as a convenience for OpenGL users since
// frame bobbing is normally done during playback together with 601 to RGB expansion.
// It's not optimized for software.


class FrameFieldConfig
{
public:
	FrameFieldConfig();
	int equivalent(FrameFieldConfig &src);
	int field_dominance;
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

// Constructs odd or even rows from the average of the surrounding rows.
	void average_rows(int offset, VFrame *frame);

	int handle_opengl();

// Last frame requested
	int64_t last_frame;
// Field needed
	int64_t field_number;
// Frame needed
	int64_t current_frame_number;
// Frame stored
	int64_t src_frame_number;
	VFrame *src_frame;

// Temporary storage of input frame for OpenGL
	BC_Texture *src_texture;
// Signal OpenGL handler a new frame was read.
	int new_frame;
// Reading frames at a different rate requires us to store the aggregation
// state when the frame isn't read.
	int aggregate_rgb601;
	int rgb601_direction;
};









REGISTER_PLUGIN(FrameField)



FrameFieldConfig::FrameFieldConfig()
{
	field_dominance = TOP_FIELD_FIRST;
}

int FrameFieldConfig::equivalent(FrameFieldConfig &src)
{
	return src.field_dominance == field_dominance;
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
	y += top->get_h() + 5;
	add_subwindow(bottom = new FrameFieldBottom(plugin, this, x, y));
	y += bottom->get_h() + 5;
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







PLUGIN_THREAD_OBJECT(FrameField, FrameFieldThread, FrameFieldWindow)
















FrameField::FrameField(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	field_number = 0;
	src_frame = 0;
	src_frame_number = -1;
	last_frame = -1;
	src_texture = 0;
	aggregate_rgb601 = 0;
	rgb601_direction = 0;
}


FrameField::~FrameField()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(src_frame) delete src_frame;
	if(src_texture) delete src_texture;
}


// 0 - current frame field 0, prev frame field 1
// 1 - current frame field 0, current frame field 1, copy current to prev
// 2 - current frame field 0, prev frame field 1

int FrameField::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	new_frame = 0;

// Calculate current field based on absolute position so the algorithm isn't
// relative to where playback started.
	field_number = get_source_position() % 2;

	if (get_direction() == PLAY_REVERSE)
	{
		start_position++;
		field_number = (field_number + 1) % 2;
	}


	current_frame_number = start_position / 2;

	VFrame *ptr = frame;
	if(get_use_opengl())
	{
// Read new frames directly into output frame for hardware
	}
	else
	{
// Read into temporary for software
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
		ptr = src_frame;
	}


// Import source frame at half frame rate
	if(current_frame_number != src_frame_number ||
// If same frame was requested, assume it was a configuration change and reprocess.
		start_position == last_frame)
	{
		read_frame(ptr, 
			0, 
			current_frame_number, 
			frame_rate / 2,
			get_use_opengl());
		src_frame_number = current_frame_number;
		new_frame = 1;
	}


	if(get_use_opengl())
	{
		run_opengl();
		return 0;
	}

	int row_size = VFrame::calculate_bytes_per_pixel(frame->get_color_model()) * 
		frame->get_w();
	int start_row;

	unsigned char **src_rows = src_frame->get_rows();
	unsigned char **output_rows = frame->get_rows();


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
			}

// Average empty rows
			/* if(config.avg) */ average_rows(0, frame);
		}
		else
		{
			for(int i = 0; i < frame->get_h() - 1; i += 2)
			{
// Copy odd lines of current to both lines of output with shift up.
				memcpy(output_rows[i + 1], 
					src_rows[i + 1], 
					row_size);
			}

// Average empty rows
			/* if(config.avg) */ average_rows(1, frame);
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
			}

// Average empty rows
			/* if(config.avg) */ average_rows(1, frame);
		}
		else
		{
			for(int i = 0; i < frame->get_h() - 1; i += 2)
			{
// Copy even lines of src to both lines of output.
				memcpy(output_rows[i], 
					src_rows[i], 
					row_size);
			}

// Average empty rows
			/* if(config.avg) */ average_rows(0, frame);
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
	defaults = new BC_Hash(directory);
	defaults->load();

	config.field_dominance = defaults->get("DOMINANCE", config.field_dominance);
	return 0;
}

int FrameField::save_defaults()
{
	defaults->update("DOMINANCE", config.field_dominance);
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
	output.append_tag();
	output.tag.set_title("/FRAME_FIELD");
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

int FrameField::handle_opengl()
{
#ifdef HAVE_GL
	static char *field_frag = 
		"uniform sampler2D tex;\n"
		"uniform float double_line_h;\n"
		"uniform float y_offset;\n"
		"void main()\n"
		"{\n"
		"	vec2 coord = gl_TexCoord[0].st;\n"
/* Number of double lines + fraction of current double line */
		"	float half_y = (coord.y - y_offset) / double_line_h;\n"
/* Lines comprising current double line */
		"	float line1 = floor(half_y) * double_line_h + y_offset;\n"
		"	float line2 = line1 + double_line_h;\n"
/* Distance from line1 to line2 */
		"	float frac = fract(half_y);\n"
		"	gl_FragColor =  mix(\n"
		"		texture2D(tex, vec2(coord.x, line1)), \n"
		"		texture2D(tex, vec2(coord.x, line2)), \n"
		"		frac);\n"
		"}\n";

	static char *_601_to_rgb_frag = 
		"void main()\n"
		"{\n"
		"	gl_FragColor.rgb = gl_FragColor.rgb * vec3(1.1644, 1.1644, 1.1644) - vec3(0.0627, 0.0627, 0.0627);\n"
		"}\n";

	static char *_601_to_yuv_frag = 
		"void main()\n"
		"{\n"
		"	gl_FragColor.r = gl_FragColor.r * 1.1644 - 0.0627;\n"
		"}\n";

	static char *rgb_to_601_frag = 
		"void main()\n"
		"{\n"
		"	gl_FragColor.rgb = gl_FragColor.rgb * vec3(0.8588, 0.8588, 0.8588) + vec3(0.0627, 0.0627, 0.0627);\n"
		"}\n";

	static char *yuv_to_601_frag = 
		"void main()\n"
		"{\n"
		"	gl_FragColor.r = gl_FragColor.r * 0.8588 + 0.0627;\n"
		"}\n";


	if(new_frame)
	{
		if(get_output()->get_opengl_state() != VFrame::SCREEN)
		{
// Copy new frame to temporary texture
			get_output()->to_texture();

// Draw it only to copy it to the temporary.
			get_output()->enable_opengl();
			VFrame::init_screen(get_output()->get_w(), get_output()->get_h());
			get_output()->bind_texture(0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			get_output()->draw_texture();
		}

		get_output()->enable_opengl();
		VFrame::init_screen(get_output()->get_w(), get_output()->get_h());
		glActiveTexture(GL_TEXTURE0);
		BC_Texture::new_texture(&src_texture,
			get_output()->get_w(), 
			get_output()->get_h(), 
			get_output()->get_color_model());
		src_texture->bind(0);
		glCopyTexSubImage2D(GL_TEXTURE_2D,
			0,
			0,
			0,
			0,
			0,
			get_output()->get_w(),
			get_output()->get_h());

// Store aggregation state only when reading a frame
//printf("FrameField::handle_opengl %p\n", get_output());
//get_output()->dump_stacks();
		if(prev_effect_is("RGB - 601") ||
			next_effect_is("RGB - 601"))
		{
			aggregate_rgb601 = 1;
			rgb601_direction = get_output()->get_params()->get("RGB601_DIRECTION", 0);
		}
		else
			aggregate_rgb601 = 0;
	}
	else
	{
		get_output()->enable_opengl();
	}

	unsigned int frag = 0;
	float y_offset = 0.0;
	if(field_number == 0)
	{
		if(config.field_dominance == BOTTOM_FIELD_FIRST)
			y_offset = 1.0;
	}
	else
	{
		if(config.field_dominance == TOP_FIELD_FIRST)
			y_offset = 1.0;
	}

	VFrame::init_screen(get_output()->get_w(), get_output()->get_h());
	glActiveTexture(GL_TEXTURE0);
	BC_Texture::new_texture(&src_texture,
		get_output()->get_w(), 
		get_output()->get_h(), 
		get_output()->get_color_model());


	char *shaders[3] = { 0, 0, 0 };
	shaders[0] = field_frag;


// Aggregate with other effect
//printf("FrameField::handle_opengl %s\n", get_output()->get_next_effect());
	if(aggregate_rgb601)
	{
		if(rgb601_direction == 1)
		{
			if(cmodel_is_yuv(get_output()->get_color_model()))
				shaders[1] = yuv_to_601_frag;
			else
				shaders[1] = rgb_to_601_frag;
		}
		else
		if(rgb601_direction == 2)
		{
			if(cmodel_is_yuv(get_output()->get_color_model()))
				shaders[1] = _601_to_yuv_frag;
			else
				shaders[1] = _601_to_rgb_frag;
		}
	}



	frag = VFrame::make_shader(0, shaders[0], shaders[1], shaders[2], 0);


	if(frag)
	{
		glUseProgram(frag);
		glUniform1i(glGetUniformLocation(frag, "tex"), 0);
		glUniform1f(glGetUniformLocation(frag, "double_line_h"), 
			2.0 / src_texture->get_texture_h());
		glUniform1f(glGetUniformLocation(frag, "y_offset"), 
			y_offset / src_texture->get_texture_h());
	}



	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	get_output()->draw_texture();

	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);

// Reset for other operations
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

#endif
}


