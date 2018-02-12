
/*
 * CINELERRA
 * Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#ifndef SHADERS_H
#define SHADERS_H

#include "shaders.inc"
#include "linklist.h"
#include "mutex.inc"

class Shader : public ListItem<Shader>
{
public:
	Shader(int type, const char *name, const char *pgm, int shared = 0);
	~Shader();

	const char *typname(int type);
	void dump(int indent = 0);

	int type;
	int shared;
	char name[SHADER_NAME_MAX];
	const char *shadersrc;
};

class Shaders : public List<Shader>
{
public:
	Shaders();
	~Shaders();

	Shader *find(int type, const char *name);
	Shader *add(int type, const char *name, const char *pgm, int shared = 0);
	void delete_shader(int type, const char *name);
	void dump(int indent = 0);

	static Mutex listlock;
};

#endif
