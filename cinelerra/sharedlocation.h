#ifndef SHAREDPLUGINLOCATION_H
#define SHAREDPLUGINLOCATION_H

#include "edl.inc"
#include "filexml.inc"

class SharedLocation
{
public:
	SharedLocation();
	SharedLocation(int module, int plugin);
	
	void save(FileXML *file);
	void load(FileXML *file);
	int operator==(const SharedLocation &that);
	SharedLocation& operator=(const SharedLocation &that);
	int get_type();
	void calculate_title(char *string, 
		EDL *edl, 
		double position, 
		int convert_units,
		int plugin_type,
		int use_nudge);
	
	int module, plugin;
};



#endif
