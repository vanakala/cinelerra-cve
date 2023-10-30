// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>

#define GL_GLEXT_PROTOTYPES
#include "bcresources.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "condition.h"
#include "glthread.h"
#include "shaders.h"
#include "mutex.h"
#include "vframe.h"

#include <unistd.h>

#include <string.h>

#ifdef HAVE_GL

// YUV -> RGB converter
static const char *yuv_to_rgb_frag =
	"void yuv2rgb(inout vec3 yuv)\n"
	"{\n"
	"  yuv -= vec3(0, 0.5, 0.5);\n"
	"  const mat3 yuv_to_rgb_matrix = mat3(\n"
	"  1,       1,        1, \n"
	"  0,       -0.34414, 1.77200, \n"
	"  1.40200, -0.71414, 0);\n"
	"  yuv = yuv_to_rgb_matrix * yuv;\n"
	"}\n";

static int attrib[] =
{
	GLX_RGBA,
	GLX_RED_SIZE, 1,
	GLX_GREEN_SIZE, 1,
	GLX_BLUE_SIZE, 1,
	GLX_DOUBLEBUFFER,
	None
};

#endif

GLThreadCommand::GLThreadCommand()
{
	command = GLThreadCommand::NONE;
	frame = 0;
	display = 0;
	screen = 0;
}

void GLThreadCommand::dump(int indent, int show_frame)
{
	printf("%*sGLThreadCommand '%s' %p dump:\n", indent, "", name(command), this);
	printf("%*sdisplay: %p screen %d frame: %p\n", indent, "",
		display, screen, frame);
	if(show_frame && frame)
		frame->dump();
}

const char *GLThreadCommand::name(int command)
{
	switch(command)
	{
	case NONE:
		return "None";

	case QUIT:
		return "Quit";
	}
	return "Unknown";
}

GLThread::GLThread()
{
	next_command = new Condition(0, "GLThread::next_command", 0);
	command_lock = new Mutex("GLThread::command_lock");
	done = 0;
	shaders = 0;
	last_command = 0;
	memset(commands, 0, sizeof(GLThreadCommand*) * GL_MAX_COMMANDS);
#ifdef HAVE_GL
	last_context = 0;
	memset(contexts, 0, sizeof(GLXContext) * GL_MAX_CONTEXTS);
#endif
	BC_WindowBase::get_resources()->set_glthread(this);
}

GLThread::~GLThread()
{
	command_lock->lock("GLThread::~GLThread");
	delete shaders;
	for(int i = 0; i < GL_MAX_COMMANDS; i++)
		delete commands[i];
	command_lock->unlock();
	delete next_command;
	delete command_lock;
}

GLThreadCommand* GLThread::new_command()
{
	if(last_command >= GL_MAX_COMMANDS)
	{
		// jääda ootama
		return 0;
	}
	if(!commands[last_command])
		commands[last_command] = new GLThreadCommand;
	return commands[last_command++];
}

int GLThread::initialize(Display *dpy, Window win, int screen)
{
#ifdef HAVE_GL
	XVisualInfo *visinfo;
	GLXContext gl_context;
	if(have_context(dpy, screen) < 0)
	{
		if(last_context >= GL_MAX_CONTEXTS)
			return 1;
		if(!(visinfo = glXChooseVisual(dpy, screen, attrib)))
		{
			fputs("GLThread::initialize: Couldn't get visual.\n", stdout);
			return 1;
		}
		if(!(gl_context = glXCreateContext(dpy, visinfo, 0, GL_TRUE)))
		{
			fputs("GLThread::initialize: Couldn't create OpenGL context.\n", stdout);
			XFree(visinfo);
			return 1;
		}
		contexts[last_context].dpy = dpy;
		contexts[last_context].screen = screen;
		contexts[last_context].win = win;
		contexts[last_context].gl_context = gl_context;
		contexts[last_context].visinfo = visinfo;
		last_context++;
		glXMakeCurrent(dpy, win, gl_context);
		if(!BC_Resources::OpenGLStrings[0])
		{
			const char *string;
			if(string = (const char*)glGetString(GL_VERSION))
				BC_Resources::OpenGLStrings[0] = strdup(string);
			if(string = (const char*)glGetString(GL_VENDOR))
				BC_Resources::OpenGLStrings[1] = strdup(string);
			if(string = (const char*)glGetString(GL_RENDERER))
				BC_Resources::OpenGLStrings[2] = strdup(string);
		}
	}

	if(!shaders)
	{
		shaders = new Shaders();
		shaders->add(SHADER_FRAG, "yuv2rgb", yuv_to_rgb_frag, 1);
	}
	// FIXIT: needs texture, fb
	return 0;
#else
	return 1;
#endif
}

