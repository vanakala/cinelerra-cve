#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "intauto.h"
#include "intautos.h"

IntAuto::IntAuto(EDL *edl, IntAutos *autos)
 : Auto(edl, (Autos*)autos)
{
	value = 0;
}

IntAuto::~IntAuto()
{
}

int IntAuto::operator==(Auto &that)
{
	return identical((IntAuto*)&that);
}

int IntAuto::operator==(IntAuto &that)
{
	return identical((IntAuto*)&that);
}

int IntAuto::identical(IntAuto *that)
{
	return that->value == value;
}

void IntAuto::load(FileXML *file)
{
	value = file->tag.get_property("VALUE", value);
//printf("IntAuto::load 1 %d\n", value);
}

void IntAuto::copy(int64_t start, int64_t end, FileXML *file, int default_auto)
{
	file->tag.set_title("AUTO");
	if(default_auto)
		file->tag.set_property("POSITION", 0);
	else
		file->tag.set_property("POSITION", position - start);
	file->tag.set_property("VALUE", value);
	file->append_tag();
	file->append_newline();
}


void IntAuto::copy_from(Auto *that)
{
	copy_from((IntAuto*)that);
//printf("IntAuto::copy_from(Auto *that) %d\n", value);
}

void IntAuto::copy_from(IntAuto *that)
{
//printf("IntAuto::copy_from(IntAuto *that) %d %d\n", value, that->value);
	Auto::copy_from(that);
	this->value = that->value;
}

float IntAuto::value_to_percentage()
{
	return (float)value;
}

int IntAuto::percentage_to_value(float percentage)
{
	return percentage > .5;
}


