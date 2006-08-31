#include "filexml.h"
#include "blur.h"
#include "blurwindow.h"
#include "bchash.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>







BlurConfig::BlurConfig()
{
	vertical = 1;
	horizontal = 1;
	radius = 5;
	a = r = g = b = 1;
}

int BlurConfig::equivalent(BlurConfig &that)
{
	return (vertical == that.vertical && 
		horizontal == that.horizontal && 
		radius == that.radius &&
		a == that.a &&
		r == that.r &&
		g == that.g &&
		b == that.b);
}

void BlurConfig::copy_from(BlurConfig &that)
{
	vertical = that.vertical;
	horizontal = that.horizontal;
	radius = that.radius;
	a = that.a;
	r = that.r;
	g = that.g;
	b = that.b;
}

void BlurConfig::interpolate(BlurConfig &prev, 
	BlurConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);


//printf("BlurConfig::interpolate %d %d %d\n", prev_frame, next_frame, current_frame);
	this->vertical = (int)(prev.vertical * prev_scale + next.vertical * next_scale);
	this->horizontal = (int)(prev.horizontal * prev_scale + next.horizontal * next_scale);
	this->radius = (int)(prev.radius * prev_scale + next.radius * next_scale);
	a = prev.a;
	r = prev.r;
	g = prev.g;
	b = prev.b;
}






REGISTER_PLUGIN(BlurMain)








BlurMain::BlurMain(PluginServer *server)
 : PluginVClient(server)
{
	defaults = 0;
	temp = 0;
	need_reconfigure = 1;
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

BlurMain::~BlurMain()
{
//printf("BlurMain::~BlurMain 1\n");
	PLUGIN_DESTRUCTOR_MACRO

	if(temp) delete temp;
	if(engine)
	{
		for(int i = 0; i < (get_project_smp() + 1); i++)
			delete engine[i];
		delete [] engine;
	}
}

char* BlurMain::plugin_title() { return N_("Blur"); }
int BlurMain::is_realtime() { return 1; }


NEW_PICON_MACRO(BlurMain)

SHOW_GUI_MACRO(BlurMain, BlurThread)

SET_STRING_MACRO(BlurMain)

RAISE_WINDOW_MACRO(BlurMain)

LOAD_CONFIGURATION_MACRO(BlurMain, BlurConfig)




int BlurMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	int i, j, k, l;
	unsigned char **input_rows, **output_rows;

	this->input = input_ptr;
	this->output = output_ptr;
	need_reconfigure |= load_configuration();


//printf("BlurMain::process_realtime 1 %d %d\n", need_reconfigure, config.radius);
	if(need_reconfigure)
	{
		int y1, y2, y_increment;

		if(!engine)
		{
			engine = new BlurEngine*[(get_project_smp() + 1)];
			for(int i = 0; i < (get_project_smp() + 1); i++)
			{
				engine[i] = new BlurEngine(this, 
					input->get_h() * i / (get_project_smp() + 1), 
					input->get_h() * (i + 1) / (get_project_smp() + 1));
				engine[i]->start();
			}
		}

		for(i = 0; i < (get_project_smp() + 1); i++)
			engine[i]->reconfigure();
		need_reconfigure = 0;
	}


	if(temp && 
		(temp->get_w() != input_ptr->get_w() ||
		temp->get_h() != input_ptr->get_h()))
	{
		delete temp;
		temp = 0;
	}

	if(!temp)
		temp = new VFrame(0,
			input_ptr->get_w(),
			input_ptr->get_h(),
			input_ptr->get_color_model());

	input_rows = input_ptr->get_rows();
	output_rows = output_ptr->get_rows();

	if(config.radius < 2 || 
		(!config.vertical && !config.horizontal))
	{
// Data never processed so copy if necessary
		if(input_rows[0] != output_rows[0])
		{
			output_ptr->copy_from(input_ptr);
		}
	}
	else
	{
// Process blur
// TODO
// Can't blur recursively.  Need to blur vertically to a temp and 
// horizontally to the output in 2 discrete passes.
		for(i = 0; i < (get_project_smp() + 1); i++)
		{
			engine[i]->start_process_frame(output_ptr, input_ptr);
		}

		for(i = 0; i < (get_project_smp() + 1); i++)
		{
			engine[i]->wait_process_frame();
		}
	}

	return 0;
}


void BlurMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->horizontal->update(config.horizontal);
		thread->window->vertical->update(config.vertical);
		thread->window->radius->update(config.radius);
		thread->window->a->update(config.a);
		thread->window->r->update(config.r);
		thread->window->g->update(config.g);
		thread->window->b->update(config.b);
		thread->window->unlock_window();
	}
}


int BlurMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%sblur.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.vertical = defaults->get("VERTICAL", config.vertical);
	config.horizontal = defaults->get("HORIZONTAL", config.horizontal);
	config.radius = defaults->get("RADIUS", config.radius);
	config.r = defaults->get("R", config.r);
	config.g = defaults->get("G", config.g);
	config.b = defaults->get("B", config.b);
	config.a = defaults->get("A", config.a);
	return 0;
}


int BlurMain::save_defaults()
{
	defaults->update("VERTICAL", config.vertical);
	defaults->update("HORIZONTAL", config.horizontal);
	defaults->update("RADIUS", config.radius);
	defaults->update("R", config.r);
	defaults->update("G", config.g);
	defaults->update("B", config.b);
	defaults->update("A", config.a);
	defaults->save();
	return 0;
}



void BlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("BLUR");
	output.tag.set_property("VERTICAL", config.vertical);
	output.tag.set_property("HORIZONTAL", config.horizontal);
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.terminate_string();
}

void BlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("BLUR"))
			{
				config.vertical = input.tag.get_property("VERTICAL", config.vertical);
				config.horizontal = input.tag.get_property("HORIZONTAL", config.horizontal);
				config.radius = input.tag.get_property("RADIUS", config.radius);
//printf("BlurMain::read_data 1 %d %d %s\n", get_source_position(), keyframe->position, keyframe->data);
				config.r = input.tag.get_property("R", config.r);
				config.g = input.tag.get_property("G", config.g);
				config.b = input.tag.get_property("B", config.b);
				config.a = input.tag.get_property("A", config.a);
			}
		}
	}
}









BlurEngine::BlurEngine(BlurMain *plugin, int start_out, int end_out)
 : Thread()
{
	int size = plugin->input->get_w() > plugin->input->get_h() ? 
		plugin->input->get_w() : plugin->input->get_h();
	this->plugin = plugin;
	this->start_out = start_out;
	this->end_out = end_out;
	last_frame = 0;
	val_p = new pixel_f[size];
	val_m = new pixel_f[size];
	src = new pixel_f[size];
	dst = new pixel_f[size];
	set_synchronous(1);
	input_lock.lock();
	output_lock.lock();
}

BlurEngine::~BlurEngine()
{
	last_frame = 1;
	input_lock.unlock();
	join();
}

int BlurEngine::start_process_frame(VFrame *output, VFrame *input)
{
	this->output = output;
	this->input = input;
	input_lock.unlock();
	return 0;
}

int BlurEngine::wait_process_frame()
{
	output_lock.lock();
	return 0;
}

