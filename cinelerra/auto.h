#ifndef AUTO_H
#define AUTO_H

#include "auto.inc"
#include "edl.inc"
#include "guicast.h"
#include "filexml.inc"
#include "autos.inc"






// The default constructor is used for menu effects.

class Auto : public ListItem<Auto>
{
public:
	Auto();
	Auto(EDL *edl, Autos *autos);
	virtual ~Auto() {};

	virtual Auto& operator=(Auto &that);
	virtual int operator==(Auto &that);
	virtual void copy_from(Auto *that);
	virtual void copy(long start, long end, FileXML *file, int default_only);

	virtual void load(FileXML *file);

	virtual void get_caption(char *string) {};
	virtual float value_to_percentage();
	virtual float invalue_to_percentage();
	virtual float outvalue_to_percentage();

	int skip;       // if added by selection event for moves
	EDL *edl;
	Autos *autos;
	int WIDTH, HEIGHT;
// Units native to the track
	int is_default;
	long position;

private:
	virtual int value_to_str(char *string, float value) {};
};



#endif
