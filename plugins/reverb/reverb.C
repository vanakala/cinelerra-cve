#include "clip.h"
#include "confirmsave.h"
#include "defaults.h"
#include "errorbox.h"
#include "filexml.h"
#include "picon_png.h"
#include "reverb.h"
#include "reverbwindow.h"

#include "vframe.h"

#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>



PluginClient* new_plugin(PluginServer *server)
{
	return new Reverb(server);
}



Reverb::Reverb(PluginServer *server)
 : PluginAClient(server)
{
	srand(time(0));
	redo_buffers = 1;       // set to redo buffers before the first render
	dsp_in_length = 0;
	ref_channels = 0;
	ref_offsets = 0;
	ref_levels = 0;
	ref_lowpass = 0;
	dsp_in = 0;
	lowpass_in1 = 0;
	lowpass_in2 = 0;
	initialized = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

Reverb::~Reverb()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(initialized)
	{
		for(int i = 0; i < total_in_buffers; i++)
		{
			delete [] dsp_in[i];
			delete [] ref_channels[i];
			delete [] ref_offsets[i];
			delete [] ref_levels[i];
			delete [] ref_lowpass[i];
			delete [] lowpass_in1[i];
			delete [] lowpass_in2[i];
		}

		delete [] dsp_in;
		delete [] ref_channels;
		delete [] ref_offsets;
		delete [] ref_levels;
		delete [] ref_lowpass;
		delete [] lowpass_in1;
		delete [] lowpass_in2;

		for(int i = 0; i < (smp + 1); i++)
		{
			delete engine[i];
		}
		delete [] engine;
		initialized = 0;
	}
}

char* Reverb::plugin_title() { return "Heroine College Concert Hall"; }
int Reverb::is_realtime() { return 1; }
int Reverb::is_multichannel() { return 1; }

