#include "intauto.h"

IntAuto::IntAuto(IntAutos *autos)
 : Auto((Autos*)autos)
{
}

IntAuto::~IntAuto()
{
}

int IntAuto::value_to_str(char *string, float value)
{
		if(value > 0) sprintf(string, "ON");
		else sprintf(string, "OFF");
}

void IntAuto::copy(int64_t start, int64_t end, FileXML *file, int default_auto)
{
	
}

void IntAuto::load(FileXML *file)
{
}
