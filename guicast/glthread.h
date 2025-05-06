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
#define GL_VERTICES_SIZE 28

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

struct glctx;

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
	double zoom;
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

	int get_glx_version(BC_WindowBase *window);
	void quit();
#ifdef HAVE_GL
	void display_vframe(VFrame *frame, BC_WindowBase *window,
		struct gl_window *inwin, struct gl_window *outwin, double zoom);
	void release_resources();
	void disable_opengl(BC_WindowBase *window);
#endif
	void run();

	GLThreadCommand* new_command();

// Handle extra commands not part of the base class.
// Contains a switch statement starting with LAST_COMMAND
	void handle_command(GLThreadCommand *command);

// Called by ~BC_WindowBase to delete OpenGL objects related to the window.
	void delete_window(Display *dpy, int screen);

private:
	void delete_contexts();
	void handle_command_base(GLThreadCommand *command);
#ifdef HAVE_GL
	int initialize(Display *dpy, Window win, int screen);
	GLuint create_texture(int num, int width, int height);
	void generate_renderframe();
	void set_viewport(GLThreadCommand *command);
// executing commands
	void do_display_vframe(GLThreadCommand *command);
	void do_disable_opengl(GLThreadCommand *command);

	Shaders *shaders;
#endif

	Condition *next_command;
	Mutex *command_lock;
// This quits the program when it's 1.
	int done;
// Command stack
	int last_command;
	GLThreadCommand *commands[GL_MAX_COMMANDS];

#ifdef HAVE_GL

	struct texture
	{
		int width;
		int height;
		GLuint id;
	};
	int last_context;
	struct glctx
	{
		Display *dpy;
		Window win;
		int screen;
		GLXContext gl_context;
		XVisualInfo *visinfo;
		int last_texture;
		struct texture textures[GL_MAX_TEXTURES];
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
		float vertices[GL_VERTICES_SIZE];
	}contexts[GL_MAX_CONTEXTS];
	struct glctx *current_glctx;
	struct glctx *have_context(Display *dpy, int screen);
	void do_release_resources(struct glctx *ctx);
public:
	static void show_glparams(int indent = 0);
	static void show_errors(const char *loc = 0, int indent = 0);
	void show_glxcontext(struct glctx *ctx, int indent = 0);
	void show_compile_status(GLuint shader, const char *name);
	void show_link_status(GLuint program, const char *name);
	void check_framebuffer_status(int indent = 0);
	void show_renderbuffer_params(GLuint id, int indent);
	void show_fb_attachment(GLuint objid, GLint objtype,
		const char *atname, int indent = 0);
	const char *textureparamstr(GLuint id);
	const char *renderbufferparamstr(GLuint id);
	void show_framebuffer_status(GLuint fbo, int indent = 0);
	void show_validation(GLuint program);
	void show_program_params(GLuint program, int indent = 0);
	void show_shaders(GLuint program, int indent = 0);
	void show_uniforms(GLuint program, int indent = 0);
	const char *glname(GLenum type);

#endif
};
#endif