int Reverb::process_realtime(int64_t size, double **input_ptr, double **output_ptr)
{
	int64_t new_dsp_length, i, j;
	main_in = input_ptr;
	main_out = output_ptr;
//printf("Reverb::process_realtime 1\n");
	redo_buffers |= load_configuration();

//printf("Reverb::process_realtime 1\n");
	if(!config.ref_total) return 0;


	if(!initialized)
	{
		dsp_in = new double*[total_in_buffers];
		ref_channels = new int64_t*[total_in_buffers];
		ref_offsets = new int64_t*[total_in_buffers];
		ref_levels = new double*[total_in_buffers];
		ref_lowpass = new int64_t*[total_in_buffers];
		lowpass_in1 = new double*[total_in_buffers];
		lowpass_in2 = new double*[total_in_buffers];

		for(i = 0; i < total_in_buffers; i++)
		{
			dsp_in[i] = new double[1];
			ref_channels[i] = new int64_t[1];
			ref_offsets[i] = new int64_t[1];
			ref_levels[i] = new double[1];
			ref_lowpass[i] = new int64_t[1];
			lowpass_in1[i] = new double[1];
			lowpass_in2[i] = new double[1];
		}

		engine = new ReverbEngine*[(smp + 1)];
		for(i = 0; i < (smp + 1); i++)
		{
			engine[i] = new ReverbEngine(this);
//printf("Reverb::start_realtime %d\n", Thread::calculate_realtime());
// Realtime priority moved to sound driver
//		engine[i]->set_realtime(realtime_priority);
			engine[i]->start();
		}
		initialized = 1;
		redo_buffers = 1;
	}

	new_dsp_length = size + 
		(config.delay_init + config.ref_length) * project_sample_rate / 1000 + 1;
//printf("Reverb::process_realtime 1 %d %d\n", in_buffer_size, size);

	if(redo_buffers || new_dsp_length != dsp_in_length)
	{
		for(i = 0; i < total_in_buffers; i++)
		{
			double *old_dsp = dsp_in[i];
			double *new_dsp = new double[new_dsp_length];
			for(j = 0; j < dsp_in_length && j < new_dsp_length; j++) 
				new_dsp[j] = old_dsp[j];
			for( ; j < new_dsp_length; j++) new_dsp[j] = 0;
			delete [] old_dsp;
			dsp_in[i] = new_dsp;
		}

		dsp_in_length = new_dsp_length;
		redo_buffers = 1;
	}
//printf("Reverb::process_realtime 1\n");

	if(redo_buffers)
	{
		for(i = 0; i < total_in_buffers; i++)
		{
			delete [] ref_channels[i];
			delete [] ref_offsets[i];
			delete [] ref_lowpass[i];
			delete [] ref_levels[i];
			delete [] lowpass_in1[i];
			delete [] lowpass_in2[i];
			
			ref_channels[i] = new int64_t[config.ref_total + 1];
			ref_offsets[i] = new int64_t[config.ref_total + 1];
			ref_lowpass[i] = new int64_t[config.ref_total + 1];
			ref_levels[i] = new double[config.ref_total + 1];
			lowpass_in1[i] = new double[config.ref_total + 1];
			lowpass_in2[i] = new double[config.ref_total + 1];

// set channels			
			ref_channels[i][0] = i;         // primary noise
			ref_channels[i][1] = i;         // first reflection
// set offsets
			ref_offsets[i][0] = 0;
			ref_offsets[i][1] = config.delay_init * project_sample_rate / 1000;
// set levels
			ref_levels[i][0] = db.fromdb(config.level_init);
			ref_levels[i][1] = db.fromdb(config.ref_level1);
// set lowpass
			ref_lowpass[i][0] = -1;     // ignore first noise
			ref_lowpass[i][1] = config.lowpass1;
			lowpass_in1[i][0] = 0;
			lowpass_in2[i][0] = 0;
			lowpass_in1[i][1] = 0;
			lowpass_in2[i][1] = 0;

			int64_t ref_division = config.ref_length * project_sample_rate / 1000 / (config.ref_total + 1);
			for(j = 2; j < config.ref_total + 1; j++)
			{
// set random channels for remaining reflections
				ref_channels[i][j] = rand() % total_in_buffers;

// set random offsets after first reflection
				ref_offsets[i][j] = ref_offsets[i][1];
				ref_offsets[i][j] += ref_division * j - (rand() % ref_division) / 2;

// set changing levels
				ref_levels[i][j] = db.fromdb(config.ref_level1 + (config.ref_level2 - config.ref_level1) / (config.ref_total - 1) * (j - 2));
				//ref_levels[i][j] /= 100;

// set changing lowpass as linear
				ref_lowpass[i][j] = (int64_t)(config.lowpass1 + (double)(config.lowpass2 - config.lowpass1) / (config.ref_total - 1) * (j - 2));
				lowpass_in1[i][j] = 0;
				lowpass_in2[i][j] = 0;
			}
		}
		
		redo_buffers = 0;
	}
//printf("Reverb::process_realtime 1\n");

	for(i = 0; i < total_in_buffers; )
	{
		for(j = 0; j < (smp + 1) && (i + j) < total_in_buffers; j++)
		{
			engine[j]->process_overlays(i + j, size);
		}
		
		for(j = 0; j < (smp + 1) && i < total_in_buffers; j++, i++)
		{
			engine[j]->wait_process_overlays();
		}
	}
//printf("Reverb::process_realtime 2 %d %d\n", total_in_buffers, size);

	for(i = 0; i < total_in_buffers; i++)
	{
		double *current_out = main_out[i];
		double *current_in = dsp_in[i];

		for(j = 0; j < size; j++) current_out[j] = current_in[j];

		int64_t k;
		for(k = 0; j < dsp_in_length; j++, k++) current_in[k] = current_in[j];
		
		for(; k < dsp_in_length; k++) current_in[k] = 0;
	}
//printf("Reverb::process_realtime 2 %d %d\n", total_in_buffers, size);
	return 0;
}

NEW_PICON_MACRO(Reverb)

SHOW_GUI_MACRO(Reverb, ReverbThread)

SET_STRING_MACRO(Reverb)

RAISE_WINDOW_MACRO(Reverb)

int Reverb::load_defaults()
{
	char directory[1024];

// set the default directory
	sprintf(directory, "%sreverb.rc", get_defaultdir());

// load the defaults

	defaults = new Defaults(directory);

	defaults->load();

	config.level_init = defaults->get("LEVEL_INIT", (double)0);
	config.delay_init = defaults->get("DELAY_INIT", 100);
	config.ref_level1 = defaults->get("REF_LEVEL1", (double)-6);
	config.ref_level2 = defaults->get("REF_LEVEL2", (double)INFINITYGAIN);
	config.ref_total = defaults->get("REF_TOTAL", 12);
	config.ref_length = defaults->get("REF_LENGTH", 1000);
	config.lowpass1 = defaults->get("LOWPASS1", 20000);
	config.lowpass2 = defaults->get("LOWPASS2", 20000);

	sprintf(config_directory, "~");
	defaults->get("CONFIG_DIRECTORY", config_directory);

//printf("Reverb::load_defaults config.ref_level2 %f\n", config.ref_level2);
	return 0;
}

