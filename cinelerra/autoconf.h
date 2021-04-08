// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef AUTOCONF_H
#define AUTOCONF_H

#include "automation.h"
#include "bchash.inc"
#include "filexml.inc"

// Store what automation is visible.

class AutoConf
{
public:
	AutoConf() {};

	AutoConf& operator=(AutoConf &that);
	void copy_from(AutoConf *src);
	void load_defaults(BC_Hash* defaults);
	void save_defaults(BC_Hash* defaults);
	void load_xml(FileXML *file);
	void save_xml(FileXML *file);
	void set_all(int value = 1);  // set all parameters to value (default = 1)


// The array entries correspond to the Automation enums.
	int auto_visible[AUTOMATION_TOTAL];

// Other viewable things
	int transitions_visible;
};

#endif
