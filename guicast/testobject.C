#include "testobject.h"

TestObject::TestObject(char *text)
{
	printf("TestObject() %s\n", text);
}

TestObject::~TestObject()
{
	printf("~TestObject()\n");
}

