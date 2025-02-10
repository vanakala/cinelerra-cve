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
#define GL_ORIG_TEXTURE 0
#define GL_MAX_TEXTURES 4

#include <X11/Xlib.h>

struct gl_commands
{
	const char *name;
	int value;
};

struct gl_window
{
	double x1, y1;
	double x2, y2;
};

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
		DISABLE,
		DISPLAY_VFRAME,
		RELEASE_RESOURCES,
// subclasses create new commands starting with this enumeration
		LAST_COMMAND
	};

	VFrame *frame;
	Display *dpy;
	Window win;
	int screen;
	int width;
	int height;
	struct gl_window glwin1;
	struct gl_window glwin2;
private:
	static const struct gl_commands gl_names[];
};


class GLThread
{
public:
	GLThread();
	~GLThread();

	int initialize(Display *dpy, Window win, int screen);
	int get_glx_version(BC_WindowBase *window);
	void quit();
	void display_vframe(VFrame *frame, BC_WindowBase *window,
		struct gl_window *inwin, struct gl_window *outwin);
	void release_resources();
	void disable_opengl(BC_WindowBase *window);

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
	GLuint create_texture(int num, int width, int height);
	void generate_renderframe();
// executing commands
	void do_display_vframe(GLThreadCommand *command);
	void do_release_resources();
	void do_disable_opengl(GLThreadCommand *command);

	Condition *next_command;
	Mutex *command_lock;

	Shaders *shaders;
// This quits the program when it's 1.
	int done;
// Command stack
	int last_command;
	GLThreadCommand *commands[GL_MAX_COMMANDS];

#ifdef HAVE_GL
	GLuint vertexarray;   // vao
	GLuint vertexbuffer;  // vbo
	GLuint elemarray;     // ebo
	GLuint vertexshader;
	GLuint fragmentshader;
	GLuint shaderprogram;
	GLuint firsttexture;  // tex
	GLuint fbtexture;
	GLint posattrib;
	GLint colattrib;
	GLint texattrib;

	struct texture
	{
		int width;
		int height;
		GLuint id;
	};
	int last_context;
	int current_context;
	struct glctx
	{
		Display *dpy;
		Window win;
		int screen;
		GLXContext gl_context;
		XVisualInfo *visinfo;
		int last_texture;
		struct texture textures[GL_MAX_TEXTURES];
	}contexts[GL_MAX_CONTEXTS];
public:
	static void show_glparams(int indent = 0);
	static void show_errors(const char *loc = 0, int indent = 0);
	void show_glxcontext(int context, int indent = 0);
	void show_compile_status(GLuint shader, const char *name);
#endif
};
#endif
