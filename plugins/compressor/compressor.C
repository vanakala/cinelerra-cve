#include "bcdisplayinfo.h"
#include "clip.h"
#include "compressor.h"
#include "cursors.h"
#include "defaults.h"
#include "filexml.h"
#include "picon_png.h"
#include "units.h"
#include "vframe.h"

#include <math.h>
#include <string.h>





REGISTER_PLUGIN(CompressorEffect)








CompressorEffect::CompressorEffect(PluginServer *server)
 : PluginAClient(server)
{
	reset();
	PLUGIN_CONSTRUCTOR_MACRO
}

CompressorEffect::~CompressorEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
	delete_dsp();
}

void CompressorEffect::delete_dsp()
{
	if(input_buffer)
	{
		for(int i = 0; i < PluginClient::total_in_buffers; i++)
			delete [] input_buffer[i];
		delete [] input_buffer;
	}
	if(coefs) delete [] coefs;

	if(reaction_buffer) delete [] reaction_buffer;

	input_buffer = 0;
	coefs = 0;
	input_size = 0;
	input_allocated = 0;
	reaction_buffer = 0;
	reaction_allocated = 0;
	reaction_position = 0;
}


void CompressorEffect::reset()
{
	input_buffer = 0;
	coefs = 0;
	input_size = 0;
	input_allocated = 0;
	coefs_allocated = 0;
	reaction_buffer = 0;
	reaction_allocated = 0;
	reaction_position = 0;
	current_coef = 1.0;
	last_peak_age = 0;
	last_peak = 0.0;
	previous_intercept = 1.0;
	previous_slope = 0.0;
	previous_max = 0.0;
	max_counter = 0;
}

char* CompressorEffect::plugin_title()
{
	return "Compressor";
}


int CompressorEffect::is_realtime()
{
	return 1;
}

int CompressorEffect::is_multichannel()
{
	return 1;
}






void CompressorEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	config.levels.remove_all();
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("COMPRESSOR"))
			{
				config.preview_len = input.tag.get_property("PREVIEW_LEN", config.preview_len);
				config.reaction_len = input.tag.get_property("REACTION_LEN", config.reaction_len);
				config.trigger = input.tag.get_property("TRIGGER", config.trigger);
			}
			else
			if(input.tag.title_is("LEVEL"))
			{
				double x = input.tag.get_property("X", (double)0);
				double y = input.tag.get_property("Y", (double)0);
				compressor_point_t point = { x, y };

				config.levels.append(point);
			}
		}
	}
}

void CompressorEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);

	output.tag.set_title("COMPRESSOR");
	output.tag.set_property("PREVIEW_LEN", config.preview_len);
	output.tag.set_property("TRIGGER", config.trigger);
	output.tag.set_property("REACTION_LEN", config.reaction_len);
	output.append_tag();
	output.append_newline();


	for(int i = 0; i < config.levels.total; i++)
	{
		output.tag.set_title("LEVEL");
		output.tag.set_property("X", config.levels.values[i].x);
		output.tag.set_property("Y", config.levels.values[i].y);

		output.append_tag();
		output.append_newline();
	}

	output.terminate_string();
}

int CompressorEffect::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
	sprintf(directory, "%scompression.rc", BCASTDIR);
	defaults = new Defaults(directory);
	defaults->load();

	config.preview_len = defaults->get("PREVIEW_LEN", config.preview_len);
	config.trigger = defaults->get("TRIGGER", config.trigger);
	config.reaction_len = defaults->get("REACTION_LEN", config.reaction_len);

	config.levels.remove_all();
	int total_levels = defaults->get("TOTAL_LEVELS", 0);
	for(int i = 0; i < total_levels; i++)
	{
		config.levels.append();
		sprintf(string, "X_%d", i);
		config.levels.values[i].x = defaults->get(string, (double)0);
		sprintf(string, "Y_%d", i);
		config.levels.values[i].y = defaults->get(string, (double)0);
	}
//config.dump();
	return 0;
}

int CompressorEffect::save_defaults()
{
	char string[BCTEXTLEN];

	defaults->update("PREVIEW_LEN", config.preview_len);
	defaults->update("TRIGGER", config.trigger);
	defaults->update("REACTION_LEN", config.reaction_len);
	defaults->update("TOTAL_LEVELS", config.levels.total);

	defaults->update("TOTAL_LEVELS", config.levels.total);
	for(int i = 0; i < config.levels.total; i++)
	{
		sprintf(string, "X_%d", i);
		defaults->update(string, config.levels.values[i].x);
		sprintf(string, "Y_%d", i);
		defaults->update(string, config.levels.values[i].y);
	}

	defaults->save();

	return 0;
}


