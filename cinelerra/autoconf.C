#include "autoconf.h"
#include "defaults.h"
#include "filexml.h"

int AutoConf::load_defaults(Defaults* defaults)
{
	fade = defaults->get("SHOW_FADE", 0);
	pan = defaults->get("SHOW_PAN", 1);
	mute = defaults->get("SHOW_MUTE", 0);
	transitions = defaults->get("SHOW_TRANSITIONS", 1);
	plugins = defaults->get("SHOW_PLUGINS", 1);
	camera = defaults->get("SHOW_CAMERA", 1);
	projector = defaults->get("SHOW_PROJECTOR", 1);
	mode = defaults->get("SHOW_MODE", 1);
	mask = defaults->get("SHOW_MASK", 0);
	czoom = defaults->get("SHOW_CZOOM", 0);
	pzoom = defaults->get("SHOW_PZOOM", 0);
	return 0;
}

void AutoConf::load_xml(FileXML *file)
{
	fade = file->tag.get_property("SHOW_FADE", 0);
	pan = file->tag.get_property("SHOW_PAN", 0);
	mute = file->tag.get_property("SHOW_MUTE", 0);
	transitions = file->tag.get_property("SHOW_TRANSITIONS", 1);
	plugins = file->tag.get_property("SHOW_PLUGINS", 1);
	camera = file->tag.get_property("SHOW_CAMERA", 1);
	projector = file->tag.get_property("SHOW_PROJECTOR", 1);
	mode = file->tag.get_property("SHOW_MODE", 1);
	mask = file->tag.get_property("SHOW_MASK", 0);
	czoom = file->tag.get_property("SHOW_CZOOM", 0);
	pzoom = file->tag.get_property("SHOW_PZOOM", 0);
}

int AutoConf::save_defaults(Defaults* defaults)
{
	defaults->update("SHOW_FADE", fade);
	defaults->update("SHOW_PAN", pan);
	defaults->update("SHOW_MUTE", mute);
	defaults->update("SHOW_TRANSITIONS", transitions);
	defaults->update("SHOW_PLUGINS", plugins);
	defaults->update("SHOW_CAMERA", camera);
	defaults->update("SHOW_PROJECTOR", projector);
	defaults->update("SHOW_MODE", mode);
	defaults->update("SHOW_MASK", mask);
	defaults->update("SHOW_CZOOM", czoom);
	defaults->update("SHOW_PZOOM", pzoom);
	return 0;
}

void AutoConf::save_xml(FileXML *file)
{
	file->tag.set_property("SHOW_FADE", fade);
	file->tag.set_property("SHOW_PAN", pan);
	file->tag.set_property("SHOW_MUTE", mute);
	file->tag.set_property("SHOW_TRANSITIONS", transitions);
	file->tag.set_property("SHOW_PLUGINS", plugins);
	file->tag.set_property("SHOW_CAMERA", camera);
	file->tag.set_property("SHOW_PROJECTOR", projector);
	file->tag.set_property("SHOW_MODE", mode);
	file->tag.set_property("SHOW_MASK", mask);
	file->tag.set_property("SHOW_CZOOM", czoom);
	file->tag.set_property("SHOW_PZOOM", pzoom);
}

int AutoConf::set_all()
{
	fade = 1;
	pan = 1;
	mute = 1;
	transitions = 1;
	plugins = 1;
	camera = 1;
	projector = 1;
	mode = 1;
	mask = 1;
	czoom = 1;
	pzoom = 1;
	return 0;
}

AutoConf& AutoConf::operator=(AutoConf &that)
{
	copy_from(&that);
	return *this;
}

void AutoConf::copy_from(AutoConf *src)
{
	fade = src->fade;
	pan = src->pan;
	mute = src->mute;
	transitions = src->transitions;
	plugins = src->plugins;
	camera = src->camera;
	projector = src->projector;
	mode = src->mode;
	mask = src->mask;
	czoom = src->czoom;
	pzoom = src->pzoom;
}
