#include "autos.h"
#include "clip.h"
#include "filexml.h"
#include "floatauto.h"

FloatAuto::FloatAuto(EDL *edl, FloatAutos *autos)
 : Auto(edl, (Autos*)autos)
{
	value = 0;
	control_in_value = 0;
	control_out_value = 0;
	control_in_position = 0;
	control_out_position = 0;
}

FloatAuto::~FloatAuto()
{
}

int FloatAuto::operator==(Auto &that)
{
	return identical((FloatAuto*)&that);
}


int FloatAuto::operator==(FloatAuto &that)
{
	return identical((FloatAuto*)&that);
}


int FloatAuto::identical(FloatAuto *src)
{
	return EQUIV(value, src->value) &&
		EQUIV(control_in_value, src->control_in_value) &&
		EQUIV(control_out_value, src->control_out_value) &&
		control_in_position == src->control_in_position &&
		control_out_position == src->control_out_position;
}

float FloatAuto::value_to_percentage()
{
//printf("FloatAuto::value_to_percentage %f %f %f %f\n", value, autos->min, autos->max, (value - autos->min) / (autos->max - autos->min));
	return (value - autos->min) / (autos->max - autos->min);
}

float FloatAuto::invalue_to_percentage()
{
//printf("FloatAuto::value_to_percentage %f %f %f %f\n", value, autos->min, autos->max, (value - autos->min) / (autos->max - autos->min));
	return (value + control_in_value - autos->min) / 
		(autos->max - autos->min);
}

float FloatAuto::outvalue_to_percentage()
{
//printf("FloatAuto::value_to_percentage %f %f %f %f\n", value, autos->min, autos->max, (value - autos->min) / (autos->max - autos->min));
	return (value + control_out_value - autos->min) / 
		(autos->max - autos->min);
}

float FloatAuto::percentage_to_value(float percentage)
{
//printf("FloatAuto::value_to_percentage %f %f %f %f\n", value, autos->min, autos->max, (value - autos->min) / (autos->max - autos->min));
	return percentage * (autos->max - autos->min) + autos->min;
}

float FloatAuto::percentage_to_invalue(float percentage)
{
//printf("FloatAuto::value_to_percentage %f %f %f %f\n", value, autos->min, autos->max, (value - autos->min) / (autos->max - autos->min));
	return percentage * (autos->max - autos->min) + autos->min - value;
}

float FloatAuto::percentage_to_outvalue(float percentage)
{
//printf("FloatAuto::value_to_percentage %f %f %f %f\n", value, autos->min, autos->max, (value - autos->min) / (autos->max - autos->min));
	return percentage * (autos->max - autos->min) + autos->min - value;
}

void FloatAuto::copy_from(Auto *that)
{
	copy_from((FloatAuto*)that);
}

void FloatAuto::copy_from(FloatAuto *that)
{
//printf("FloatAuto::copy_from(IntAuto *that) %f %f\n", value, that->value);
	Auto::copy_from(that);
	this->value = that->value;
	this->control_in_value = that->control_in_value;
	this->control_out_value = that->control_out_value;
	this->control_in_position = that->control_in_position;
	this->control_out_position = that->control_out_position;
}

int FloatAuto::value_to_str(char *string, float value)
{
	int j = 0, i = 0;
	if(value > 0) 
		sprintf(string, "+%.2f", value);
	else
		sprintf(string, "%.2f", value);

// fix number
	if(value == 0)
	{
		j = 0;
		string[1] = 0;
	}
	else
	if(value < 1 && value > -1) 
	{
		j = 1;
		string[j] = string[0];
	}
	else 
	{
		j = 0;
		string[3] = 0;
	}
	
	while(string[j] != 0) string[i++] = string[j++];
	string[i] = 0;

	return 0;
}

void FloatAuto::copy(int64_t start, int64_t end, FileXML *file, int default_auto)
{
	file->tag.set_title("AUTO");
	if(default_auto)
		file->tag.set_property("POSITION", 0);
	else
		file->tag.set_property("POSITION", position - start);
	file->tag.set_property("VALUE", value);
	file->tag.set_property("CONTROL_IN_VALUE", control_in_value);
	file->tag.set_property("CONTROL_OUT_VALUE", control_out_value);
	file->tag.set_property("CONTROL_IN_POSITION", control_in_position);
	file->tag.set_property("CONTROL_OUT_POSITION", control_out_position);
	file->append_tag();
	file->append_newline();
}

void FloatAuto::load(FileXML *file)
{
	value = file->tag.get_property("VALUE", value);
	control_in_value = file->tag.get_property("CONTROL_IN_VALUE", control_in_value);
	control_out_value = file->tag.get_property("CONTROL_OUT_VALUE", control_out_value);
	control_in_position = file->tag.get_property("CONTROL_IN_POSITION", control_in_position);
	control_out_position = file->tag.get_property("CONTROL_OUT_POSITION", control_out_position);
}