void BlurEngine::run()
{
	int i, j, k, l;
	int strip_size;


	while(1)
	{
		input_lock.lock();
		if(last_frame)
		{
			output_lock.unlock();
			return;
		}

		start_in = start_out - plugin->config.radius;
		end_in = end_out + plugin->config.radius;
		if(start_in < 0) start_in = 0;
		if(end_in > plugin->input->get_h()) end_in = plugin->input->get_h();
		strip_size = end_in - start_in;
		color_model = input->get_color_model();
		int w = input->get_w();
		int h = input->get_h();





#define BLUR(type, max, components) \
{ \
	type **input_rows = (type **)input->get_rows(); \
	type **output_rows = (type **)output->get_rows(); \
	type **current_input = input_rows; \
	type **current_output = output_rows; \
	vmax = max; \
 \
	if(plugin->config.vertical) \
	{ \
/* Vertical pass */ \
		if(plugin->config.horizontal) \
		{ \
			current_output = (type **)plugin->temp->get_rows(); \
		} \
 \
		for(j = 0; j < w; j++) \
		{ \
			bzero(val_p, sizeof(pixel_f) * (end_in - start_in)); \
			bzero(val_m, sizeof(pixel_f) * (end_in - start_in)); \
 \
			for(l = 0, k = start_in; k < end_in; l++, k++) \
			{ \
				if(plugin->config.r) src[l].r = (float)current_input[k][j * components]; \
				if(plugin->config.g) src[l].g = (float)current_input[k][j * components + 1]; \
				if(plugin->config.b) src[l].b = (float)current_input[k][j * components + 2]; \
				if(components == 4) \
					if(plugin->config.a) src[l].a = (float)current_input[k][j * components + 3]; \
			} \
 \
			if(components == 4) \
				blur_strip4(strip_size); \
			else \
				blur_strip3(strip_size); \
 \
			for(l = start_out - start_in, k = start_out; k < end_out; l++, k++) \
			{ \
				if(plugin->config.r) current_output[k][j * components] = (type)dst[l].r; \
				if(plugin->config.g) current_output[k][j * components + 1] = (type)dst[l].g; \
				if(plugin->config.b) current_output[k][j * components + 2] = (type)dst[l].b; \
				if(components == 4) \
					if(plugin->config.a) current_output[k][j * components + 3] = (type)dst[l].a; \
			} \
		} \
 \
		current_input = current_output; \
		current_output = output_rows; \
	} \
 \
 \
	if(plugin->config.horizontal) \
	{ \
/* Horizontal pass */ \
		for(j = start_out; j < end_out; j++) \
		{ \
			bzero(val_p, sizeof(pixel_f) * w); \
			bzero(val_m, sizeof(pixel_f) * w); \
 \
			for(k = 0; k < w; k++) \
			{ \
				if(plugin->config.r) src[k].r = (float)current_input[j][k * components]; \
				if(plugin->config.g) src[k].g = (float)current_input[j][k * components + 1]; \
				if(plugin->config.b) src[k].b = (float)current_input[j][k * components + 2]; \
				if(components == 4) \
					if(plugin->config.a) src[k].a = (float)current_input[j][k * components + 3]; \
			} \
 \
 			if(components == 4) \
				blur_strip4(w); \
			else \
				blur_strip3(w); \
 \
			for(k = 0; k < w; k++) \
			{ \
				if(plugin->config.r) current_output[j][k * components] = (type)dst[k].r; \
				if(plugin->config.g) current_output[j][k * components + 1] = (type)dst[k].g; \
				if(plugin->config.b) current_output[j][k * components + 2] = (type)dst[k].b; \
				if(components == 4) \
					if(plugin->config.a) current_output[j][k * components + 3] = (type)dst[k].a; \
			} \
		} \
	} \
}



		switch(color_model)
		{
			case BC_RGB888:
			case BC_YUV888:
				BLUR(unsigned char, 0xff, 3);
				break;
			case BC_RGB_FLOAT:
				BLUR(float, 1.0, 3);
				break;
			case BC_RGBA8888:
			case BC_YUVA8888:
				BLUR(unsigned char, 0xff, 4);
				break;
			case BC_RGBA_FLOAT:
				BLUR(float, 1.0, 4);
				break;
			case BC_RGB161616:
			case BC_YUV161616:
				BLUR(uint16_t, 0xffff, 3);
				break;
			case BC_RGBA16161616:
			case BC_YUVA16161616:
				BLUR(uint16_t, 0xffff, 4);
				break;
		}

		output_lock.unlock();
	}
}

int BlurEngine::reconfigure()
{
	std_dev = sqrt(-(double)(plugin->config.radius * plugin->config.radius) / 
		(2 * log (1.0 / 255.0)));
	get_constants();
}

