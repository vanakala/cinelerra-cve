// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef GLTHREAD_H
#define GLTHREAD_H

#include "bcwindowbase.inc"
#include "condition.inc"
#include "glthread.inc"
#include "mutex.inc"
#include "shaders.inc"
#include "thread.h"
#include "vframe.inc"

#include "config.h"

#ifdef HAVE_GL
#include <GL/glx.h>
#endif

#define GL_MAX_CONTEXTS 8
#define GL_MAX_COMMANDS 16

#include <X11/Xlib.h>

// This takes requests and runs all OpenGL calls in the main thread.
// Past experience showed OpenGL couldn't be run from multiple threads
// reliably even if MakeCurrent was used and only 1 thread at a time did 
// anything.

// In addition to synchronous operations, it handles global OpenGL variables.

class GLThreadCommand
{
public:
	GLThreadCommand();

	void dump(int indent = 0, int show_frame = 0);
	const char *name(int command);

	int command;

// Commands
	enum
	{
		NONE,
		QUIT,
// subclasses create new commands starting with this enumeration
		LAST_COMMAND
	};

	VFrame *frame;
	Display *display;
	int screen;
};


class GLThread
{
public:
	GLThread();
	~GLThread();

	int initialize(Display *dpy, Window win, int screen);
	void quit();

	void run();

	GLThreadCommand* new_command();

// Handle extra commands not part of the base class.
// Contains a switch statement starting with LAST_COMMAND
	void handle_command(GLThreadCommand *command);

// Called by ~BC_WindowBase to delete OpenGL objects related to the window.
	void delete_window(Display *dpy, int screen);

private:
	void handle_command_base(GLThreadCommand *command);
	int have_context(Display *dpy, int screen);
	void delete_contexts();

	Condition *next_command;
	Mutex *command_lock;

	Shaders *shaders;
// This quits the program when it's 1.
	int done;
// Command stack
	int last_command;
	GLThreadCommand *commands[GL_MAX_COMMANDS];

#ifdef HAVE_GL
	int last_context;
	struct glctx
	{
		Display *dpy;
		Window win;
		int screen;
		GLXContext gl_context;
		XVisualInfo *visinfo;
	}contexts[GL_MAX_CONTEXTS];
public:
	static void show_glparams(int indent = 0);
	static void show_errors(const char *loc = 0, int indent = 0);
	void show_glxcontext(int context, int indent = 0);
#endif
};
#endif
