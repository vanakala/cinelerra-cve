
/*
 * CINELERRA
 * Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#include "bcwindowbase.inc"
#include "clip.h"
#include "filexml.h"
#include "paramlist.h"
#include "bcsignals.h"

#include <inttypes.h>
#include <math.h>
#include <string.h>

Param::Param(const char *name, int value)
 : ListItem<Param>()
{
	initialize(name);
	type = PARAMTYPE_INT;
	intvalue = value;
}

Param::Param(const char *name, int64_t value)
 : ListItem<Param>()
{
	initialize(name);
	type = PARAMTYPE_LNG;
	longvalue = value;
}

Param::Param(const char *name, const char *value)
 : ListItem<Param>()
{
	initialize(name);
	type = PARAMTYPE_STR;
	set_string(value);
}

Param::Param(const char *name, double value)
 : ListItem<Param>()
{
	initialize(name);
	type = PARAMTYPE_DBL;
	floatvalue = value;
}

Param::Param(const char *name)
 : ListItem<Param>()
{
	initialize(name);
	type = 0;
}

void Param::initialize(const char *name)
{
	strncpy(this->name, name, PARAM_NAMELEN);
	this->name[PARAM_NAMELEN - 1] = 0;
	intvalue = 0;
	stringvalue = 0;
	floatvalue = 0;
	helptext = 0;
	help_allocated = 0;
	string_allocated = 0;
	longvalue = 0;
	subparams = 0;
}

Param::~Param()
{
	delete [] stringvalue;
	delete [] helptext;
	if(subparams)
	{
		delete subparams;
		subparams = 0;
	}
}

void Param::copy_from(Param *that)
{
	type |= that->type;

	if(that->type & PARAMTYPE_INT)
		intvalue = that->intvalue;
	if(that->type & PARAMTYPE_LNG)
		longvalue = that->longvalue;
	if(that->type & PARAMTYPE_DBL)
		floatvalue = that->floatvalue;
	if(that->type & PARAMTYPE_STR)
		set_string(that->stringvalue);
}

void Param::set_help(const char *txt)
{
	int l;

	if(txt && (l = strlen(txt)))
	{
		if(help_allocated && l > help_allocated - 1)
			delete [] helptext;
		l++;
		helptext = new char[l];
		help_allocated = l;
		strcpy(helptext, txt);
	}
	else
	{
		delete [] helptext;
		helptext = 0;
		help_allocated = 0;
	}
}

void Param::set_string(const char *txt)
{
	int l;

	if(txt && (l = strlen(txt)))
	{
		if(string_allocated && l > string_allocated - 1)
			delete [] stringvalue;
		l++;
		stringvalue = new char[l];
		string_allocated = l;
		strcpy(stringvalue, txt);
	}
	else
	{
		delete [] stringvalue;
		stringvalue = 0;
		string_allocated = 0;
	}
}

void Param::set(const char *value)
{
	type |= PARAMTYPE_STR;
	set_string(value);
}

void Param::set(int value)
{
	type |= PARAMTYPE_INT;
	intvalue = value;
}

void Param::set(int64_t value)
{
	type |= PARAMTYPE_LNG;
	longvalue = value;
}

void Param::set(double value)
{
	type |= PARAMTYPE_DBL;
	floatvalue = value;
}

Paramlist *Param::add_subparams(const char *name)
{
	if(subparams)
		delete subparams;
	subparams = new Paramlist(name);
	return subparams;
}

void Param::save_param(FileXML *file)
{
	char string[BCTEXTLEN];

	sprintf(string, "/%s", name);
	file->tag.set_title(string + 1);
	file->tag.set_property("type", type);

	if(type & PARAMTYPE_INT)
		file->tag.set_property("intval", intvalue);

	if(type & PARAMTYPE_LNG)
		file->tag.set_property("longval", longvalue);

	if(type & PARAMTYPE_DBL)
		file->tag.set_property("dblval", floatvalue);

	file->append_tag();

	if((type & PARAMTYPE_STR) && stringvalue)
		file->append_text(stringvalue);

	file->tag.set_title(string);
	file->append_tag();
	file->append_newline();
}

void Param::load_param(FileXML *file)
{
	char *s;

	type = file->tag.get_property("type", type);

	if(type & PARAMTYPE_INT)
		intvalue = file->tag.get_property("intval", intvalue);

	if(type & PARAMTYPE_LNG)
		longvalue = file->tag.get_property("longval", longvalue);

	if(type & PARAMTYPE_DBL)
		floatvalue = file->tag.get_property("dblval", floatvalue);

	if(type & PARAMTYPE_STR)
		set_string(file->read_text());
}

void Param::copy_values(Param *that)
{
	if(type & PARAMTYPE_INT)
		intvalue = that->intvalue;

	if(type & PARAMTYPE_LNG)
		longvalue = that->longvalue;

	if(type & PARAMTYPE_DBL)
		floatvalue = that->floatvalue;

	if(type & PARAMTYPE_STR)
		set_string(that->stringvalue);
}

int Param::equiv_value(Param *that)
{
	int res = 0;

	if(type & PARAMTYPE_INT)
		res |= !(intvalue == that->intvalue);

	if(type & PARAMTYPE_LNG)
		res |= !(longvalue == that->longvalue);

	if(type & PARAMTYPE_DBL)
		res |= !EQUIV(floatvalue, that->floatvalue);

	if(type & PARAMTYPE_STR)
	{
		if(stringvalue && that->stringvalue)
			res |= strcmp(stringvalue, that->stringvalue);
		else
			res |= !(!stringvalue && !that->stringvalue);
	}
	return res;
}

void Param::dump(int indent)
{
	printf("%*sParam %p dump:\n", indent, "", this);
	indent += 2;
	printf("%*sName: '%s' type %#x\n", indent, "", name, type);
	printf("%*svalues int:%d long:%" PRId64 " float:%.3f", indent, "",
		intvalue, longvalue, floatvalue);
	if(stringvalue)
		printf(" '%s'", stringvalue);
	putchar('\n');
	if(helptext)
		printf("%*sHelp:'%s'\n", indent, "", helptext);

	if(subparams)
	{
		printf("%*sSubparams:\n", indent, "");
		subparams->dump(indent + 2);
	}
}

Paramlist::Paramlist(const char *name)
 : List<Param>()
{
	strncpy(this->name, name, PARAM_NAMELEN);
	this->name[PARAM_NAMELEN - 1] = 0;
	selectedint = 0;
}

Paramlist::~Paramlist()
{
	delete_params();
}

void Paramlist::copy_from(Paramlist *that)
{
	Param *cur, *newparam;

	for(cur = that->first; cur; cur = cur->next)
	{
		newparam = append_param(cur->name);
		newparam->copy_from(cur);
	}
}

void Paramlist::delete_params()
{
	while(last)
		delete last;
}

void Paramlist::clean_list()
{
	Param *cur;

	for(cur = first; cur; cur = cur->next)
	{
		if(cur->subparams)
		{
			delete cur->subparams;
			cur->subparams = 0;
		}
		if(cur->helptext)
			cur->set_help(0);
	}
}

void Paramlist::copy_values(Paramlist *src)
{
	Param *cp1, *cp2;

	if(!src)
		return;

	for(cp1 = first; cp1; cp1 = cp1->next)
	{
		for(cp2 = src->first; cp2; cp2 = cp2->next)
		{
			if(strcmp(cp1->name, cp2->name) == 0)
			{
				cp1->copy_values(cp2);
				break;
			}
		}
	}
}

Param *Paramlist::append_param(const char *name, const char *value)
{
	return append(new Param(name, value));
}

Param *Paramlist::append_param(const char *name, int value)
{
	return append(new Param(name, value));
}

Param *Paramlist::append_param(const char *name, int64_t value)
{
	return append(new Param(name, value));
}

Param *Paramlist::append_param(const char *name, double value)
{
	return append(new Param(name, value));
}

Param *Paramlist::append_param(const char *name)
{
	return append(new Param(name));
}

Param *Paramlist::find(const char *name)
{
	Param *current;

	for(current = first; current; current = current->next)
	{
		if(strcmp(current->name, name) == 0)
			return current;
	}
	return 0;
}

Param  *Paramlist::set(const char *name, const char *value)
{
	Param *pp;

	if(pp = find(name))
		pp->set(value);
	else
		pp = append_param(name, value);
	return pp;
}

Param  *Paramlist::set(const char *name, int value)
{
	Param *pp;

	if(pp = find(name))
		pp->set(value);
	else
		pp = append_param(name, value);
	return pp;
}

Param  *Paramlist::set(const char *name, int64_t value)
{
	Param *pp;

	if(pp = find(name))
		pp->set(value);
	else
		pp = append_param(name, value);
	return pp;
}

Param  *Paramlist::set(const char *name, double value)
{
	Param *pp;

	if(pp = find(name))
		pp->set(value);
	else
		pp = append_param(name, value);
	return pp;
}

void Paramlist::save_list(FileXML *file)
{
	Param *current;
	char string[BCTEXTLEN];

	sprintf(string, "/%s", name);
	file->tag.set_title(string + 1);
	file->tag.set_property("selectedint", selectedint);
	file->append_tag();
	file->append_newline();
	for(current = first; current; current = current->next)
		current->save_param(file);
	file->tag.set_title(string);
	file->append_tag();
	file->append_newline();
	file->terminate_string();
}

void Paramlist::load_list(FileXML *file)
{
	Param *current;
	const char *t;

	if(file->read_tag())
		return;

	// name of the list
	strncpy(name, file->tag.get_title(), PARAM_NAMELEN);
	name[PARAM_NAMELEN - 1] = 0;
	selectedint = file->tag.get_property("selectedint", 0);

	while(!file->read_tag())
	{
		t = file->tag.get_title();

		if(!strcmp(t + 1, name))
			break;

		if(*t != '/')
		{
			current = append_param(t);
			current->load_param(file);
		}
	}
}

void Paramlist::remove_equiv(Paramlist *that)
{
	Param *cp1, *cp2;

	if(!that)
		return;

	for(cp2 = that->first; cp2; cp2 = cp2->next)
	{
		if(cp1 = find(cp2->name))
		{
			if(!cp1->equiv_value(cp2))
				remove(cp1);
		}
	}
}

void Paramlist::dump(int indent)
{
	Param *current;
	printf("%*sParamlist %s (%p) dump\n", indent, "", name, this);
	printf("%*s Selected: int %d\n", indent, "", selectedint);
	for(current = first; current; current = current->next)
	{
		current->dump(indent + 2);
	}
}
