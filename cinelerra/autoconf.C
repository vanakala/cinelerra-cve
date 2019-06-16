
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

#include "autoconf.h"
#include "bchash.h"
#include "filexml.h"

void AutoConf::load_defaults(BC_Hash* defaults)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
		auto_visible[i] = defaults->get(Automation::automation_tbl[i].xml_visible,
			Automation::automation_tbl[i].is_visble);

	transitions_visible = defaults->get("SHOW_TRANSITIONS", 1);
	plugins_visible = defaults->get("SHOW_PLUGINS", 1);
}

void AutoConf::load_xml(FileXML *file)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
		auto_visible[i] = file->tag.get_property(
			Automation::automation_tbl[i].xml_visible,
			Automation::automation_tbl[i].is_visble);

	transitions_visible = file->tag.get_property("SHOW_TRANSITIONS", 1);
	plugins_visible = file->tag.get_property("SHOW_PLUGINS", 1);
}

void AutoConf::save_defaults(BC_Hash* defaults)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
		defaults->update(Automation::automation_tbl[i].xml_visible,
			auto_visible[i]);

	defaults->update("SHOW_TRANSITIONS", transitions_visible);
	defaults->update("SHOW_PLUGINS", plugins_visible);
}

void AutoConf::save_xml(FileXML *file)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
		file->tag.set_property(Automation::automation_tbl[i].xml_visible,
			auto_visible[i]);

	file->tag.set_property("SHOW_TRANSITIONS", transitions_visible);
	file->tag.set_property("SHOW_PLUGINS", plugins_visible);
}

void AutoConf::set_all(int value)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
		auto_visible[i] = value;

	transitions_visible = value;
	plugins_visible = value;
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
	plugins_visible = src->plugins_visible;
}


