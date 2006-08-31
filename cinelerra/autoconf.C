#include "autoconf.h"
#include "bchash.h"
#include "filexml.h"


static char *xml_titles[] = 
{
	"SHOW_MUTE",
	"SHOW_CAMERA_X",
	"SHOW_CAMERA_Y",
	"SHOW_CAMERA_Z",
	"SHOW_PROJECTOR_X",
	"SHOW_PROJECTOR_Y",
	"SHOW_PROJECTOR_Z",
	"SHOW_FADE",
	"SHOW_PAN",
	"SHOW_MODE",
	"SHOW_MASK",
	"SHOW_NUDGE"
};

static int auto_defaults[] = 
{
	0,
	1,
	1,
	0,
	0,
	0,
	1,
	1,
	0,
	0
};

int AutoConf::load_defaults(BC_Hash* defaults)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		autos[i] = defaults->get(xml_titles[i], auto_defaults[i]);
	}
	transitions = defaults->get("SHOW_TRANSITIONS", 1);
	plugins = defaults->get("SHOW_PLUGINS", 1);
	return 0;
}

void AutoConf::load_xml(FileXML *file)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		autos[i] = file->tag.get_property(xml_titles[i], auto_defaults[i]);
	}
	transitions = file->tag.get_property("SHOW_TRANSITIONS", 1);
	plugins = file->tag.get_property("SHOW_PLUGINS", 1);
}

int AutoConf::save_defaults(BC_Hash* defaults)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		defaults->update(xml_titles[i], autos[i]);
	}
	defaults->update("SHOW_TRANSITIONS", transitions);
	defaults->update("SHOW_PLUGINS", plugins);
	return 0;
}

void AutoConf::save_xml(FileXML *file)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		file->tag.set_property(xml_titles[i], autos[i]);
	}
	file->tag.set_property("SHOW_TRANSITIONS", transitions);
	file->tag.set_property("SHOW_PLUGINS", plugins);
}

int AutoConf::set_all(int value)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		autos[i] = value;
	}
	transitions = value;
	plugins = value;
	return 0;
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
		autos[i] = src->autos[i];
	}
	transitions = src->transitions;
	plugins = src->plugins;
}


