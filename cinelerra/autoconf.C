// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "autoconf.h"
#include "bchash.h"
#include "filexml.h"

void AutoConf::load_defaults(BC_Hash* defaults)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
		auto_visible[i] = defaults->get(Automation::automation_tbl[i].xml_visible,
			Automation::automation_tbl[i].is_visble);

	transitions_visible = defaults->get("SHOW_TRANSITIONS", 1);
}

void AutoConf::load_xml(FileXML *file)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
		auto_visible[i] = file->tag.get_property(
			Automation::automation_tbl[i].xml_visible,
			Automation::automation_tbl[i].is_visble);

	transitions_visible = file->tag.get_property("SHOW_TRANSITIONS", 1);
}

void AutoConf::save_defaults(BC_Hash* defaults)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
		defaults->update(Automation::automation_tbl[i].xml_visible,
			auto_visible[i]);

	defaults->update("SHOW_TRANSITIONS", transitions_visible);
}

void AutoConf::save_xml(FileXML *file)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
		file->tag.set_property(Automation::automation_tbl[i].xml_visible,
			auto_visible[i]);

	file->tag.set_property("SHOW_TRANSITIONS", transitions_visible);
}

void AutoConf::set_all(int value)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
		auto_visible[i] = value;

	transitions_visible = value;
}

AutoConf& AutoConf::operator=(AutoConf &that)
{
	copy_from(&that);
	return *this;
}

void AutoConf::copy_from(AutoConf *src)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		auto_visible[i] = src->auto_visible[i];
	}
	transitions_visible = src->transitions_visible;
}