void CompressorEffect::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->update();
		thread->window->unlock_window();
	}
}


NEW_PICON_MACRO(CompressorEffect)
SHOW_GUI_MACRO(CompressorEffect, CompressorThread)
RAISE_WINDOW_MACRO(CompressorEffect)
SET_STRING_MACRO(CompressorEffect)
LOAD_CONFIGURATION_MACRO(CompressorEffect, CompressorConfig)



int CompressorEffect::process_realtime(int64_t size, double **input_ptr, double **output_ptr)
{
//printf("CompressorEffect::process_realtime 1 %f\n", DB::fromdb(-100));
	load_configuration();

	if(coefs_allocated < size)
	{
		if(coefs) delete [] coefs;
		coefs = new double[size];
		coefs_allocated = size;
	}

//printf("CompressorEffect::process_realtime 2\n");
	int preview_samples = (int)(config.preview_len * PluginAClient::project_sample_rate + 0.5);
	int reaction_samples = (int)(config.reaction_len * PluginAClient::project_sample_rate + 0.5);
	int trigger = CLIP(config.trigger, 0, PluginAClient::total_in_buffers - 1);
//	trigger = PluginAClient::total_in_buffers - trigger - 1;
	CLAMP(reaction_samples, 1, 128000000);
	CLAMP(preview_samples, 1, 128000000);

//printf("CompressorEffect::process_realtime 3\n");


// Append input buffers
	if(input_size + size > input_allocated)
	{
		double **new_input = new double*[PluginClient::total_in_buffers];
		for(int i = 0; i < PluginClient::total_in_buffers; i++)
			new_input[i] = new double[input_size + size];

		if(input_buffer)
		{
			for(int i = 0; i < PluginClient::total_in_buffers; i++)
			{
				memcpy(new_input[i], input_buffer[i], sizeof(double) * input_size);
				delete [] input_buffer[i];
			}

			delete [] input_buffer;
		}

		input_buffer = new_input;
		input_allocated = input_size + size;
	}

//printf("CompressorEffect::process_realtime 4\n");
	for(int i = 0; i < PluginClient::total_in_buffers; i++)
		memcpy(input_buffer[i] + input_size, 
			input_ptr[i], 
			size * sizeof(double));
	input_size += size;



//printf("CompressorEffect::process_realtime 5 %d\n", size);




// Have enough to send to output
	if(input_size >= preview_samples)
	{
		int output_offset = 0;
		if(input_size - preview_samples < size)
		{
			output_offset = size - (input_size - preview_samples);
			size = input_size - preview_samples;
			for(int i = 0; i < PluginAClient::total_in_buffers; i++)
				bzero(output_ptr[i], output_offset * sizeof(double));
		}
			
//printf("CompressorEffect::process_realtime 6 %d\n", reaction_samples);
		if(reaction_allocated != reaction_samples)
		{
			double *new_buffer = new double[reaction_samples];
			if(reaction_buffer)
			{
				memcpy(new_buffer, 
					reaction_buffer, 
					sizeof(double) * MIN(reaction_allocated, reaction_samples));
				delete [] reaction_buffer;
			}
			if(reaction_samples - reaction_allocated > 0)
				bzero(new_buffer + reaction_allocated, 
					sizeof(double) * (reaction_samples - reaction_allocated));
			reaction_buffer = new_buffer;
			reaction_allocated = reaction_samples;
			if(reaction_position >= reaction_allocated) reaction_position = 0;
		}


//printf("CompressorEffect::process_realtime 7\n");
// Calculate coef buffer for current size
		double max = 0, min = 0;
		int64_t max_age = 0;
		int64_t min_age = 0;
		for(int i = 0; i < size; i++)
		{
// Put new sample in reaction buffer
			reaction_buffer[reaction_position] = 
				input_buffer[trigger][i + preview_samples];




// Get peak in last reaction buffer size of samples
			max = 0;
			min = 0;
// Try to optimize
			if(last_peak_age < reaction_samples)
			{
				if(fabs(input_buffer[trigger][i + preview_samples]) > last_peak)
				{
					last_peak = fabs(input_buffer[trigger][i + preview_samples]);
					last_peak_age = 0;
				}
				max = last_peak;
			}
			else
			{
// Rescan history
				for(int j = 0; j < reaction_samples; j++)
				{
					if(reaction_buffer[j] > max)
					{
						max = reaction_buffer[j];
						max_age = (j <= reaction_position) ? 
							(reaction_position - j) :
							(reaction_samples - (j - reaction_position));
					}
					else
					if(reaction_buffer[j] < min)
					{
						min = reaction_buffer[j];
						min_age = (j <= reaction_position) ? 
							(reaction_position - j) :
							(reaction_samples - (j - reaction_position));
					}
				}

				if(-min > max)
				{
					max = -min;
					max_age = min_age;
				}

				last_peak = max;
				last_peak_age = max_age;
			}

// Here's the brain of the effect.

// Test expiration of previous max.  If previous max is bigger than
// current max and still valid, it replaces the current max.
// Otherwise, the current max becomes the previous max and the counter
// is reset.

			if(max_counter > 0 && previous_max >= max)
			{
				;
			}
			else
			if(max > 0.00001)
			{
// Get new slope based on current coef, peak, and reaction len.
// The slope has a counter which needs to expire before it can be
// replaced with a less steep value.
				double x_db = DB::todb(max);
				double y_db = config.calculate_db(x_db);
				double y_linear = DB::fromdb(y_db);
				double new_coef = y_linear / max;
				double slope = (new_coef - current_coef) / reaction_samples;
				previous_slope = slope;
				previous_max = max;
				previous_intercept = current_coef;
				max_counter = reaction_samples;
			}
			else
			{
				previous_slope = 0.0;
				previous_intercept = current_coef;
				max_counter = 0;
			}

//printf("%f %f %f %d\n", current_coef, previous_slope, previous_intercept, max_counter);
			max_counter--;
			current_coef = previous_intercept + 
				previous_slope * (reaction_samples - max_counter);
			coefs[i] = current_coef;

			last_peak_age++;
			reaction_position++;
			if(reaction_position >= reaction_allocated)
				reaction_position = 0;
		}

//printf("CompressorEffect::process_realtime 8 %f %f\n", input_buffer[0][0], coefs[0]);
// Multiply coef buffer by input buffer
		for(int i = 0; i < PluginAClient::total_in_buffers; i++)
		{
			for(int j = 0; j < size; j++)
			{
				output_ptr[i][j + output_offset] = input_buffer[i][j] * coefs[j];
			}
		}

//printf("CompressorEffect::process_realtime 9 %d\n", PluginAClient::total_in_buffers);
// Shift input forward
		for(int i = 0; i < PluginAClient::total_in_buffers; i++)
		{
			for(int j = 0, k = size; k < input_size; j++, k++)
			{
				input_buffer[i][j] = input_buffer[i][k];
			}
		}
//printf("CompressorEffect::process_realtime 10\n");
		input_size -= size;
	}
	else
	{
		for(int i = 0; i < PluginAClient::total_in_buffers; i++)
			bzero(output_ptr[i], sizeof(double) * size);
	}

//printf("CompressorEffect::process_realtime 11\n");

	return 0;
}













