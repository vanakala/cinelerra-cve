

#include "properties.h"

Property::Property(char *p, char *v) : ListItem<Property>()
{
	property = new char[strlen(p) + 1];
	value = new char[strlen(v) + 1];
	strcpy(property, p);
	strcpy(value, v);
}

Property::~Property()
{
	delete [] property;
	delete [] value;
}

char *Property::getProperty()
{
	return property;
}

char *Property::getValue()
{
	return value;
}

void Property::setValue(char *v)
{
	delete [] value;
	value = new char[strlen(v) + 1];
	strcpy(value, v);
}


Properties::Properties() : List<Property>()
{
}

Properties::~Properties()
{
}

Property *Properties::get(char *property)
{
	Property *current;
	for (current = first; current; current = NEXT)
	{
		if (!strcmp(property, current->getProperty()))
			break;
	}

	return current;
}
