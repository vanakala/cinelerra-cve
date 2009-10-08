
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

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
