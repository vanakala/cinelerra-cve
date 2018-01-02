
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#ifndef BCPBUFFER_H
#define BCPBUFFER_H

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef HAVE_GL
#include <GL/glx.h>
#endif

#include "vframe.inc"


// This is created by the user to create custom PBuffers and by VFrame.
// Uses the window currently bound by BC_WindowBase::enable_opengl to
// create pbuffer.
// Must be called synchronously.
class BC_PBuffer
{
public:
	BC_PBuffer(int w, int h);
	~BC_PBuffer();

	friend class VFrame;
// Must be called after BC_WindowBase::enable_opengl to make the PBuffer
// the current drawing surface.  Call BC_WindowBase::enable_opengl
// after this to switch back to the window.
// Returns nonzero if fails
	int enable_opengl();
#ifdef HAVE_GL
	GLXPbuffer get_pbuffer();
#endif
	void dump_FbConfig(Display *dpy, int screen,
		GLXFBConfig config, int indent = 0);
private:
// Called by constructor
	void new_pbuffer(int w, int h);

// Used by dump_FbConfig
	int GetFBConfigAttrib(Display *dpy, int screen,
		GLXFBConfig config, int attrib);

#ifdef HAVE_GL
	GLXPbuffer pbuffer;
	GLXContext gl_context;
#endif
	int w;
	int h;
	int window_id;
};





#endif