int BlurEngine::get_constants()
{
	int i;
	double constants[8];
	double div;

	div = sqrt(2 * M_PI) * std_dev;
	constants[0] = -1.783 / std_dev;
	constants[1] = -1.723 / std_dev;
	constants[2] = 0.6318 / std_dev;
	constants[3] = 1.997  / std_dev;
	constants[4] = 1.6803 / div;
	constants[5] = 3.735 / div;
	constants[6] = -0.6803 / div;
	constants[7] = -0.2598 / div;

	n_p[0] = constants[4] + constants[6];
	n_p[1] = exp(constants[1]) *
				(constants[7] * sin(constants[3]) -
				(constants[6] + 2 * constants[4]) * cos(constants[3])) +
				exp(constants[0]) *
				(constants[5] * sin(constants[2]) -
				(2 * constants[6] + constants[4]) * cos(constants[2]));

	n_p[2] = 2 * exp(constants[0] + constants[1]) *
				((constants[4] + constants[6]) * cos(constants[3]) * 
				cos(constants[2]) - constants[5] * 
				cos(constants[3]) * sin(constants[2]) -
				constants[7] * cos(constants[2]) * sin(constants[3])) +
				constants[6] * exp(2 * constants[0]) +
				constants[4] * exp(2 * constants[1]);

	n_p[3] = exp(constants[1] + 2 * constants[0]) *
				(constants[7] * sin(constants[3]) - 
				constants[6] * cos(constants[3])) +
				exp(constants[0] + 2 * constants[1]) *
				(constants[5] * sin(constants[2]) - constants[4] * 
				cos(constants[2]));
	n_p[4] = 0.0;

	d_p[0] = 0.0;
	d_p[1] = -2 * exp(constants[1]) * cos(constants[3]) -
				2 * exp(constants[0]) * cos(constants[2]);

	d_p[2] = 4 * cos(constants[3]) * cos(constants[2]) * 
				exp(constants[0] + constants[1]) +
				exp(2 * constants[1]) + exp (2 * constants[0]);

	d_p[3] = -2 * cos(constants[2]) * exp(constants[0] + 2 * constants[1]) -
				2 * cos(constants[3]) * exp(constants[1] + 2 * constants[0]);

	d_p[4] = exp(2 * constants[0] + 2 * constants[1]);

	for(i = 0; i < 5; i++) d_m[i] = d_p[i];

	n_m[0] = 0.0;
	for(i = 1; i <= 4; i++)
		n_m[i] = n_p[i] - d_p[i] * n_p[0];

	double sum_n_p, sum_n_m, sum_d;
	double a, b;

	sum_n_p = 0.0;
	sum_n_m = 0.0;
	sum_d = 0.0;
	for(i = 0; i < 5; i++)
	{
		sum_n_p += n_p[i];
		sum_n_m += n_m[i];
		sum_d += d_p[i];
	}

	a = sum_n_p / (1 + sum_d);
	b = sum_n_m / (1 + sum_d);

	for (i = 0; i < 5; i++)
	{
		bd_p[i] = d_p[i] * a;
		bd_m[i] = d_m[i] * b;
	}
	return 0;
}

#define BOUNDARY(x) if((x) > vmax) (x) = vmax; else if((x) < 0) (x) = 0;

int BlurEngine::transfer_pixels(pixel_f *src1, pixel_f *src2, pixel_f *dest, int size)
{
	int i;
	float sum;

// printf("BlurEngine::transfer_pixels %d %d %d %d\n", 
// plugin->config.r, 
// plugin->config.g, 
// plugin->config.b, 
// plugin->config.a);

	for(i = 0; i < size; i++)
    {
		sum = src1[i].r + src2[i].r;
		BOUNDARY(sum);
		dest[i].r = sum;
		sum = src1[i].g + src2[i].g;
		BOUNDARY(sum);
		dest[i].g = sum;
		sum = src1[i].b + src2[i].b;
		BOUNDARY(sum);
		dest[i].b = sum;
		sum = src1[i].a + src2[i].a;
		BOUNDARY(sum);
		dest[i].a = sum;
    }
	return 0;
}


int BlurEngine::multiply_alpha(pixel_f *row, int size)
{
	register int i;
	register float alpha;

// 	for(i = 0; i < size; i++)
// 	{
// 		alpha = (float)row[i].a / vmax;
// 		row[i].r *= alpha;
// 		row[i].g *= alpha;
// 		row[i].b *= alpha;
// 	}
	return 0;
}

int BlurEngine::separate_alpha(pixel_f *row, int size)
{
	register int i;
	register float alpha;
	register float result;
	
// 	for(i = 0; i < size; i++)
// 	{
// 		if(row[i].a > 0 && row[i].a < vmax)
// 		{
// 			alpha = (float)row[i].a / vmax;
// 			result = (float)row[i].r / alpha;
// 			row[i].r = (result > vmax ? vmax : result);
// 			result = (float)row[i].g / alpha;
// 			row[i].g = (result > vmax ? vmax : result);
// 			result = (float)row[i].b / alpha;
// 			row[i].b = (result > vmax ? vmax : result);
// 		}
// 	}
	return 0;
}

