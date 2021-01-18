// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef PARAMLIST_H
#define PARAMLIST_H

#include "filexml.inc"
#include "paramlist.inc"
#include "linklist.h"
#include "selection.h"
#include "stdint.h"

struct paramlist_defaults
{
	const char *name;
	const char *prompt;
	int type;
	int value;
};

class Param : public ListItem<Param>
{
public:
	Param(const char *name, int value);
	Param(const char *name, int64_t value);
	Param(const char *name, const char *value);
	Param(const char *name, double value);
	Param(const char *name);
	~Param();

	void set(const char *value);
	void set(int value);
	void set(int64_t value);
	void set(double value);

	// Copy data
	void copy_from(Param *that);
	// Copy data and help text
	void copy_all(Param *that);
	// Copy data without subparams and help text
	void copy_top_level(Param *that);
	void set_help(const char *txt);
	void set_string(const char *txt);
	Paramlist *add_subparams(const char *name);
	void save_param(FileXML *file);
	void load_param(FileXML *file);
	void copy_values(Param *that);
	int equiv_value(Param *that);
	int equiv(Param *that);
	void store_defaults();
	void reset_defaults();
	void set_default(int64_t defaultvalue);
	void dump(int indent = 0);

	int type;
	char name[PARAM_NAMELEN];
	int help_allocated;
	int string_allocated;
	char *helptext;
	char *stringvalue;
	const char *prompt;
	int intvalue;
	int64_t longvalue;
	double floatvalue;
	Paramlist *subparams;
	static const char *known_properties[];
private:
	void initialize(const char *name);
	// Saved defaults
	int defaulttype;
	int defaultint;
	int defaultstr_allocated;
	char *defaultstring;
	int64_t defaultlong;
	double defaultfloat;
};

class Paramlist : public List<Param>
{
public:
	Paramlist(const char *name = 0);
	~Paramlist();

	void delete_params();
	// Removes subparams and helptexts
	void clean_list();
	// Copy all data
	void copy_from(Paramlist *that);
	void copy_all(Paramlist *that);
	void copy_top_level(Paramlist *that);
	void copy_values(Paramlist *src);
	Param *append_param(const char *name, const char *value);
	Param *append_param(const char *name, int value);
	Param *append_param(const char *name, int64_t value);
	Param *append_param(const char *name, double value);
	Param *append_param(const char *name);

	Param *set(const char *name, const char *value);
	Param *set(const char *name, int value);
	Param *set(const char *name, int64_t value);
	Param *set(const char *name, double value);
	Param *set(Param *param);
	int get(const char *name, int dflt);
	int64_t get(const char *name, int64_t dflt);
	double get(const char *name, double dflt);
	char *get(const char *name, char *dflt);

	void set_selected(int value);
	void set_selected(int64_t value);
	void set_selected(double value);

	Param *find(const char *name);
	Param *find_value(int value);
	Param *find_value(int64_t value);
	Param *find_value(double value);
	void save_list(FileXML *file);
	void load_list(FileXML *file);
	void remove_equiv(Paramlist *that);
	void remove_param(const char *name);
	void join_list(Paramlist *that);
	int equiv(Paramlist *that, int sublvl = 0);
	void store_defaults();
	void reset_defaults();
	static Paramlist *construct(const char *name, Paramlist *plist,
		struct paramlist_defaults *defaults);
	static Paramlist *construct_from_selection(const char *name, Paramlist *plist,
		const struct selection_int *selection);
	static Paramlist *clone(Paramlist *that);
	static void save_paramlist(Paramlist *list, const char *filepath,
		struct paramlist_defaults *defaults);
	static Paramlist *load_paramlist(const char *filepath);
	void dump(int indent = 0);

	char name[PARAM_NAMELEN];
	// Type of values in use here
	int type;
	int selectedint;
	int64_t selectedlong;
	double selectedfloat;
};

#endif
