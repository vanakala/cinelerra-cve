#ifndef PROPERTIES_H
#define PROPERTIES_H



// Linked list of name-value pair properties

#include <string.h>
#include "linklist.h"

class Property : public ListItem<Property>
{
public:
	Property(char *property, char *value);
	~Property();

	char *getProperty();
	char *getValue();
	void setValue(char *value);

private:
	char *property, *value;
};

class Properties : public List<Property>
{
public:
	Properties();
	~Properties();

	Property *get(char *property);
};

#endif
