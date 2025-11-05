// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2025 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef GLGUIDES_H
#define GLGUIDES_H

#include "glguides.inc"
#include "glthread.inc"

#ifdef HAVE_GL
#include <GL/glx.h>
#endif

class GLGuides
{
public:
	GLGuides();
	~GLGuides();

	void set_glthread(GLThread *ptr) { glthread = ptr; }
	void add_guide(GLThreadCommand *command);
	void draw(struct glctx *current_glctx);

private:
	int allocate_guides();
	double x_to_output(double x);
	double y_to_output(double y);

	struct glctx *glctx;
	GLThread *glthread;
	GLThreadCommand *guides;
	GLuint guidevxshader;
	int lastguide;
	int guides_alloc;
};

#endif
