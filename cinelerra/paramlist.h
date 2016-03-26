
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

#ifndef PARAMLIST_H
#define PARAMLIST_H

#include "paramlist.inc"
#include "linklist.h"
#include "stdint.h"

class Param : public ListItem<Param>
{
public:
	Param(const char *name, int value);
	Param(const char *name, int64_t value);
	Param(const char *name, const char *value);
	Param(const char *name, double value);
	~Param();

	void set_help(const char *txt);
	void set_string(const char *txt);
	Paramlist *add_subparams(const char *name);
	void dump(int indent = 0);

	int type;
	char name[PARAM_NAMELEN];
	int help_allocated;
	int string_allocated;
	char *helptext;
	char *stringvalue;
	int intvalue;
	int64_t longvalue;
	double floatvalue;
	Paramlist *subparams;

private:
	void initialize(const char *name);
};

class Paramlist : public List<Param>
{
public:
	Paramlist(const char *name);
	~Paramlist();

	void delete_params();
	Param *append_param(const char *name, const char *value);
	Param *append_param(const char *name, int value);
	Param *append_param(const char *name, int64_t value);
	Param *append_param(const char *name, double value);
	Param *find(const char *name);
	void dump(int indent = 0);

	char name[PARAM_NAMELEN];
	int selectedint;
};

#endif
