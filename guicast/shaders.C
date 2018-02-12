
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

#include "bcsignals.h"
#include "mutex.h"
#include "shaders.h"

Mutex Shaders::listlock("Shaders::listlock", 1);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Shader::Shader(int type, const char *name, const char *pgm, int shared)
 : ListItem<Shader>()
{
	strncpy(this->name, name, SHADER_NAME_MAX - 1);
	this->shared = shared;
	if(shared)
		shadersrc = pgm;
	else
		shadersrc = strdup(pgm);
	this->type = type;
}

Shader::~Shader()
{
	if(!shared)
		free((char *)shadersrc);
}

void Shader::dump(int indent)
{
	printf("%*s%sshader %p '%s'%s\n", indent, "",
		typname(type), this, name,  shared ? " shared" : "");
	fputs(shadersrc, stdout);
	putc('\n', stdout);
}

const char *Shader::typname(int type)
{
	switch(type)
	{
	case SHADER_NONE:
		return "None";

	case SHADER_VERT:
		return "Verex";

	case SHADER_FRAG:
		return "Fragment";
	}
	return "Unknown";
}

Shaders::Shaders()
 : List<Shader>()
{
}

Shaders::~Shaders()
{
	while(last)
		delete last;
}

Shader *Shaders::find(int type, const char *name)
{
	Shader *rslt = 0;

	listlock.lock("Shaders::find");
	for(Shader *cur = first; cur; cur = cur->next)
	{
		if(type == cur->type && !strcmp(name, cur->name))
		{
			rslt = cur;
			break;
		}
	}
	listlock.unlock();
	return rslt;
}

Shader *Shaders::add(int type, const char *name, const char *pgm, int shared)
{
	Shader *elem;

	listlock.lock("Shaders::add");
	if(!(elem = find(type, name)))
		elem = append(new Shader(type, name, pgm, shared));
	listlock.unlock();
	return elem;
}

void Shaders::delete_shader(int type, const char *name)
{
	Shader *elem;

	listlock.lock("Shaders::delete_shader");
	if(elem = find(type, name))
		delete elem;
	listlock.unlock();
}

void Shaders::dump(int indent)
{
	Shader *cur;

	printf("%*sShaders %p dump:\n", indent, "", this);
	for(cur = first; cur; cur = cur->next)
		cur->dump(indent + 2);
}
