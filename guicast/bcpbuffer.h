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

	void reset();
// Must be called after BC_WindowBase::enable_opengl to make the PBuffer
// the current drawing surface.  Call BC_WindowBase::enable_opengl
// after this to switch back to the window.
	void enable_opengl();
#ifdef HAVE_GL
	GLXPbuffer get_pbuffer();
#endif

private:
// Called by constructor
	void new_pbuffer(int w, int h);

#ifdef HAVE_GL
	GLXPbuffer pbuffer;
	GLXContext gl_context;
#endif
	int w;
	int h;
	int window_id;
};





#endif