CompressorConfig::CompressorConfig()
{
	reaction_len = 1.0;
	preview_len = 1.0;
	min_db = -80.0;
	min_x = min_db;
	min_y = min_db;
	max_x = 0;
	max_y = 0;
	trigger = 0;
}

void CompressorConfig::copy_from(CompressorConfig &that)
{
	this->reaction_len = that.reaction_len;
	this->preview_len = that.preview_len;
	this->min_db = that.min_db;
	this->min_x = that.min_x;
	this->min_y = that.min_y;
	this->max_x = that.max_x;
	this->max_y = that.max_y;
	this->trigger = that.trigger;
	levels.remove_all();
	for(int i = 0; i < that.levels.total; i++)
		this->levels.append(that.levels.values[i]);
}

int CompressorConfig::equivalent(CompressorConfig &that)
{
	return 0;
}

void CompressorConfig::interpolate(CompressorConfig &prev, 
	CompressorConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	copy_from(prev);
}

int CompressorConfig::total_points()
{
	if(!levels.total) 
		return 1;
	else
		return levels.total;
}

void CompressorConfig::dump()
{
	printf("CompressorConfig::dump\n");
	for(int i = 0; i < levels.total; i++)
	{
		printf("	%f %f\n", levels.values[i].x, levels.values[i].y);
	}
}


double CompressorConfig::get_y(int number)
{
	if(!levels.total) 
		return 1.0;
	else
	if(number >= levels.total)
		return levels.values[levels.total - 1].y;
	else
		return levels.values[number].y;
}

