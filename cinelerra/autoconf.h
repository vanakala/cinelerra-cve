#ifndef AUTOCONF_H
#define AUTOCONF_H

#include "defaults.inc"
#include "filexml.inc"
#include "maxchannels.h"
#include "module.inc"

class AutoConf
{
public:
	AutoConf() {};
	~AutoConf() {};

	AutoConf& operator=(AutoConf &that);
	void copy_from(AutoConf *src);
	int load_defaults(Defaults* defaults);
	int save_defaults(Defaults* defaults);
	void load_xml(FileXML *file);
	void save_xml(FileXML *file);
	int set_all(int value = 1);  // set all parameters to value (default = 1)

	int fade;
	int pan;
	int mute;
	int transitions;
	int plugins;
	int camera;
	int projector;
	int mode;
	int mask;
	int czoom;
	int pzoom;
};

#endif