int GLThread::have_context(Display *dpy, int screen)
{
#ifdef HAVE_GL
	for(int i = 0; i < last_context; i++)
	{
		if(contexts[i].dpy == dpy && contexts[i].screen == screen)
			return i;
	}
#endif
	return -1;
}

void GLThread::quit()
{
	command_lock->lock("GLThread::quit");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::QUIT;
	command_lock->unlock();

	next_command->unlock();
}

void GLThread::run()
{
	while(!done)
	{
		next_command->lock("GLThread::run");

		command_lock->lock("GLThread::run");
		for(int i = 0; i < last_command; i++)
		{
			GLThreadCommand *command = commands[i];
			handle_command_base(command);
		}
		last_command = 0;
		command_lock->unlock();
	}
}

void GLThread::handle_command_base(GLThreadCommand *command)
{
	if(command)
	{
		switch(command->command)
		{
		case GLThreadCommand::QUIT:
			delete_contexts();
			done = 1;
			break;

		default:
			handle_command(command);
			break;
		}
	}
}

void GLThread::handle_command(GLThreadCommand *command)
{
}

void GLThread::delete_contexts()
{
#ifdef HAVE_GL
	for(int i = 0; i < last_context; i++)
	{
		if(contexts[i].dpy)
		{
			XFree(contexts[i].visinfo);
			glXDestroyContext(contexts[i].dpy, contexts[i].gl_context);
			contexts[i].dpy = 0;
			contexts[i].gl_context = 0;
		}
	}
#endif
}

void GLThread::delete_window(Display *dpy, int screen)
{
#ifdef HAVE_GL
	int i;

	if((i = have_context(dpy, screen)) >= 0)
	{
		XFree(contexts[i].visinfo);
		glXMakeCurrent(dpy, None, NULL);
		glXDestroyContext(contexts[i].dpy, contexts[i].gl_context);
		contexts[i].gl_context = 0;
		contexts[i].dpy = 0;
	}
#endif
}

