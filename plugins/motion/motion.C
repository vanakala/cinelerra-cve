#include "bcdisplayinfo.h"
#include "clip.h"
#include "filexml.h"
#include "keyframe.h"
#include "motion.h"
#include "overlayframe.h"
#include "picon_png.h"


#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)





REGISTER_PLUGIN(MotionMain)



MotionConfig::MotionConfig()
{
	search_radius = 32;
	block_size = 16;
	magnitude = 128;
	return_speed = 128;
	mode = STABILIZE;
}

int MotionConfig::equivalent(MotionConfig &that)
{
	return search_radius == that.search_radius &&
		mode == that.mode &&
		block_size == that.block_size &&
		magnitude == that.magnitude &&
		return_speed == that.return_speed;
}

void MotionConfig::copy_from(MotionConfig &that)
{
	search_radius = that.search_radius;
	mode = that.mode;
	block_size = that.block_size;
	magnitude = that.magnitude;
	return_speed = that.return_speed;
}

void MotionConfig::interpolate(MotionConfig &prev, 
	MotionConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	copy_from(prev);
}









PLUGIN_THREAD_OBJECT(MotionMain, MotionThread, MotionWindow)



MotionWindow::MotionWindow(MotionMain *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x,
	y,
	290, 
	250, 
	290,
	250,
	0, 
	1)
{
	this->plugin = plugin; 
}

MotionWindow::~MotionWindow()
{
}

int MotionWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, _("Search radius:")));
	add_subwindow(radius = new MotionSearchRadius(plugin, 
		x + 190, 
		y));

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Block size:")));
	add_subwindow(block_size = new MotionBlockSize(plugin, 
		x + 230, 
		y));

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Maximum absolute offset:")));
	add_subwindow(magnitude = new MotionMagnitude(plugin, 
		x + 190, 
		y));

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Stabilizer settling time:")));
	add_subwindow(return_speed = new MotionReturnSpeed(plugin,
		x + 230, 
		y));

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Mode:")));
	y += 20;
	add_subwindow(track = new MotionTrack(plugin, 
		this,
		x, 
		y));
	y += 30;
	add_subwindow(stabilize = new MotionStabilize(plugin, 
		this,
		x, 
		y));

	y += 30;
	add_subwindow(vectors = new MotionDrawVectors(plugin,
		this,
		x,
		y));

	show_window();
	flush();
	return 0;
}

void MotionWindow::update_mode()
{
	stabilize->update(plugin->config.mode == MotionConfig::STABILIZE);
	track->update(plugin->config.mode == MotionConfig::TRACK);
	vectors->update(plugin->config.mode == MotionConfig::DRAW_VECTORS);
}


WINDOW_CLOSE_EVENT(MotionWindow)










MotionSearchRadius::MotionSearchRadius(MotionMain *plugin, 
	int x, 
	int y)
 : BC_IPot(x, 
		y, 
		(int64_t)plugin->config.search_radius,
		(int64_t)0,
		(int64_t)128)
{
	this->plugin = plugin;
}


int MotionSearchRadius::handle_event()
{
	plugin->config.search_radius = get_value();
	plugin->send_configure_change();
	return 1;
}







MotionBlockSize::MotionBlockSize(MotionMain *plugin, 
	int x, 
	int y)
 : BC_IPot(x, 
		y, 
		(int64_t)plugin->config.block_size,
		(int64_t)0,
		(int64_t)128)
{
	this->plugin = plugin;
}

int MotionBlockSize::handle_event()
{
	plugin->config.block_size =get_value();
	plugin->send_configure_change();
	return 1;
}


MotionMagnitude::MotionMagnitude(MotionMain *plugin, 
	int x, 
	int y)
 : BC_IPot(x, 
		y, 
		(int64_t)plugin->config.magnitude,
		(int64_t)0,
		(int64_t)480)
{
	this->plugin = plugin;
}

int MotionMagnitude::handle_event()
{
	plugin->config.magnitude = get_value();
	plugin->send_configure_change();
	return 1;
}


