#ifndef FLOATAUTO_H
#define FLOATAUTO_H

// Automation point that takes floating point values

class FloatAuto;

#include "auto.h"
#include "edl.inc"
#include "floatautos.inc"

class FloatAuto : public Auto
{
public:
	FloatAuto() {};
	FloatAuto(EDL *edl, FloatAutos *autos);
	~FloatAuto();

	int operator==(Auto &that);
	int operator==(FloatAuto &that);
	int identical(FloatAuto *src);
	void copy_from(Auto *that);
	void copy_from(FloatAuto *that);
	void copy(int64_t start, int64_t end, FileXML *file, int default_only);
	void load(FileXML *xml);

 	float value_to_percentage();
 	float invalue_to_percentage();
 	float outvalue_to_percentage();
/* 	float percentage_to_value(float percentage);
 * 	float percentage_to_invalue(float percentage);
 * 	float percentage_to_outvalue(float percentage);
 */

// Control values are relative to value
	float value, control_in_value, control_out_value;
// Control positions relative to value position for drawing
	int64_t control_in_position, control_out_position;

private:
	int value_to_str(char *string, float value);
};



#endif
