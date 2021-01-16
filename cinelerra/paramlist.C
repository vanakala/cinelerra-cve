// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "bcwindowbase.inc"
#include "clip.h"
#include "filexml.h"
#include "paramlist.h"
#include "bcsignals.h"

#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

const char *Param::known_properties[] =
{
	"intval",
	"longval",
	"dblval",
	"type",
	0
};

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
	type = 0;
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
	defaulttype = 0;
	defaultstring = 0;
	defaultint = 0;
	defaultlong = 0;
	defaultfloat = 0;
	prompt = 0;
}

Param::~Param()
{
	delete [] stringvalue;
	delete [] helptext;
	delete [] defaultstring;
	if(subparams)
	{
		delete subparams;
		subparams = 0;
	}
}

void Param::copy_from(Param *that)
{
	copy_top_level(that);

	if(that->subparams)
	{
		add_subparams(that->subparams->name);
		subparams->copy_from(that->subparams);
	}
}

void Param::copy_all(Param *that)
{
	copy_from(that);
	set_help(that->helptext);
}

void Param::copy_top_level(Param *that)
{
	type |= that->type;
	prompt = that->prompt;

	if(that->type & PARAMTYPE_INT)
		intvalue = that->intvalue;
	if(that->type & PARAMTYPE_LNG)
		longvalue = that->longvalue;
	if(that->type & PARAMTYPE_DBL)
		floatvalue = that->floatvalue;
	if(that->type & PARAMTYPE_STR)
		set_string(that->stringvalue);

	if(that->defaulttype)
	{
		defaulttype = that->defaulttype;

		if(defaulttype & PARAMTYPE_INT)
			defaultint = that->defaultint;
		if(defaulttype & PARAMTYPE_LNG)
			defaultlong = that->defaultlong;
		if(defaulttype & PARAMTYPE_DBL)
			defaultfloat = that->defaultfloat;
		delete [] defaultstring;
		defaultstring = 0;
		defaultstr_allocated = 0;

		if(defaulttype & PARAMTYPE_STR && that->defaultstring)
		{
			defaultstring = new char[that->defaultstr_allocated];
			strcpy(defaultstring, that->defaultstring);
		}
	}
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

	type |= PARAMTYPE_STR;
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

	if(type & PARAMTYPE_CODK && subparams && !(subparams->type & PARAMTYPE_HIDN))
	{
		for(Param *p = subparams->first; p; p = p->next)
		{
			if(p->type & PARAMTYPE_INT)
				file->tag.set_property(p->name, p->intvalue);
			if(p->type & PARAMTYPE_LNG)
				file->tag.set_property(p->name, p->longvalue);
			if(p->type & PARAMTYPE_DBL)
				file->tag.set_property(p->name, p->floatvalue);
		}
	}
	file->append_tag();

	if((type & PARAMTYPE_STR) && stringvalue)
		file->append_text(stringvalue);

	file->tag.set_title(string);
	file->append_tag();
	file->append_newline();
}

