#include "auto.h"
#include "autos.h"
#include "filexml.h"

Auto::Auto()
 : ListItem<Auto>()
{
	this->edl = 0;
	this->autos = 0;
	position = 0;
	skip = 0;
	WIDTH = 10;
	HEIGHT = 10;
	is_default = 0;
}

Auto::Auto(EDL *edl, Autos *autos)
 : ListItem<Auto>()
{
	this->edl = edl;
	this->autos = autos;
	position = 0;
	skip = 0;
	WIDTH = 10;
	HEIGHT = 10;
	is_default = 0;
}

Auto& Auto::operator=(Auto& that)
{
	copy_from(&that);
	return *this;
}

int Auto::operator==(Auto &that)
{
	printf("Auto::operator== called\n");
	return 0;
}

void Auto::copy(int64_t start, int64_t end, FileXML *file, int default_only)
{
	printf("Auto::copy called\n");
}

void Auto::copy_from(Auto *that)
{
	this->position = that->position;
}


void Auto::load(FileXML *xml)
{
	printf("Auto::load\n");
}



float Auto::value_to_percentage()
{
	return 0;
}

float Auto::invalue_to_percentage()
{
	return 0;
}

float Auto::outvalue_to_percentage()
{
	return 0;
}

