#ifndef AUTOCONF_H
#define AUTOCONF_H

#include "automation.h"
#include "bchash.inc"
#include "filexml.inc"
#include "maxchannels.h"
#include "module.inc"

// Store what automation is visible.

class AutoConf
{
public:
	AutoConf() {};
	~AutoConf() {};

	AutoConf& operator=(AutoConf &that);
	void copy_from(AutoConf *src);
	int load_defaults(BC_Hash* defaults);
	int save_defaults(BC_Hash* defaults);
	void load_xml(FileXML *file);
	void save_xml(FileXML *file);
	int set_all(int value = 1);  // set all parameters to value (default = 1)


// The array entries correspond to the Automation enums.
	int autos[AUTOMATION_TOTAL];

// Other viewable things
	int transitions;
	int plugins;
};

#endif