void Param::load_param(FileXML *file)
{
	int i, j;
	const char *s;

	type = file->tag.get_property("type", type);

	if(type & PARAMTYPE_INT)
		intvalue = file->tag.get_property("intval", intvalue);

	if(type & PARAMTYPE_LNG)
		longvalue = file->tag.get_property("longval", longvalue);

	if(type & PARAMTYPE_DBL)
		floatvalue = file->tag.get_property("dblval", floatvalue);

	if(type & PARAMTYPE_CODK)
	{
		for(i = 0; i < MAX_PROPERTIES; i++)
		{
			s = file->tag.get_property_text(i);
			if(*s == 0)
				break;
			for(j = 0; known_properties[j]; j++)
			{
				if(strcmp(known_properties[j], s) == 0)
					break;
			}
			if(known_properties[j])
				continue;
			if(!subparams)
				add_subparams(name);
			subparams->set(s, file->tag.get_property(s));
		}
	}

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

int Param::equiv(Param *that)
{
	if(!that)
		return 0;

	if((type ^ that->type) & ~PARAMTYPE_CHNG)
		return 0;

	return !equiv_value(that);
}

void Param::store_defaults()
{
	defaulttype = type;
	if(type & PARAMTYPE_INT)
		defaultint = intvalue;
	if(type & PARAMTYPE_LNG)
		defaultlong = longvalue;
	if(type & PARAMTYPE_DBL)
		defaultfloat = floatvalue;
	delete [] defaultstring;
	defaultstring = 0;
	defaultstr_allocated = 0;

	if(type & PARAMTYPE_STR && stringvalue)
	{
		defaultstring = new char[string_allocated];
		defaultstr_allocated = string_allocated;
		strcpy(defaultstring, stringvalue);
	}
}

void Param::set_default(int64_t defaultvalue)
{
	defaulttype = type;

// STR and DBL are not implemented
	if(type & PARAMTYPE_INT)
		defaultint = defaultvalue;
	if(type & PARAMTYPE_LNG)
		defaultlong = defaultvalue;
}

void Param::reset_defaults()
{
	if(defaulttype)
	{
		type = defaulttype;
		if(type & PARAMTYPE_INT)
			intvalue = defaultint;
		if(type & PARAMTYPE_LNG)
			longvalue = defaultlong;
		if(type & PARAMTYPE_DBL)
			floatvalue = defaultfloat;
		if(type & PARAMTYPE_STR)
			set_string(defaultstring);
	}
}

void Param::dump(int indent)
{
	printf("%*sParam '%s' (%p) dump:\n", indent, "", name, this);
	indent += 2;
	printf("%*stype %#x values int:%d long:%" PRId64 " float:%.3f", indent, "",
		type, intvalue, longvalue, floatvalue);
	if(stringvalue)
		printf(" '%s'", stringvalue);
	if(prompt)
		printf(" prompt: '%s'", prompt);
	putchar('\n');
	if(helptext)
		printf("%*sHelp:'%s'\n", indent, "", helptext);
	if(defaulttype)
	{
		printf("%*sdefaults type %#x values int:%d long:%" PRId64 " float:%.3f", indent, "",
			defaulttype, defaultint, defaultlong, defaultfloat);
		if(defaultstring)
			printf(" '%s'", defaultstring);
		putchar('\n');
	}

	if(subparams)
	{
		printf("%*sSubparams:\n", indent, "");
		subparams->dump(indent + 2);
	}
}

Paramlist::Paramlist(const char *name)
 : List<Param>()
{
	if(name)
		strncpy(this->name, name, PARAM_NAMELEN);
	else
		this->name[0] = 0;
	this->name[PARAM_NAMELEN - 1] = 0;
	type = 0;
	selectedint = 0;
	selectedlong = 0;
	selectedfloat = 0;
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

void Paramlist::copy_all(Paramlist *that)
{
	Param *cur, *newparam;

	for(cur = that->first; cur; cur = cur->next)
	{
		newparam = append_param(cur->name);
		newparam->copy_all(cur);
	}
}

void Paramlist::copy_top_level(Paramlist *that)
{
	Param *cur, *newparam;

	for(cur = that->first; cur; cur = cur->next)
	{
		newparam = append_param(cur->name);
		newparam->copy_top_level(cur);
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

Param *Paramlist::find_value(int value)
{
	Param *current;

	for(current = first; current; current = current->next)
	{
		if((current->type & PARAMTYPE_INT) && value == current->intvalue)
			return current;
	}
	return 0;
}

Param *Paramlist::find_value(int64_t value)
{
	Param *current;

	for(current = first; current; current = current->next)
	{
		if((current->type & PARAMTYPE_LNG) && value == current->longvalue)
			return current;
	}
	return 0;
}

Param *Paramlist::find_value(double value)
{
	Param *current;

	for(current = first; current; current = current->next)
	{
		if((current->type & PARAMTYPE_DBL) && EQUIV(value, current->floatvalue))
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

Param *Paramlist::set(Param *param)
{
	Param *pp;

	if(!(pp = find(param->name)))
		pp = append_param(param->name);
	pp->type = param->type;
	pp->copy_values(param);
	return pp;
}

int Paramlist::get(const char *name, int dflt)
{
	Param *pp;

	if((pp = find(name)) && (pp->type & PARAMTYPE_INT))
		return pp->intvalue;
	return dflt;
}

int64_t Paramlist::get(const char *name, int64_t dflt)
{
	Param *pp;

	if((pp = find(name)) && (pp->type & PARAMTYPE_LNG))
		return pp->longvalue;
	return dflt;
}

double Paramlist::get(const char *name, double dflt)
{
	Param *pp;

	if((pp = find(name)) && (pp->type & PARAMTYPE_DBL))
		return pp->floatvalue;
	return dflt;
}

char *Paramlist::get(const char *name, char *dflt)
{
	Param *pp;

	if((pp = find(name)) && (pp->type & PARAMTYPE_STR) && pp->stringvalue)
		strcpy(dflt, pp->stringvalue);
	return dflt;
}

void Paramlist::set_selected(int value)
{
	selectedint = value;
	type |= PARAMTYPE_INT;
}

void Paramlist::set_selected(int64_t value)
{
	selectedlong = value;
	type |= PARAMTYPE_LNG;
}

void Paramlist::set_selected(double value)
{
	selectedfloat = value;
	type |= PARAMTYPE_DBL;
}

void Paramlist::save_list(FileXML *file)
{
	Param *current;
	char *p;
	char string[BCTEXTLEN];

	sprintf(string, "/%s", name);
	file->tag.set_title(string + 1);
	type &= ~PARAMTYPE_CHNG;
	if(type)
	{
		file->tag.set_property("type", type);
		if(type & PARAMTYPE_INT)
			file->tag.set_property("selectedint", selectedint);
		if(type & PARAMTYPE_LNG)
			file->tag.set_property("selectedlong", selectedlong);
		if(type & PARAMTYPE_DBL)
			file->tag.set_property("selectedfloat", selectedfloat);
	}
	file->append_tag();
	file->append_newline();
	for(current = first; current; current = current->next)
		current->save_param(file);
	file->tag.set_title(string);
	file->append_tag();
	file->append_newline();
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
	type = file->tag.get_property("type", 0);
	type &= ~PARAMTYPE_CHNG;

	if(type)
	{
		selectedint = file->tag.get_property("selectedint", 0);
		selectedlong = file->tag.get_property("selectedlong", (int64_t)0);
		selectedfloat = file->tag.get_property("selectedfloat", (double)0);
	}
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

void Paramlist::remove_param(const char *name)
{
	Param *p;

	if(p = find(name))
		remove(p);
}

void Paramlist::join_list(Paramlist *that)
{
	Param *p, *tp, *ttp;

	if(!that)
		return;

	for(tp = that->first; tp;)
	{
		if(p = find(that->name))
			remove(p);

		ttp = tp->next;
		that->remove_pointer(tp);
		append(tp);
		tp = ttp;
	}
	delete that;
}

int Paramlist::equiv(Paramlist *that, int sublvl)
{
	if(!that || total() != that->total() || (type ^ that->type) & ~PARAMTYPE_CHNG)
		return 0;

	if(type & ~PARAMTYPE_CHNG)
	{
		if(type & PARAMTYPE_INT && selectedint != that->selectedint)
			return 0;
		if(type & PARAMTYPE_LNG && selectedlong != that->selectedlong)
			return 0;
		if(type & PARAMTYPE_DBL && !EQUIV(selectedfloat, that->selectedfloat))
			return 0;
	}
	for(Param *current = first; current; current = current->next)
	{
		Param *o = that->find(current->name);

		if(!o)
			return 0;

		if(!current->equiv(o))
			return 0;

		if(sublvl > 0)
		{
			if(current->subparams && o->subparams)
			{
				if(!current->subparams->equiv(o->subparams, sublvl - 1))
					return 0;
			}
			else if(!(!current->subparams && !o->subparams))
				return 0;
		}
	}
	return 1;
}

void Paramlist::store_defaults()
{
	Param *current;

	for(current = first; current; current = current->next)
		current->store_defaults();
}

void Paramlist::reset_defaults()
{
	Param *current;

	for(current = first; current; current = current->next)
		current->reset_defaults();
}

Paramlist *Paramlist::construct(const char *name, Paramlist *plist,
	struct paramlist_defaults *defaults)
{
	Param *parm;

	if(!plist)
		plist = new Paramlist(name);

	for(int i = 0; defaults[i].name; i++)
	{
		if(parm = plist->find(defaults[i].name))
		{
			if(plist->type & (PARAMTYPE_MASK | PARAMTYPE_BITS | PARAMTYPE_BOOL) !=
					defaults[i].type)
			{
				delete parm;
				parm = 0;
			}
		}
		if(!parm)
		{
			if(defaults[i].type & PARAMTYPE_INT)
				parm = plist->append_param(defaults[i].name,
					defaults[i].value);
			else if(defaults[i].type & PARAMTYPE_LNG)
				parm = plist->append_param(defaults[i].name,
					(int64_t)defaults[i].value);
			else
				continue;
		}
		parm->type |= defaults[i].type & ~PARAMTYPE_MASK;
		parm->prompt = defaults[i].prompt;
		// set default to initial default
		parm->set_default(defaults[i].value);
	}
	return plist;
}

Paramlist *Paramlist::construct_from_selection(const char *name, Paramlist *plist,
    const struct selection_int *selection)
{
	Param *parm;

	if(!plist)
		plist = new Paramlist(name);

	for(int i = 0; selection[i].text; i++)
	{
		if(plist->find(selection[i].text))
			continue;
		plist->append_param(selection[i].text, selection[i].value);
	}
	return plist;
}

void Paramlist::save_paramlist(Paramlist *list, const char *filepath,
	struct paramlist_defaults *defaults)
{
	Paramlist *tmp = 0;
	FileXML file;
	Param *parm;

	if(!list)
	{
		unlink(filepath);
		return;
	}

	if(!file.read_from_file(filepath, 1) && !file.read_tag())
	{
		tmp = new Paramlist();
		tmp->load_list(&file);
		if(tmp->equiv(list))
		{
			// nothing changed
			delete tmp;
			return;
		}
		delete tmp;
		tmp = 0;
	}

	file.rewind();

	for(int i = 0; defaults[i].name; i++)
	{
		if(parm = list->find(defaults[i].name))
		{
			switch(parm->type & PARAMTYPE_MASK)
			{
			case PARAMTYPE_INT:
				if(parm->intvalue == defaults[i].value)
					continue;
				if(!tmp)
					tmp = new Paramlist(list->name);
				tmp->append_param(parm->name, parm->intvalue);
				break;
			case PARAMTYPE_LNG:
				if(parm->longvalue == (int64_t)defaults[i].value)
					continue;
				if(!tmp)
					tmp = new Paramlist(list->name);
				tmp->append_param(parm->name, parm->longvalue);
				break;
			}
		}
	}
	if(!tmp)
		unlink(filepath);
	else
	{
		tmp->save_list(&file);
		file.write_to_file(filepath);
		delete tmp;
	}
}

Paramlist *Paramlist::load_paramlist(const char *filepath)
{
	FileXML file;
	Paramlist *list = 0;

	if(!file.read_from_file(filepath, 1) && !file.read_tag())
	{
		list = new Paramlist();
		list->load_list(&file);
	}
	return list;
}

void Paramlist::dump(int indent)
{
	Param *current;
	printf("%*sParamlist '%s' (%p) dump\n", indent, "", name, this);
	printf("%*s type: %#x Selected: int:%d long: %" PRId64 " float:%.2f\n",
		indent, "", type, selectedint, selectedlong, selectedfloat);
	for(current = first; current; current = current->next)
	{
		current->dump(indent + 2);
	}
}