MotionReturnSpeed::MotionReturnSpeed(MotionMain *plugin, 
	int x, 
	int y)
 : BC_IPot(x, 
		y, 
		(int64_t)plugin->config.return_speed,
		(int64_t)0,
		(int64_t)128)
{
	this->plugin = plugin;
}

int MotionReturnSpeed::handle_event()
{
	plugin->config.return_speed = get_value();
	plugin->send_configure_change();
	return 1;
}






MotionStabilize::MotionStabilize(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_Radial(x, 
 	y, 
	plugin->config.mode == MotionConfig::STABILIZE,
	_("Stabilize"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int MotionStabilize::handle_event()
{
	plugin->config.mode = MotionConfig::STABILIZE;
	gui->update_mode();
	plugin->send_configure_change();
	return 1;
}





MotionTrack::MotionTrack(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_Radial(x,
 	y, 
	plugin->config.mode == MotionConfig::TRACK,
	_("Track"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int MotionTrack::handle_event()
{
	plugin->config.mode = MotionConfig::TRACK;
	gui->update_mode();
	plugin->send_configure_change();
	return 1;
}





MotionDrawVectors::MotionDrawVectors(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_Radial(x,
 	y, 
	plugin->config.mode == MotionConfig::DRAW_VECTORS,
	_("Draw vectors"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int MotionDrawVectors::handle_event()
{
	plugin->config.mode = MotionConfig::DRAW_VECTORS;
	gui->update_mode();
	plugin->send_configure_change();
	return 1;
}












MotionMain::MotionMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
	prev_frame = 0;
	macroblocks = 0;
	total_macroblocks = 0;
	current_dx = 0;
	current_dy = 0;
	overlayer = 0;
	search_area = 0;
	search_size = 0;
	temp_frame = 0;
}

MotionMain::~MotionMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(engine) delete engine;
	if(prev_frame) delete prev_frame;
	if(macroblocks) delete [] macroblocks;
	if(overlayer) delete overlayer;
	if(search_area) delete [] search_area;
	if(temp_frame) delete temp_frame;
}

char* MotionMain::plugin_title() { return _("Motion"); }
int MotionMain::is_realtime() { return 1; }
int MotionMain::is_multichannel() { return 1; }

NEW_PICON_MACRO(MotionMain)

SHOW_GUI_MACRO(MotionMain, MotionThread)

SET_STRING_MACRO(MotionMain)

RAISE_WINDOW_MACRO(MotionMain)

LOAD_CONFIGURATION_MACRO(MotionMain, MotionConfig)



void MotionMain::update_gui()
{
	if(thread)
	{
		thread->window->lock_window();
		load_configuration();
		thread->window->radius->update(config.search_radius);
		thread->window->block_size->update(config.block_size);
		thread->window->magnitude->update(config.magnitude);
		thread->window->return_speed->update(config.return_speed);
		thread->window->update_mode();
		thread->window->unlock_window();
	}
}


int MotionMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%smotion.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.block_size = defaults->get("BLOCK_SIZE", config.block_size);
	config.search_radius = defaults->get("SEARCH_RADIUS", config.search_radius);
	config.magnitude = defaults->get("MAGNITUDE", config.magnitude);
	config.return_speed = defaults->get("RETURN_SPEED", config.return_speed);
	config.mode = defaults->get("MODE", config.mode);
	return 0;
}


int MotionMain::save_defaults()
{
	defaults->update("BLOCK_SIZE", config.block_size);
	defaults->update("SEARCH_RADIUS", config.search_radius);
	defaults->update("MAGNITUDE", config.magnitude);
	defaults->update("RETURN_SPEED", config.return_speed);
	defaults->update("MODE", config.mode);
	defaults->save();
	return 0;
}



void MotionMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("MOTION");

	output.tag.set_property("BLOCK_SIZE", config.block_size);
	output.tag.set_property("SEARCH_RADIUS", config.search_radius);
	output.tag.set_property("MAGNITUDE", config.magnitude);
	output.tag.set_property("RETURN_SPEED", config.return_speed);
	output.tag.set_property("MODE", config.mode);
	output.append_tag();
	output.terminate_string();
}

void MotionMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("MOTION"))
			{
				config.block_size = input.tag.get_property("BLOCK_SIZE", config.block_size);
				config.search_radius = input.tag.get_property("SEARCH_RADIUS", config.search_radius);
				config.magnitude = input.tag.get_property("MAGNITUDE", config.magnitude);
				config.return_speed = input.tag.get_property("RETURN_SPEED", config.return_speed);
				config.mode = input.tag.get_property("MODE", config.mode);
			}
		}
	}
}




void MotionMain::oversample(int32_t *dst, 
	VFrame *src,
	int x1,
	int x2,
	int y1,
	int y2,
	int dst_stride)
{

#define DO_OVERSAMPLE(components, type) \
{ \
	int oversample_w = (x2 - x1) * OVERSAMPLE; \
	int oversample_h = (y2 - y1) * OVERSAMPLE; \
	type **src_rows = (type**)src->get_rows(); \
 \
	for(int i = 0; i < oversample_h; i++) \
	{ \
		int row2_factor = i % OVERSAMPLE; \
		int row1_factor = OVERSAMPLE - row2_factor; \
		int row1_number = i / OVERSAMPLE + y1; \
		int row2_number = row1_number + 1; \
		row2_number = MIN(row2_number, y2 - 1); \
 \
		type *row1 = src_rows[row1_number]; \
		type *row2 = src_rows[row2_number]; \
		int32_t *dst_row = dst + i * dst_stride; \
 \
		for(int j = 0; j < oversample_w; j++) \
		{ \
			int pixel2_factor = j % OVERSAMPLE; \
			int pixel1_factor = OVERSAMPLE - pixel2_factor; \
			int pixel1_number = j / OVERSAMPLE + x1; \
			int pixel2_number = pixel1_number + 1; \
			pixel2_number = MIN(pixel2_number, x2 - 1); \
 \
			type *pixel1 = row1 + pixel1_number * components; \
			type *pixel2 = row1 + pixel2_number * components; \
			type *pixel3 = row2 + pixel1_number * components; \
			type *pixel4 = row2 + pixel2_number * components; \
			int32_t dst_pixel = 0; \
			for(int k = 0; k < components; k++) \
			{ \
				dst_pixel += *pixel1++ * pixel1_factor * row1_factor; \
				dst_pixel += *pixel2++ * pixel2_factor * row1_factor; \
				dst_pixel += *pixel3++ * pixel1_factor * row2_factor; \
				dst_pixel += *pixel4++ * pixel2_factor * row2_factor; \
			} \
			*dst_row++ = dst_pixel; \
		} \
	} \
}

	switch(src->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			DO_OVERSAMPLE(3, unsigned char);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			DO_OVERSAMPLE(3, uint16_t);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			DO_OVERSAMPLE(4, unsigned char);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			DO_OVERSAMPLE(4, uint16_t);
			break;
	}

}

void MotionMain::draw_pixel(VFrame *frame, int x, int y)
{
	if(!(x >= 0 && y >= 0 && x < frame->get_w() && y < frame->get_h())) return;
#define DRAW_PIXEL(x, y, components, do_yuv, max, type) \
{ \
	type **rows = (type**)frame->get_rows(); \
	rows[y][x * components] = max; \
	if(!do_yuv) \
	{ \
		rows[y][x * components + 1] = max; \
		rows[y][x * components + 2] = max; \
	} \
	if(components == 4) \
		rows[y][x * components + 3] = max; \
}


	switch(frame->get_color_model())
	{
		case BC_RGB888:
			DRAW_PIXEL(x, y, 3, 0, 0xff, unsigned char);
			break;
		case BC_YUV888:
			DRAW_PIXEL(x, y, 3, 1, 0xff, unsigned char);
			break;
		case BC_RGBA8888:
			DRAW_PIXEL(x, y, 4, 0, 0xff, unsigned char);
			break;
		case BC_YUVA8888:
			DRAW_PIXEL(x, y, 4, 1, 0xff, unsigned char);
			break;
		case BC_RGB161616:
			DRAW_PIXEL(x, y, 3, 0, 0xffff, uint16_t);
			break;
		case BC_YUV161616:
			DRAW_PIXEL(x, y, 3, 1, 0xffff, uint16_t);
			break;
		case BC_RGBA16161616:
			DRAW_PIXEL(x, y, 4, 0, 0xffff, uint16_t);
			break;
		case BC_YUVA16161616:
			DRAW_PIXEL(x, y, 4, 1, 0xffff, uint16_t);
			break;
	}
}


void MotionMain::draw_line(VFrame *frame, int x1, int y1, int x2, int y2)
{
	int w = labs(x2 - x1);
	int h = labs(y2 - y1);
//printf("MotionMain::draw_line %d %d %d %d\n", x1, y1, x2, y2);

	if(!w && !h)
	{
		draw_pixel(frame, x1, y1);
	}
	else
	if(w > h)
	{
// Flip coordinates so x1 < x2
		if(x2 < x1)
		{
			y2 ^= y1;
			y1 ^= y2;
			y2 ^= y1;
			x1 ^= x2;
			x2 ^= x1;
			x1 ^= x2;
		}
		int numerator = y2 - y1;
		int denominator = x2 - x1;
		for(int i = x1; i < x2; i++)
		{
			int y = y1 + (i - x1) * numerator / denominator;
			draw_pixel(frame, i, y);
		}
	}
	else
	{
// Flip coordinates so y1 < y2
		if(y2 < y1)
		{
			y2 ^= y1;
			y1 ^= y2;
			y2 ^= y1;
			x1 ^= x2;
			x2 ^= x1;
			x1 ^= x2;
		}
		int numerator = x2 - x1;
		int denominator = y2 - y1;
		for(int i = y1; i < y2; i++)
		{
			int x = x1 + (i - y1) * numerator / denominator;
			draw_pixel(frame, x, i);
		}
	}
}

int MotionMain::process_realtime(VFrame **input_ptr, VFrame **output_ptr)
{
	int need_reconfigure = load_configuration();
	int bottom_layer = PluginClient::total_in_buffers - 1;
	current_frame = input_ptr[bottom_layer];
	int w = current_frame->get_w();
	int h = current_frame->get_h();
	int color_model = current_frame->get_color_model();
	int skip_current = 0;
	VFrame *input = input_ptr[0];
	VFrame *output = output_ptr[0];

printf("MotionMain::process_realtime 1\n");

	if(!temp_frame)
		temp_frame = new VFrame(0, 
			input->get_w(), 
			input->get_h(), 
			input->get_color_model());


	if(!engine) engine = new MotionEngine(this,
		PluginClient::get_project_smp() + 1,
		PluginClient::get_project_smp() + 1);

	if(need_reconfigure) skip_current = 1;
	if(!prev_frame)
	{
		prev_frame = new VFrame(0, w, h, color_model);
		skip_current = 1;
	}

	int new_total = (w / config.block_size) * (h / config.block_size);
	if(total_macroblocks != new_total) 
	{
		delete [] macroblocks;
		macroblocks = 0;
	}

	if(!macroblocks)
	{
		total_macroblocks = new_total;
		macroblocks = new macroblock_t[total_macroblocks];
	}
	bzero(macroblocks, sizeof(macroblock_t) * total_macroblocks);
	int per_row = w / config.block_size;
	for(int i = 0; i < total_macroblocks; i++)
	{
		macroblock_t *macroblock = &macroblocks[i];
		macroblock->x = config.block_size * (i % per_row);
		macroblock->y = config.block_size * (i / per_row);
	}


// Skip processing
	if(skip_current)
	{
		prev_frame->copy_from(input_ptr[PluginClient::total_in_buffers - 1]);
		for(int i = 0; i < PluginClient::total_in_buffers; i++)
		{
			if(!output_ptr[i]->equals(input_ptr[i]))
				output_ptr[i]->copy_from(input_ptr[i]);
		}
	}
	else
	{
// Create oversampled buffers
		int new_search_size = current_frame->get_w() * 
			OVERSAMPLE * 
			current_frame->get_h() * 
			OVERSAMPLE;
		if(new_search_size != search_size && search_area)
		{
			delete [] search_area;
			search_area = 0;
		}

		if(!search_area)
		{
			search_area = new int32_t[new_search_size];
			search_size = new_search_size;
		}

		MotionMain::oversample(search_area, 
			current_frame,
			0,
			current_frame->get_w(),
			0,
			current_frame->get_h(),
			current_frame->get_w() * OVERSAMPLE);

// Get the motion vectors
//printf("MotionMain::process_realtime 1\n");
		engine->set_package_count(total_macroblocks);
//printf("MotionMain::process_realtime 1\n");
		engine->process_packages();
//printf("MotionMain::process_realtime 1\n");

// Put the current frame in the temporary for the next run
		prev_frame->copy_from(current_frame);

//printf("MotionMain::process_realtime 1\n");
// Get average of all motion vectors
		int avg_dy = 0;
		int avg_dx = 0;
		int total_valid = 0;
		for(int i = 0; i < total_macroblocks; i++)
		{
			if(macroblocks[i].valid)
			{
				avg_dy += macroblocks[i].dy_oversampled;
				avg_dx += macroblocks[i].dx_oversampled;
				total_valid++;
			}
		}

//printf("MotionMain::process_realtime 1 %d %d %d\n", avg_dx, avg_dy, total_valid);
		if(total_valid)
		{
			avg_dx /= total_valid;
			avg_dy /= total_valid;
		}

		if(!overlayer) overlayer = new OverlayFrame(
			PluginClient::get_project_smp() + 1);
		if(config.mode == MotionConfig::TRACK)
		{
// Match top layer to bottom layer's motion
			current_dx += avg_dx;
			current_dy += avg_dy;

// Clamp to absolute limit
			CLAMP(current_dx, -config.magnitude * OVERSAMPLE, config.magnitude * OVERSAMPLE);
			CLAMP(current_dy, -config.magnitude * OVERSAMPLE, config.magnitude * OVERSAMPLE);

// Translate top layer
			temp_frame->copy_from(input);
			overlayer->overlay(output, 
				temp_frame, 
				0, 
				0, 
				input->get_w(), 
				input->get_h(), 
				(float)current_dx / OVERSAMPLE, 
				(float)current_dy / OVERSAMPLE, 
				output->get_w() + (float)current_dx / OVERSAMPLE, 
				output->get_h() + (float)current_dy / OVERSAMPLE, 
				1.0, 
				TRANSFER_REPLACE,
				CUBIC_LINEAR);
printf("MotionMain::process_realtime 10 %d %d\n", current_dx, current_dy);
		}
		else
		if(config.mode == MotionConfig::STABILIZE)
		{
// Move top layer opposite to bottom layer's motion
			current_dx -= avg_dx;
			current_dy -= avg_dy;

// Clamp to absolute limit
			CLAMP(current_dx, -config.magnitude * OVERSAMPLE, config.magnitude * OVERSAMPLE);
			CLAMP(current_dy, -config.magnitude * OVERSAMPLE, config.magnitude * OVERSAMPLE);
printf("MotionMain::process_realtime 100 %d %d %d %d\n", avg_dx, avg_dy, current_dx, current_dy);

// for(int i = 0; i < output->get_h(); i++)
// {
// 	for(int j = 0; j < output->get_w(); j++)
// 	{
// 		output->get_rows()[i][j * 3] = search_area[(i * 4) * w * OVERSAMPLE + j * 4] >> 4;
// 		output->get_rows()[i][j * 3 + 1] = search_area[(i * 4) * w * OVERSAMPLE + j * 4] >> 4;
// 		output->get_rows()[i][j * 3 + 2] = search_area[(i * 4) * w * OVERSAMPLE + j * 4] >> 4;
// 	}
// }

// Translate top layer
			temp_frame->copy_from(input);
			overlayer->overlay(output, 
				temp_frame, 
				0, 
				0, 
				input->get_w(), 
				input->get_h(), 
				(float)current_dx / OVERSAMPLE, 
				(float)current_dy / OVERSAMPLE, 
				output->get_w() + (float)current_dx / OVERSAMPLE, 
				output->get_h() + (float)current_dy / OVERSAMPLE, 
				1.0, 
				TRANSFER_REPLACE,
				CUBIC_LINEAR);
		}
		else
		if(config.mode == MotionConfig::DRAW_VECTORS)
		{
			if(!output->equals(input))
				output->copy_from(input);
printf("MotionMain::process_realtime 110 %d %d %d %d\n", avg_dx, avg_dy, current_dx, current_dy);
			for(int i = 0; i < total_macroblocks; i++)
			{
				if(macroblocks[i].valid)
				{
					int x1 = macroblocks[i].x + config.block_size / 2;
					int y1 = macroblocks[i].y + config.block_size / 2;
					int x2 = x1 + macroblocks[i].dx_oversampled / OVERSAMPLE;
					int y2 = y1 + macroblocks[i].dy_oversampled / OVERSAMPLE;
//printf("MotionMain::process_realtime 10 %d %d %d %d\n", x1, y1, x2, y2);
					draw_line(output, 
						x1,
						y1,
						x2,
						y2);
				}
			}
		}
	}

	return 1;
}












MotionPackage::MotionPackage()
 : LoadPackage()
{
}




MotionUnit::MotionUnit(MotionEngine *server, 
	MotionMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
	block_area = 0;
	block_size = 0;
	scan_result = 0;
	scan_result_size = 0;
}

MotionUnit::~MotionUnit()
{
	if(block_area) delete [] block_area;
	if(scan_result) delete [] scan_result;
}



int64_t MotionUnit::abs_diff(int32_t *search_pixel,
	int32_t *block_pixel,
	int search_side,
	int block_side)
{
	int64_t result = 0;
	for(int i = 0; i < block_side; i++)
	{
		for(int j = 0; j < block_side; j++)
		{
			int64_t difference;
			difference = search_pixel[j] - block_pixel[j];
			if(difference < 0)
				result -= difference;
			else
				result += difference;
		}
		search_pixel += search_side;
		block_pixel += block_side;
	}
	return result;
}

void MotionUnit::process_package(LoadPackage *package)
{
	MotionPackage *pkg = (MotionPackage*)package;
	macroblock_t *macroblock = pkg->macroblock;
	int w = plugin->current_frame->get_w();
	int h = plugin->current_frame->get_h();
	int search_w = plugin->config.search_radius * 2;
	int search_h = plugin->config.search_radius * 2;
	int block_w = plugin->config.block_size;
	int block_h = plugin->config.block_size; 
	int new_block_size = block_w * OVERSAMPLE * block_h * OVERSAMPLE;


	if(new_block_size != block_size && block_area)
	{
		delete [] block_area;
		block_area = 0;
	}

	if(!block_area)
	{
		block_area = new int32_t[new_block_size];
		block_size = new_block_size;
	}

	int new_result_size = search_w * OVERSAMPLE * search_h * OVERSAMPLE;
	if(new_result_size != scan_result_size && scan_result)
	{
		delete [] scan_result;
		scan_result = 0;
	}
	if(!scan_result)
	{
		scan_result = new int64_t[new_result_size];
		scan_result_size = new_result_size;
	}

	for(int i = 0; i < scan_result_size; i++)
		scan_result[i] = -1;

// Get coordinates in current frame for search data
	int search_x1 = macroblock->x + 
		plugin->config.block_size / 2 - 
		plugin->config.search_radius;
	int search_x2 = search_x1 + 
		search_w - 
		plugin->config.block_size;
	int search_y1 = macroblock->y + 
		plugin->config.block_size / 2 - 
		plugin->config.search_radius;
	int search_y2 = search_y1 + 
		search_h - 
		plugin->config.block_size;
	search_x1 = MAX(search_x1, 0);
	search_x2 = MIN(search_x2, w);
	search_y1 = MAX(search_y1, 0);
	search_y2 = MIN(search_y2, h);
	int search_w_oversampled = (search_x2 - search_x1) * OVERSAMPLE;
	int search_h_oversampled = (search_y2 - search_y1) * OVERSAMPLE;
	int block_side_oversampled = plugin->config.block_size * OVERSAMPLE;
//printf("MotionUnit::process_package 1\n");

// Transfer images to temporaries with oversampling

	MotionMain::oversample(block_area,
		plugin->prev_frame,
		macroblock->x,
		macroblock->x + plugin->config.block_size,
		macroblock->y,
		macroblock->y + plugin->config.block_size,
		block_side_oversampled);


	int64_t least_difference = -1;
	int64_t most_difference = -1;
	int least_x = (macroblock->x - search_x1) * OVERSAMPLE;
	int least_y = (macroblock->y - search_y1) * OVERSAMPLE;



#if 0
// Zigzag motion estimation in the oversampled area.
// Not practical
	for(int i = 0; i < search_h_oversampled - block_side_oversampled; i++)
	{
		for(int j = 0; j < search_w_oversampled - block_side_oversampled; j++)
		{
			int32_t *search_pixel = search_area + 
				i * search_w_oversampled +
				j;
			int32_t *block_pixel = block_area;
			int64_t current_difference = abs_diff(search_pixel, 
				block_pixel,
				search_w_oversampled,
				block_side_oversampled);
			if(least_difference < 0 || current_difference < least_difference)
			{
//printf("MotionUnit::process_package %lld %d %d\n", current_difference, j, i);
				least_difference = current_difference;
				least_x = j;
				least_y = i;
			}
		}
	}
#endif


// Log motion estimation in the oversampled area
// Get motion estimation range
	int scan_x1 = search_x1 * OVERSAMPLE;
	int scan_x2 = search_x2 * OVERSAMPLE - block_side_oversampled;
	int scan_y1 = search_y1 * OVERSAMPLE;
	int scan_y2 = search_y2 * OVERSAMPLE - block_side_oversampled;
#define INIT_SCAN_STEPS 4
	int scan_step_x = block_side_oversampled / INIT_SCAN_STEPS;
	int scan_step_y = block_side_oversampled / INIT_SCAN_STEPS;
	if(scan_step_x > (scan_x2 - scan_x1) / INIT_SCAN_STEPS) scan_step_x = (scan_x2 - scan_x1) / INIT_SCAN_STEPS;
	if(scan_step_y > (scan_y2 - scan_y1) / INIT_SCAN_STEPS) scan_step_y = (scan_y2 - scan_y1) / INIT_SCAN_STEPS;
	scan_step_x = MAX(1, scan_step_x);
	scan_step_y = MAX(1, scan_step_y);

// Do original position first
	int i = macroblock->y * OVERSAMPLE;
	int j = macroblock->x * OVERSAMPLE;
	int32_t *search_pixel = plugin->search_area + 
		i * OVERSAMPLE * w +
		j;
// Top left of macroblock
	int64_t *scan_result_ptr = &scan_result[(i - search_y1 * OVERSAMPLE) * search_w * OVERSAMPLE + j - search_x1 * OVERSAMPLE];
	int32_t *block_pixel = block_area;
	int64_t current_difference;
	least_difference = current_difference = *scan_result_ptr = 
		abs_diff(search_pixel, 
			block_pixel,
			w * OVERSAMPLE,
			block_side_oversampled);
	least_x = j;
	least_y = i;


// Scan logarithmically
	while(1)
	{

//printf("MotionUnit::process_package 20 %d %d\n", scan_step_x, scan_step_y);
// Scan top to bottom
		for(int i = scan_y1; i < scan_y2; i += scan_step_y)
		{
// Scan left to right
			for(int j = scan_x1; j < scan_x2; j += scan_step_x)
			{
// printf("MotionUnit::process_package 10 %d %d %d %d\n", 
// j, 
// i,
// search_w_oversampled - block_side_oversampled,
// search_h_oversampled - block_side_oversampled);
				scan_result_ptr = 
					&scan_result[(i - search_y1 * OVERSAMPLE) * search_w * OVERSAMPLE + j - search_x1 * OVERSAMPLE];
				if(*scan_result_ptr >= 0)
				{
					current_difference = *scan_result_ptr;
				}
				else
				{
// Top left of search area
					search_pixel = plugin->search_area + 
						i * w * OVERSAMPLE +
						j;
// Top left of macroblock
					block_pixel = block_area;
					current_difference = abs_diff(search_pixel, 
						block_pixel,
						w * OVERSAMPLE,
						block_side_oversampled);
					*scan_result_ptr = current_difference;
				}

				if(least_difference < 0 || current_difference < least_difference)
				{
// printf("%d, %d %lld\n", 
// j - macroblock->x * OVERSAMPLE, 
// i - macroblock->y * OVERSAMPLE, 
// current_difference);
					least_difference = current_difference;
					least_x = j;
					least_y = i;
				}

				if(most_difference < 0 || current_difference > most_difference)
					most_difference = current_difference;
			}
		}

		if(scan_step_x == 1 && scan_step_y == 1) break;

		scan_step_x >>= 1;
		scan_step_y >>= 1;
		scan_step_x = MAX(scan_step_x, 1);
		scan_step_y = MAX(scan_step_y, 1);

// Reduce scan range around hot zone
		int new_x_range = (scan_x2 - scan_x1) >> 1;
		int new_y_range = (scan_y2 - scan_y1) >> 1;

		new_x_range = MAX(new_x_range, 1);
		new_y_range = MAX(new_y_range, 1);
		scan_x1 = least_x - new_x_range / 2;
		scan_x2 = scan_x1 + new_x_range;
		scan_y1 = least_y - new_y_range / 2;
		scan_y2 = scan_y1 + new_y_range;
		CLAMP(scan_x1, search_x1 * OVERSAMPLE, search_x2 * OVERSAMPLE - block_side_oversampled);
		CLAMP(scan_y1, search_y1 * OVERSAMPLE, search_y2 * OVERSAMPLE - block_side_oversampled);
		CLAMP(scan_x2, search_x1 * OVERSAMPLE, search_x2 * OVERSAMPLE - block_side_oversampled);
		CLAMP(scan_y2, search_y1 * OVERSAMPLE, search_y2 * OVERSAMPLE - block_side_oversampled);
	}
//printf("MotionUnit::process_package 50\n");


	if(least_difference < most_difference / 4 && 
		most_difference > block_side_oversampled * block_side_oversampled)
	{
		macroblock->dx_oversampled = 
			least_x - 
			macroblock->x * OVERSAMPLE;
		macroblock->dy_oversampled = 
			least_y - 
			macroblock->y * OVERSAMPLE;
		macroblock->valid = 1;
	}
	else
	{
		macroblock->dx_oversampled = 0;
		macroblock->dy_oversampled = 0;
	}
// printf("MotionUnit::process_package 100 %lld %lld %d %d %d %d\n",
// least_difference,
// most_difference,
// search_x1,
// search_x2,
// search_y1,
// search_y2);

}






MotionEngine::MotionEngine(MotionMain *plugin, 
	int total_clients,
	int total_packages)
 : LoadServer(
//1, 1 
total_clients, total_packages 
)
{
	this->plugin = plugin;
}

void MotionEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		MotionPackage *package = (MotionPackage*)get_package(i);
		package->macroblock = &plugin->macroblocks[i];
	}
}

LoadClient* MotionEngine::new_client()
{
	return new MotionUnit(this, plugin);
}

LoadPackage* MotionEngine::new_package()
{
	return new MotionPackage;
}