int BlurEngine::blur_strip3(int &size)
{
	multiply_alpha(src, size);

	sp_p = src;
	sp_m = src + size - 1;
	vp = val_p;
	vm = val_m + size - 1;

	initial_p = sp_p[0];
	initial_m = sp_m[0];

	int l;
	for(int k = 0; k < size; k++)
	{
		terms = (k < 4) ? k : 4;
		for(l = 0; l <= terms; l++)
		{
			if(plugin->config.r)
			{
				vp->r += n_p[l] * sp_p[-l].r - d_p[l] * vp[-l].r;
				vm->r += n_m[l] * sp_m[l].r - d_m[l] * vm[l].r;
			}
			if(plugin->config.g)
			{
				vp->g += n_p[l] * sp_p[-l].g - d_p[l] * vp[-l].g;
				vm->g += n_m[l] * sp_m[l].g - d_m[l] * vm[l].g;
			}
			if(plugin->config.b)
			{
				vp->b += n_p[l] * sp_p[-l].b - d_p[l] * vp[-l].b;
				vm->b += n_m[l] * sp_m[l].b - d_m[l] * vm[l].b;
			}
		}
		for( ; l <= 4; l++)
		{
			if(plugin->config.r)
			{
				vp->r += (n_p[l] - bd_p[l]) * initial_p.r;
				vm->r += (n_m[l] - bd_m[l]) * initial_m.r;
			}
			if(plugin->config.g)
			{
				vp->g += (n_p[l] - bd_p[l]) * initial_p.g;
				vm->g += (n_m[l] - bd_m[l]) * initial_m.g;
			}
			if(plugin->config.b)
			{
				vp->b += (n_p[l] - bd_p[l]) * initial_p.b;
				vm->b += (n_m[l] - bd_m[l]) * initial_m.b;
			}
		}
		sp_p++;
		sp_m--;
		vp++;
		vm--;
	}
	transfer_pixels(val_p, val_m, dst, size);
	separate_alpha(dst, size);
	return 0;
}


int BlurEngine::blur_strip4(int &size)
{
	multiply_alpha(src, size);

	sp_p = src;
	sp_m = src + size - 1;
	vp = val_p;
	vm = val_m + size - 1;

	initial_p = sp_p[0];
	initial_m = sp_m[0];

	int l;
	for(int k = 0; k < size; k++)
	{
		terms = (k < 4) ? k : 4;

		for(l = 0; l <= terms; l++)
		{
			if(plugin->config.r)
			{
				vp->r += n_p[l] * sp_p[-l].r - d_p[l] * vp[-l].r;
				vm->r += n_m[l] * sp_m[l].r - d_m[l] * vm[l].r;
			}
			if(plugin->config.g)
			{
				vp->g += n_p[l] * sp_p[-l].g - d_p[l] * vp[-l].g;
				vm->g += n_m[l] * sp_m[l].g - d_m[l] * vm[l].g;
			}
			if(plugin->config.b)
			{
				vp->b += n_p[l] * sp_p[-l].b - d_p[l] * vp[-l].b;
				vm->b += n_m[l] * sp_m[l].b - d_m[l] * vm[l].b;
			}
			if(plugin->config.a)
			{
				vp->a += n_p[l] * sp_p[-l].a - d_p[l] * vp[-l].a;
				vm->a += n_m[l] * sp_m[l].a - d_m[l] * vm[l].a;
			}
		}

		for( ; l <= 4; l++)
		{
			if(plugin->config.r)
			{
				vp->r += (n_p[l] - bd_p[l]) * initial_p.r;
				vm->r += (n_m[l] - bd_m[l]) * initial_m.r;
			}
			if(plugin->config.g)
			{
				vp->g += (n_p[l] - bd_p[l]) * initial_p.g;
				vm->g += (n_m[l] - bd_m[l]) * initial_m.g;
			}
			if(plugin->config.b)
			{
				vp->b += (n_p[l] - bd_p[l]) * initial_p.b;
				vm->b += (n_m[l] - bd_m[l]) * initial_m.b;
			}
			if(plugin->config.a)
			{
				vp->a += (n_p[l] - bd_p[l]) * initial_p.a;
				vm->a += (n_m[l] - bd_m[l]) * initial_m.a;
			}
		}

		sp_p++;
		sp_m--;
		vp++;
		vm--;
	}
	transfer_pixels(val_p, val_m, dst, size);
	separate_alpha(dst, size);
	return 0;
}