double CompressorConfig::get_x(int number)
{
	if(!levels.total)
		return 0.0;
	else
	if(number >= levels.total)
		return levels.values[levels.total - 1].x;
	else
		return levels.values[number].x;
}

// Returns linear output given linear input
double CompressorConfig::calculate_db(double x)
{
	if(x > -0.001) return 0.0;

	for(int i = levels.total - 1; i >= 0; i--)
	{
		if(levels.values[i].x <= x)
		{
			if(i < levels.total - 1)
			{
				return levels.values[i].y + 
					(x - levels.values[i].x) *
					(levels.values[i + 1].y - levels.values[i].y) / 
					(levels.values[i + 1].x - levels.values[i].x);
			}
			else
			{
				return levels.values[i].y +
					(x - levels.values[i].x) * 
					(max_y - levels.values[i].y) / 
					(max_x - levels.values[i].x);
			}
		}
	}

	if(levels.total)
	{
		return min_y + 
			(x - min_x) * 
			(levels.values[0].y - min_y) / 
			(levels.values[0].x - min_x);
	}
	else
		return x;
}


int CompressorConfig::set_point(double x, double y)
{
	for(int i = levels.total - 1; i >= 0; i--)
	{
		if(levels.values[i].x < x)
		{
			levels.append();
			i++;
			for(int j = levels.total - 2; j >= i; j--)
			{
				levels.values[j + 1] = levels.values[j];
			}
			levels.values[i].x = x;
			levels.values[i].y = y;

			return i;
		}
	}

	levels.append();
	for(int j = levels.total - 2; j >= 0; j--)
	{
		levels.values[j + 1] = levels.values[j];
	}
	levels.values[0].x = x;
	levels.values[0].y = y;
	return 0;
}

void CompressorConfig::remove_point(int number)
{
	for(int j = number; j < levels.total - 1; j++)
	{
		levels.values[j] = levels.values[j + 1];
	}
	levels.remove();
}

void CompressorConfig::optimize()
{
	int done = 0;
	
	while(!done)
	{
		done = 1;
		
		
		for(int i = 0; i < levels.total - 1; i++)
		{
			if(levels.values[i].x >= levels.values[i + 1].x)
			{
				done = 0;
				for(int j = i + 1; j < levels.total - 1; j++)
				{
					levels.values[j] = levels.values[j + 1];
				}
				levels.remove();
			}
		}
		
	}
}














PLUGIN_THREAD_OBJECT(CompressorEffect, CompressorThread, CompressorWindow)













