#include "apatchgui.inc"
#include "bcpan.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "panauto.h"

PanAuto::PanAuto(EDL *edl, PanAutos *autos)
 : Auto(edl, (Autos*)autos)
{
	bzero(values, MAXCHANNELS * sizeof(float));
	handle_x = handle_y = 0;
}

PanAuto::~PanAuto()
{
}

int PanAuto::operator==(Auto &that)
{
	PanAuto *panauto = (PanAuto*)&that;

	return handle_x == panauto->handle_x &&
		handle_y == panauto->handle_y;
}

void PanAuto::rechannel()
{
	BC_Pan::stick_to_values(values,
		edl->session->audio_channels, 
		edl->session->achannel_positions, 
		handle_x, 
		handle_y,
		PAN_RADIUS,
		1);
}

void PanAuto::load(FileXML *file)
{
	bzero(values, MAXCHANNELS * sizeof(float));
	position = file->tag.get_property("POSITION", (int64_t)0);
	handle_x = file->tag.get_property("HANDLE_X", (int64_t)handle_x);
	handle_y = file->tag.get_property("HANDLE_Y", (int64_t)handle_y);
	for(int i = 0; i < edl->session->audio_channels; i++)
	{
		char string[BCTEXTLEN];
		sprintf(string, "VALUE%d", i);
		values[i] = file->tag.get_property(string, values[i]);
	}
}

void PanAuto::copy(int64_t start, int64_t end, FileXML *file, int default_auto)
{
	file->tag.set_title("AUTO");
	if(default_auto)
		file->tag.set_property("POSITION", 0);
	else
		file->tag.set_property("POSITION", position - start);
	file->tag.set_property("HANDLE_X", (int64_t)handle_x);
	file->tag.set_property("HANDLE_Y", (int64_t)handle_y);
	for(int i = 0; i < edl->session->audio_channels; i++)
	{
		char  string[BCTEXTLEN];
		sprintf(string, "VALUE%d", i);
		file->tag.set_property(string, values[i]);
	}
	file->append_tag();
}


void PanAuto::copy_from(Auto *that)
{
	Auto::copy_from(that);

	PanAuto *pan_auto = (PanAuto*)that;
	memcpy(this->values, pan_auto->values, MAXCHANNELS * sizeof(float));
	this->handle_x = pan_auto->handle_x;
	this->handle_y = pan_auto->handle_y;
}

void PanAuto::dump()
{
	printf("		handle_x %d\n", handle_x);
	printf("		handle_y %d\n", handle_y);
	for(int i = 0; i < edl->session->audio_channels; i++)
		printf("		value %d %f\n", i, values[i]);
}


