// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2025 Einar Rünkaru <einarrunkaru@gmail dot com>

#define GL_GLEXT_PROTOTYPES
#include "config.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "glguides.h"
#include "glthread.h"

#define ALLOC_CHUNK 16

static const char *vertex_guides =
R"vx(#version 130
	in vec2 position;
	void main()
	{
		gl_Position = vec4(position, 0.0, 1.0);
	}
)vx";

GLGuides::GLGuides()
{
	glthread = 0;
	guides = 0;
	lastguide = 0;
	guides_alloc = 0;
	guidevxshader = 0;
}

GLGuides::~GLGuides()
{
	free(guides);
}

int GLGuides::allocate_guides()
{
	if(guides_alloc < lastguide + 1)
	{
		int newalloc = (guides_alloc + ALLOC_CHUNK) * sizeof(GLThreadCommand);
		GLThreadCommand *newguides = (GLThreadCommand *)malloc(newalloc);

		if(!newguides)
			return 0;

		for(int i = 0; i < lastguide; i++)
			newguides[i] = guides[i];

		free(guides);
		guides = newguides;
		guides_alloc = newalloc;
	}
	return 1;
}

void GLGuides::add_guide(GLThreadCommand *command)
{
	if(allocate_guides())
		guides[lastguide++] = *command;
}

void GLGuides::draw(struct glctx *current_glctx)
{
	glctx = current_glctx;
	if(lastguide)
	{
		float rect[GL_RECTANGLE_SIZE];

		if(!guidevxshader)
		{
			GLint posattrib;

			guidevxshader = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(guidevxshader, 1, &vertex_guides, NULL);
			glCompileShader(guidevxshader);
//			glthread->show_compile_status(guidevxshader, "guidevxshader");
			glDetachShader(current_glctx->shaderprogram, current_glctx->vertexshader);
			glAttachShader(current_glctx->shaderprogram, guidevxshader);
			glDetachShader(current_glctx->shaderprogram, current_glctx->fragmentshader);
			glLinkProgram(current_glctx->shaderprogram);
			glLineWidth(1);
			glEnable(GL_COLOR_LOGIC_OP);
			glLogicOp(GL_XOR);
			glEnableVertexAttribArray(0);
		}
		for(int i = 0; i < lastguide; i++)
		{
			switch(guides[i].command)
			{
			case GLThreadCommand::GUIDE_RECTANGLE:
				// left
				rect[0] = x_to_output(guides[i].glwin1.x1 / guides[i].glwin2.x2);
				// top
				rect[1] = y_to_output(guides[i].glwin1.y1 / guides[i].glwin2.y2);
				// right
				rect[2] = x_to_output((guides[i].glwin1.x1 +
					guides[i].glwin1.x2) / guides[i].glwin2.x2);
				rect[3] = rect[1];
				rect[4] = rect[2];
				// bottom
				rect[5] = y_to_output((guides[i].glwin1.y1 +
					guides[i].glwin1.y2) / guides[i].glwin2.y2);
				rect[6] = rect[0];
				rect[7] = rect[5];
				glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
				glBindBuffer(GL_ARRAY_BUFFER, current_glctx->vertexbuffer);
				glBufferData(GL_ARRAY_BUFFER, sizeof(rect), rect, GL_STATIC_DRAW);
				glDrawArrays(GL_LINE_LOOP, 0, 4);
				break;
			default:
				printf("Command '%s' is not implemented yet\n",
					guides[i].name(guides[i].command));
				break;
			}
		}
		lastguide = 0;
		glDetachShader(current_glctx->shaderprogram, guidevxshader);
		glDeleteShader(guidevxshader);
		guidevxshader = 0;
		glDisable(GL_COLOR_LOGIC_OP);
		glDisableVertexAttribArray(0);
	}
	glthread->show_errors("do_rectangle", 4);
	glthread->show_program_params(current_glctx->shaderprogram, 4);
	glthread->show_shaders(current_glctx->shaderprogram, 4);
	glthread->show_uniforms(current_glctx->shaderprogram, 4);
	glthread->show_attributes(current_glctx->shaderprogram, 4);
}

double GLGuides::x_to_output(double x)
{
	return glctx->vertices[0] +
		(x - glctx->vertices[5]) *
		(glctx->vertices[7] - glctx->vertices[0]);
}

double GLGuides::y_to_output(double y)
{
	return glctx->vertices[1] -
		(y - glctx->vertices[6]) *
		(glctx->vertices[8] - glctx->vertices[22]);
}