CompressorWindow::CompressorWindow(CompressorEffect *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	640, 
	480, 
	640, 
	480,
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

void CompressorWindow::create_objects()
{
	int x = 35, y = 10;

	add_subwindow(canvas = new CompressorCanvas(plugin, 
		x, 
		y, 
		get_w() - x - 120, 
		get_h() - y - 70));
	canvas->set_cursor(CROSS_CURSOR);
	x = get_w() - 110;
	add_subwindow(new BC_Title(x, y, "Preview secs:"));
	y += 20;
	add_subwindow(preview = new CompressorPreview(plugin, x, y));
	y += 30;
	add_subwindow(new BC_Title(x, y, "Reaction secs:"));
	y += 20;
	add_subwindow(reaction = new CompressorReaction(plugin, x, y));
	y += 30;
	add_subwindow(new BC_Title(x, y, "Trigger:"));
	y += 20;
	add_subwindow(trigger = new CompressorTrigger(plugin, x, y));
	y += 30;
	add_subwindow(clear = new CompressorClear(plugin, x, y));
	x = 10;
	y = get_h() - 40;
	add_subwindow(new BC_Title(x, y, "Point:"));
	x += 50;
	add_subwindow(x_text = new CompressorX(plugin, x, y));
	x += 110;
	add_subwindow(new BC_Title(x, y, "x"));
	x += 20;
	add_subwindow(y_text = new CompressorY(plugin, x, y));
	draw_scales();

	update_canvas();
	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(CompressorWindow)

void CompressorWindow::draw_scales()
{
	set_font(SMALLFONT);
	set_color(BLACK);

#define DIVISIONS 8
	for(int i = 0; i <= DIVISIONS; i++)
	{
		int y = canvas->get_y() + 10 + canvas->get_h() / DIVISIONS * i;
		int x = canvas->get_x() - 30;
		char string[BCTEXTLEN];
		
		sprintf(string, "%.0f", (float)i / DIVISIONS * plugin->config.min_db);
		draw_text(x, y, string);
		
		int y1 = canvas->get_y() + canvas->get_h() / DIVISIONS * i;
		int y2 = canvas->get_y() + canvas->get_h() / DIVISIONS * (i + 1);
		for(int j = 0; j < 10; j++)
		{
			y = y1 + (y2 - y1) * j / 10;
			if(j == 0)
			{
				draw_line(canvas->get_x() - 10, y, canvas->get_x(), y);
			}
			else
			if(i < DIVISIONS)
			{
				draw_line(canvas->get_x() - 5, y, canvas->get_x(), y);
			}
		}
	}



	for(int i = 0; i <= DIVISIONS; i++)
	{
		int y = canvas->get_h() + 30;
		int x = canvas->get_x() + (canvas->get_w() - 10) / DIVISIONS * i;
		char string[BCTEXTLEN];

		sprintf(string, "%.0f", (1.0 - (float)i / DIVISIONS) * plugin->config.min_db);
		draw_text(x, y, string);

		int x1 = canvas->get_x() + canvas->get_w() / DIVISIONS * i;
		int x2 = canvas->get_x() + canvas->get_w() / DIVISIONS * (i + 1);
		for(int j = 0; j < 10; j++)
		{
			x = x1 + (x2 - x1) * j / 10;
			if(j == 0)
			{
				draw_line(x, canvas->get_y() + canvas->get_h(), x, canvas->get_y() + canvas->get_h() + 10);
			}
			else
			if(i < DIVISIONS)
			{
				draw_line(x, canvas->get_y() + canvas->get_h(), x, canvas->get_y() + canvas->get_h() + 5);
			}
		}
	}



	flash();
}

void CompressorWindow::update()
{
	update_textboxes();
	update_canvas();
}

void CompressorWindow::update_textboxes()
{
	preview->update((float)plugin->config.preview_len);
	trigger->update((int64_t)plugin->config.trigger);
	reaction->update((float)plugin->config.reaction_len);
	if(canvas->current_operation == CompressorCanvas::DRAG)
	{
		x_text->update((float)plugin->config.levels.values[canvas->current_point].x);
		y_text->update((float)plugin->config.levels.values[canvas->current_point].y);
	}
}

#define POINT_W 6
void CompressorWindow::update_canvas()
{
	int y1, y2;


	canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
	canvas->set_color(GREEN);
	for(int i = 1; i < DIVISIONS; i++)
	{
		int y = canvas->get_h() * i / DIVISIONS;
		canvas->draw_line(0, y, canvas->get_w(), y);
		
		int x = canvas->get_w() * i / DIVISIONS;
		canvas->draw_line(x, 0, x, canvas->get_h());
	}
	
	
	
	canvas->set_color(BLACK);
	for(int i = 0; i < canvas->get_w(); i++)
	{
		double x_db = ((double)1 - (double)i / canvas->get_w()) * plugin->config.min_db;
		double y_db = plugin->config.calculate_db(x_db);
		y2 = (int)(y_db / plugin->config.min_db * canvas->get_h());

		if(i > 0)
		{
			canvas->draw_line(i - 1, y1, i, y2);
		}

		y1 = y2;
	}

	int total = plugin->config.levels.total ? plugin->config.levels.total : 1;
	for(int i = 0; i < plugin->config.levels.total; i++)
	{
		double x_db = plugin->config.get_x(i);
		double y_db = plugin->config.get_y(i);

		int x = (int)(((double)1 - x_db / plugin->config.min_db) * canvas->get_w());
		int y = (int)(y_db / plugin->config.min_db * canvas->get_h());
		
		canvas->draw_box(x - POINT_W / 2, y - POINT_W / 2, POINT_W, POINT_W);
	}
	
	canvas->flash();
	canvas->flush();
}








CompressorCanvas::CompressorCanvas(CompressorEffect *plugin, int x, int y, int w, int h) 
 : BC_SubWindow(x, y, w, h, WHITE)
{
	this->plugin = plugin;
}

int CompressorCanvas::button_press_event()
{
// Check existing points
	if(is_event_win() && cursor_inside())
	{
		for(int i = 0; i < plugin->config.levels.total; i++)
		{
			double x_db = plugin->config.get_x(i);
			double y_db = plugin->config.get_y(i);

			int x = (int)(((double)1 - x_db / plugin->config.min_db) * get_w());
			int y = (int)(y_db / plugin->config.min_db * get_h());

			if(get_cursor_x() < x + POINT_W / 2 && get_cursor_x() >= x - POINT_W / 2 &&
				get_cursor_y() < y + POINT_W / 2 && get_cursor_y() >= y - POINT_W / 2)
			{
				current_operation = DRAG;
				current_point = i;
				return 1;
			}
		}



// Create new point
		double x_db = (double)(1 - (double)get_cursor_x() / get_w()) * plugin->config.min_db;
		double y_db = (double)get_cursor_y() / get_h() * plugin->config.min_db;

		current_point = plugin->config.set_point(x_db, y_db);
		current_operation = DRAG;
		plugin->thread->window->update();
		plugin->send_configure_change();
		return 1;
	}
	return 0;
//plugin->config.dump();
}

int CompressorCanvas::button_release_event()
{
	if(current_operation == DRAG)
	{
		if(current_point > 0)
		{
			if(plugin->config.levels.values[current_point].x <
				plugin->config.levels.values[current_point - 1].x)
				plugin->config.remove_point(current_point);
		}

		if(current_point < plugin->config.levels.total - 1)
		{
			if(plugin->config.levels.values[current_point].x >=
				plugin->config.levels.values[current_point + 1].x)
				plugin->config.remove_point(current_point);
		}

		plugin->thread->window->update();
		plugin->send_configure_change();
		current_operation = NONE;
		return 1;
	}

	return 0;
}

int CompressorCanvas::cursor_motion_event()
{
	if(current_operation == DRAG)
	{
		int x = get_cursor_x();
		int y = get_cursor_y();
		CLAMP(x, 0, get_w());
		CLAMP(y, 0, get_h());
		double x_db = (double)(1 - (double)x / get_w()) * plugin->config.min_db;
		double y_db = (double)y / get_h() * plugin->config.min_db;
		plugin->config.levels.values[current_point].x = x_db;
		plugin->config.levels.values[current_point].y = y_db;
		plugin->thread->window->update();
		plugin->send_configure_change();
		return 1;
//plugin->config.dump();
	}
	return 0;
}



CompressorPreview::CompressorPreview(CompressorEffect *plugin, int x, int y) 
 : BC_TextBox(x, y, 100, 1, (float)plugin->config.preview_len)
{
	this->plugin = plugin;
}

int CompressorPreview::handle_event()
{
	plugin->config.preview_len = atof(get_text());
	plugin->send_configure_change();
	return 1;
}



CompressorReaction::CompressorReaction(CompressorEffect *plugin, int x, int y) 
 : BC_TextBox(x, y, 100, 1, (float)plugin->config.reaction_len)
{
	this->plugin = plugin;
}

int CompressorReaction::handle_event()
{
//printf("CompressorReaction::handle_event 1\n");
	plugin->config.reaction_len = atof(get_text());
	plugin->send_configure_change();
//printf("CompressorReaction::handle_event 2\n");
	return 1;
}


CompressorX::CompressorX(CompressorEffect *plugin, int x, int y) 
 : BC_TextBox(x, y, 100, 1, "")
{
	this->plugin = plugin;
}
int CompressorX::handle_event()
{
	int current_point = plugin->thread->window->canvas->current_point;
	if(current_point < plugin->config.levels.total)
	{
		plugin->config.levels.values[current_point].x = atof(get_text());
		plugin->thread->window->update_canvas();
		plugin->send_configure_change();
	}
	return 1;
}



CompressorY::CompressorY(CompressorEffect *plugin, int x, int y) 
 : BC_TextBox(x, y, 100, 1, "")
{
	this->plugin = plugin;
}
int CompressorY::handle_event()
{
	int current_point = plugin->thread->window->canvas->current_point;
	if(current_point < plugin->config.levels.total)
	{
		plugin->config.levels.values[current_point].y = atof(get_text());
		plugin->thread->window->update_canvas();
		plugin->send_configure_change();
	}
	return 1;
}


CompressorTrigger::CompressorTrigger(CompressorEffect *plugin, int x, int y) 
 : BC_TextBox(x, y, (int64_t)100, (int64_t)1, (int64_t)plugin->config.trigger)
{
	this->plugin = plugin;
}
int CompressorTrigger::handle_event()
{
	plugin->config.trigger = atol(get_text());
	plugin->send_configure_change();
	return 1;
}





CompressorClear::CompressorClear(CompressorEffect *plugin, int x, int y) 
 : BC_GenericButton(x, y, "Clear")
{
	this->plugin = plugin;
}

int CompressorClear::handle_event()
{
	plugin->config.levels.remove_all();
//plugin->config.dump();
	plugin->thread->window->update();
	plugin->send_configure_change();
	return 1;
}