#ifdef HAVE_GL
// Debug functions
void GLThread::show_glparams(int indent)
{
	GLint auxbufnum;
	GLint max3dtexsiz;
	GLint maxdrawbufs;
	GLint maxtexiunits;
	GLint maxtexsize;
	GLint maxtexunits;
	GLint maxcomtexunits;
	GLint viewportdims[2];
	GLint viewport[4];

	glGetIntegerv(GL_AUX_BUFFERS, &auxbufnum);
	glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3dtexsiz);
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxdrawbufs);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxtexiunits);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtexsize);
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &maxtexunits);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxcomtexunits);
	glGetIntegerv(GL_MAX_VIEWPORT_DIMS, viewportdims);
	glGetIntegerv(GL_VIEWPORT, viewport);
	printf("%*sSupported GLSL version is %s.\n", indent, "",
		(char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
	indent++;
	printf("%*sSome GL parameter values:\n", indent, "");
	indent += 2;
	printf("%*sNumber of auxiliary color buffers available: %d\n", indent, "",
		auxbufnum);
	printf("%*sLargest 3D texture that the GL can handle: %d\n", indent, "",
		max3dtexsiz);
	printf("%*sMaximum number of simultaneous output colors: %d\n", indent, "",
		maxdrawbufs);
	printf("%*sMaximum supported texture image units: %d\n", indent, "",
		maxtexiunits);
	printf("%*sLargest texture that the GL can handle: %d\n", indent, "",
		maxtexsize);
	printf("%*sNumber of conventional texture units supported: %d\n", indent, "",
		maxtexunits);
	printf("%*sMaximum supported combined texture image units: %d\n", indent, "",
		maxcomtexunits);
	printf("%*sMaximum supported width and height of the viewport %dx%d\n", indent, "",
		viewportdims[0], viewportdims[1]);
	printf("%*sCurrent viewport: (%d,%d) [%d,%d]\n", indent, "",
		viewport[0], viewport[1],
		viewport[2], viewport[3]);
}

void GLThread::show_glxcontext(int context, int indent)
{
	int use_gl;
	int buffer_size;
	int level;
	int rgba;
	int doublebuffer;
	int stereo;
	int aux_bufs;
	int red, green, blue, alpha, depth, stencil;
	int acured, acugreen, acublue, acualpha;
	int maj, min;
	Display *dpy = contexts[context].dpy;
	XVisualInfo *visinfo = contexts[context].visinfo;

	glXQueryVersion(dpy, &maj, &min);
	printf("%*sGLX version %d.%d context %d\n", indent, "", maj, min, context);
	printf("%*sDirect rendering: %s\n", indent, "",
		glXIsDirect(dpy, contexts[context].gl_context) ? "Yes" : "No");
	glXGetConfig(dpy, visinfo, GLX_USE_GL, &use_gl);
	glXGetConfig(dpy, visinfo, GLX_BUFFER_SIZE, &buffer_size);
	glXGetConfig(dpy, visinfo, GLX_LEVEL, &level);
	glXGetConfig(dpy, visinfo, GLX_RGBA, &rgba);
	glXGetConfig(dpy, visinfo, GLX_DOUBLEBUFFER, &doublebuffer);
	glXGetConfig(dpy, visinfo, GLX_STEREO, &rgba);
	glXGetConfig(dpy, visinfo, GLX_AUX_BUFFERS, &aux_bufs);
	glXGetConfig(dpy, visinfo, GLX_RED_SIZE, &red);
	glXGetConfig(dpy, visinfo, GLX_GREEN_SIZE, &green);
	glXGetConfig(dpy, visinfo, GLX_BLUE_SIZE, &blue);
	glXGetConfig(dpy, visinfo, GLX_ALPHA_SIZE, &alpha);
	glXGetConfig(dpy, visinfo, GLX_DEPTH_SIZE, &depth);
	glXGetConfig(dpy, visinfo, GLX_STENCIL_SIZE, &stencil);
	glXGetConfig(dpy, visinfo, GLX_ACCUM_RED_SIZE, &acured);
	glXGetConfig(dpy, visinfo, GLX_ACCUM_GREEN_SIZE, &acugreen);
	glXGetConfig(dpy, visinfo, GLX_ACCUM_BLUE_SIZE, &acublue);
	glXGetConfig(dpy, visinfo, GLX_ACCUM_ALPHA_SIZE, &acualpha);

	indent += 2;
	printf("%*sGlx context: use_gl %d buffer_size %d buffer_level %d\n", indent, "",
		use_gl, buffer_size, level);
	printf("%*srgba %d doublebuffer %d stereo %d aux_bufs %d\n", indent, "",
		rgba, doublebuffer, stereo, aux_bufs);
	printf("%*sred %d green %d blue %d alpha %d depth %d stencil %d\n", indent, "",
		red, green, blue, alpha, depth, stencil);
	printf("%*saccum red %d green %d blue %d alpha %d\n", indent, "",
		acured, acugreen, acublue, acualpha);
}

void GLThread::show_errors(const char *loc, int indent)
{
	GLenum err;
	const char *s = 0;

	while((err = glGetError()) != GL_NO_ERROR)
	{
		switch(err)
		{
		case GL_INVALID_ENUM:
			s = "Invalid enum";
			break;
		case GL_INVALID_VALUE:
			s = "Invalid value";
			break;
		case GL_INVALID_OPERATION:
			s = "Invalid operation";
			break;
		case GL_STACK_OVERFLOW:
			s = "Stack overflow";
			break;
		case GL_STACK_UNDERFLOW:
			s = "Stack underflow";
			break;
		case GL_OUT_OF_MEMORY:
			s = "Out of memory";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			s = "Invalid framebuffer operation";
			break;
		case GL_CONTEXT_LOST:
			s = "Context lost";
			break;
		case GL_TABLE_TOO_LARGE:
			s = "Table too large";
			break;
		default:
			s = "Unknown";
			break;
		}
		printf("%*sOpenGL ERROR: %s.\n", indent, "", s);
	}
	if(loc)
	{
		if(s)
			printf("%*sEnd of error list at '%s'\n", indent, "", loc);
		else
			printf("%*sNo errors at '%s'\n", indent, "", loc);
	}
	else
	{
		if(s)
			printf("%*sEnd of error list\n", indent, "");
		else
			printf("%*sNo opengl errors\n", indent, "");
	}
}

#endif
