#include "fadeengine.h"
#include "vframe.h"

#include <stdint.h>









FadeUnit::FadeUnit(FadeEngine *server)
 : LoadClient(server)
{
	this->engine = server;
}

FadeUnit::~FadeUnit()
{
}


#define APPLY_FADE(equivalent, input_rows, output_rows, max, type, chroma_zero, components) \
{ \
	int64_t opacity = (int64_t)(alpha * max); \
	int64_t transparency = (int64_t)(max - opacity); \
 \
	for(int i = row1; i < row2; i++) \
	{ \
		type *in_row = (type*)input_rows[i]; \
		type *out_row = (type*)output_rows[i]; \
 \
		for(int j = 0; j < width; j++) \
		{ \
			if(components == 3) \
			{ \
				out_row[j * components] =  \
					(type)((int64_t)in_row[j * components] * opacity / max); \
				out_row[j * components + 1] =  \
					(type)(((int64_t)in_row[j * components + 1] * opacity +  \
						(int64_t)chroma_zero * transparency) / max); \
				out_row[j * components + 2] =  \
					(type)(((int64_t)in_row[j * components + 2] * opacity +  \
						(int64_t)chroma_zero * transparency) / max); \
			} \
			else \
			{ \
				if(!equivalent) \
				{ \
					out_row[j * components] = in_row[j * components]; \
					out_row[j * components + 1] = in_row[j * components + 1]; \
					out_row[j * components + 2] = in_row[j * components + 2]; \
				} \
 \
				out_row[j * components + 3] =  \
					(type)((int64_t)in_row[j * components + 3] * opacity / max); \
			} \
		} \
	} \
}



void FadeUnit::process_package(LoadPackage *package)
{
	FadePackage *pkg = (FadePackage*)package;
	VFrame *output = engine->output;
	VFrame *input = engine->input;
	float alpha = engine->alpha;
	int row1 = pkg->out_row1;
	int row2 = pkg->out_row2;
	unsigned char **in_rows = input->get_rows();
	unsigned char **out_rows = output->get_rows();
	int width = input->get_w();

	if(input->get_rows()[0] == output->get_rows()[0])
	{
		switch(input->get_color_model())
		{
			case BC_RGB888:
				APPLY_FADE(1, out_rows, in_rows, 0xff, unsigned char, 0x0, 3);
				break;
			case BC_RGBA8888:
				APPLY_FADE(1, out_rows, in_rows, 0xff, unsigned char, 0x0, 4);
				break;
			case BC_RGB161616:
				APPLY_FADE(1, out_rows, in_rows, 0xffff, uint16_t, 0x0, 3);
				break;
			case BC_RGBA16161616:
				APPLY_FADE(1, out_rows, in_rows, 0xffff, uint16_t, 0x0, 4);
				break;
			case BC_YUV888:
				APPLY_FADE(1, out_rows, in_rows, 0xff, unsigned char, 0x80, 3);
				break;
			case BC_YUVA8888:
				APPLY_FADE(1, out_rows, in_rows, 0xff, unsigned char, 0x80, 4);
				break;
			case BC_YUV161616:
				APPLY_FADE(1, out_rows, in_rows, 0xffff, uint16_t, 0x8000, 3);
				break;
			case BC_YUVA16161616:
				APPLY_FADE(1, out_rows, in_rows, 0xffff, uint16_t, 0x8000, 4);
				break;
		}
	}
	else
	{
		switch(input->get_color_model())
		{
			case BC_RGB888:
				APPLY_FADE(0, out_rows, in_rows, 0xff, unsigned char, 0x0, 3);
				break;
			case BC_RGBA8888:
				APPLY_FADE(0, out_rows, in_rows, 0xff, unsigned char, 0x0, 4);
				break;
			case BC_RGB161616:
				APPLY_FADE(0, out_rows, in_rows, 0xffff, uint16_t, 0x0, 3);
				break;
			case BC_RGBA16161616:
				APPLY_FADE(0, out_rows, in_rows, 0xffff, uint16_t, 0x0, 4);
				break;
			case BC_YUV888:
				APPLY_FADE(0, out_rows, in_rows, 0xff, unsigned char, 0x80, 3);
				break;
			case BC_YUVA8888:
				APPLY_FADE(0, out_rows, in_rows, 0xff, unsigned char, 0x80, 4);
				break;
			case BC_YUV161616:
				APPLY_FADE(0, out_rows, in_rows, 0xffff, uint16_t, 0x8000, 3);
				break;
			case BC_YUVA16161616:
				APPLY_FADE(0, out_rows, in_rows, 0xffff, uint16_t, 0x8000, 4);
				break;
		}
	}
}









FadeEngine::FadeEngine(int cpus)
 : LoadServer(cpus, cpus)
{
}

FadeEngine::~FadeEngine()
{
}

void FadeEngine::do_fade(VFrame *output, VFrame *input, float alpha)
{
	this->output = output;
	this->input = input;
	this->alpha = alpha;
	
// Sanity
	if(alpha == 1)
		output->copy_from(input);
	else
		process_packages();
}


void FadeEngine::init_packages()
{
	for(int i = 0; i < total_packages; i++)
	{
		FadePackage *package = (FadePackage*)packages[i];
		package->out_row1 = (int)(input->get_h() / 
			total_packages * 
			i);
		package->out_row2 = (int)((float)package->out_row1 +
			input->get_h() / 
			total_packages);

		if(i >= total_packages - 1)
			package->out_row2 = input->get_h();
	}
}

LoadClient* FadeEngine::new_client()
{
	return new FadeUnit(this);
}

LoadPackage* FadeEngine::new_package()
{
	return new FadePackage;
}


FadePackage::FadePackage()
{
}