int Reverb::save_defaults()
{
	defaults->update("LEVEL_INIT", config.level_init);
	defaults->update("DELAY_INIT", config.delay_init);
	defaults->update("REF_LEVEL1", config.ref_level1);
	defaults->update("REF_LEVEL2", config.ref_level2);
	defaults->update("REF_TOTAL", config.ref_total);
	defaults->update("REF_LENGTH", config.ref_length);
	defaults->update("LOWPASS1", config.lowpass1);
	defaults->update("LOWPASS2", config.lowpass2);
	defaults->update("CONFIG_DIRECTORY", config_directory);
	defaults->save();
	return 0;
}

LOAD_CONFIGURATION_MACRO(Reverb, ReverbConfig)


void Reverb::save_data(KeyFrame *keyframe)
{
//printf("Reverb::save_data 1\n");
	FileXML output;
//printf("Reverb::save_data 1\n");

// cause xml file to store data directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
//printf("Reverb::save_data 1\n");

	output.tag.set_title("REVERB");
	output.tag.set_property("LEVELINIT", config.level_init);
	output.tag.set_property("DELAY_INIT", config.delay_init);
	output.tag.set_property("REF_LEVEL1", config.ref_level1);
	output.tag.set_property("REF_LEVEL2", config.ref_level2);
	output.tag.set_property("REF_TOTAL", config.ref_total);
//printf("Reverb::save_data 1\n");
	output.tag.set_property("REF_LENGTH", config.ref_length);
	output.tag.set_property("LOWPASS1", config.lowpass1);
	output.tag.set_property("LOWPASS2", config.lowpass2);
//printf("Reverb::save_data config.ref_level2 %f\n", config.ref_level2);
	output.append_tag();
	output.append_newline();
//printf("Reverb::save_data 1\n");
	
	
	
	output.terminate_string();
//printf("Reverb::save_data 2\n");
}

void Reverb::read_data(KeyFrame *keyframe)
{
	FileXML input;
// cause xml file to read directly from text
	input.set_shared_string(keyframe->data, strlen(keyframe->data));
	int result = 0;

	result = input.read_tag();

	if(!result)
	{
		if(input.tag.title_is("REVERB"))
		{
			config.level_init = input.tag.get_property("LEVELINIT", config.level_init);
			config.delay_init = input.tag.get_property("DELAY_INIT", config.delay_init);
			config.ref_level1 = input.tag.get_property("REF_LEVEL1", config.ref_level1);
			config.ref_level2 = input.tag.get_property("REF_LEVEL2", config.ref_level2);
			config.ref_total = input.tag.get_property("REF_TOTAL", config.ref_total);
			config.ref_length = input.tag.get_property("REF_LENGTH", config.ref_length);
			config.lowpass1 = input.tag.get_property("LOWPASS1", config.lowpass1);
			config.lowpass2 = input.tag.get_property("LOWPASS2", config.lowpass2);
		}
	}
}

void Reverb::update_gui()
{
	if(thread)
	{
		thread->window->lock_window();
		thread->window->level_init->update(config.level_init);
		thread->window->delay_init->update(config.delay_init);
		thread->window->ref_level1->update(config.ref_level1);
		thread->window->ref_level2->update(config.ref_level2);
		thread->window->ref_total->update(config.ref_total);
		thread->window->ref_length->update(config.ref_length);
		thread->window->lowpass1->update(config.lowpass1);
		thread->window->lowpass2->update(config.lowpass2);
		thread->window->unlock_window();
	}
}




int Reverb::load_from_file(char *path)
{
	FILE *in;
	int result = 0;
	int length;
	char string[1024];
	
	if(in = fopen(path, "rb"))
	{
		fseek(in, 0, SEEK_END);
		length = ftell(in);
		fseek(in, 0, SEEK_SET);
		fread(string, length, 1, in);
		fclose(in);
//		read_data(string);
	}
	else
	{
		perror("fopen:");
// failed
		ErrorBox errorbox("");
		char string[1024];
		sprintf(string, "Couldn't open %s.", path);
		errorbox.create_objects(string);
		errorbox.run_window();
		result = 1;
	}
	
	return result;
}

int Reverb::save_to_file(char *path)
{
	FILE *out;
	int result = 0;
	char string[1024];
	
	{
// 		ConfirmSave confirm;
// 		result = confirm.test_file("", path);
	}
	
	if(!result)
	{
		if(out = fopen(path, "wb"))
		{
//			save_data(string);
			fwrite(string, strlen(string), 1, out);
			fclose(out);
		}
		else
		{
			result = 1;
// failed
			ErrorBox errorbox("");
			char string[1024];
			sprintf(string, "Couldn't save %s.", path);
			errorbox.create_objects(string);
			errorbox.run_window();
			result = 1;
		}
	}
	
	return result;
}

ReverbEngine::ReverbEngine(Reverb *plugin)
 : Thread()
{
	this->plugin = plugin;
	completed = 0;
	input_lock.lock();
	output_lock.lock();
}

ReverbEngine::~ReverbEngine()
{
	completed = 1;
	input_lock.unlock();
	join();
}

int ReverbEngine::process_overlays(int output_buffer, int64_t size)
{
	this->output_buffer = output_buffer;
	this->size = size;
	input_lock.unlock();
}

int ReverbEngine::wait_process_overlays()
{
	output_lock.lock();
}
	
int ReverbEngine::process_overlay(double *in, double *out, double &out1, double &out2, double level, int64_t lowpass, int64_t samplerate, int64_t size)
{
// Modern niquist frequency is 44khz but pot limit is 20khz so can't use
// niquist
	if(lowpass == -1 || lowpass >= 20000)
	{
// no lowpass filter
		for(int i = 0; i < size; i++) out[i] += in[i] * level;
	}
	else
	{
		double coef = 0.25 * 2.0 * M_PI * (double)lowpass / (double)plugin->project_sample_rate;
		double a = coef * 0.25;
		double b = coef * 0.50;

		for(int i = 0; i < size; i++)
		{
			out2 += a * (3 * out1 + in[i] - out2);
			out2 += b * (out1 + in[i] - out2);
			out2 += a * (out1 + 3 * in[i] - out2);
			out2 += coef * (in[i] - out2);
			out1 = in[i];
			out[i] += out2 * level;
		}
	}
}

void ReverbEngine::run()
{
	int j, i;
	while(1)
	{
		input_lock.lock();
		if(completed) return;

// Process reverb
		for(i = 0; i < plugin->total_in_buffers; i++)
		{
			for(j = 0; j < plugin->config.ref_total + 1; j++)
			{
				if(plugin->ref_channels[i][j] == output_buffer)
					process_overlay(plugin->main_in[i], 
								&(plugin->dsp_in[plugin->ref_channels[i][j]][plugin->ref_offsets[i][j]]), 
								plugin->lowpass_in1[i][j], 
								plugin->lowpass_in2[i][j], 
								plugin->ref_levels[i][j], 
								plugin->ref_lowpass[i][j], 
								plugin->project_sample_rate, 
								size);
			}
		}

		output_lock.unlock();
	}
}







ReverbConfig::ReverbConfig()
{
}

int ReverbConfig::equivalent(ReverbConfig &that)
{
	return (EQUIV(level_init, that.level_init) &&
		delay_init == that.delay_init &&
		EQUIV(ref_level1, that.ref_level1) &&
		EQUIV(ref_level2, that.ref_level2) &&
		ref_total == that.ref_total &&
		ref_length == that.ref_length &&
		lowpass1 == that.lowpass1 &&
		lowpass2 == that.lowpass2);
}

void ReverbConfig::copy_from(ReverbConfig &that)
{
	level_init = that.level_init;
	delay_init = that.delay_init;
	ref_level1 = that.ref_level1;
	ref_level2 = that.ref_level2;
	ref_total = that.ref_total;
	ref_length = that.ref_length;
	lowpass1 = that.lowpass1;
	lowpass2 = that.lowpass2;
}

void ReverbConfig::interpolate(ReverbConfig &prev, 
	ReverbConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	level_init = prev.level_init;
	delay_init = prev.delay_init;
	ref_level1 = prev.ref_level1;
	ref_level2 = prev.ref_level2;
	ref_total = prev.ref_total;
	ref_length = prev.ref_length;
	lowpass1 = prev.lowpass1;
	lowpass2 = prev.lowpass2;
}

void ReverbConfig::dump()
{
	printf("ReverbConfig::dump %f %d %f %f %d %d %d %d\n", 
	level_init,
	delay_init, 
	ref_level1, 
	ref_level2, 
	ref_total, 
	ref_length, 
	lowpass1, 
	lowpass2);
}


