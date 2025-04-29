// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>

#define GL_GLEXT_PROTOTYPES
#include "bcresources.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "clip.h"
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
R"vx(
    void yuv2rgb(inout vec3 yuv)
    {
	yuv -= vec3(0, 0.5, 0.5);
	const mat3 yuv_to_rgb_matrix = mat3(
	    1,       1,        1,
	    0,       -0.34414, 1.77200,
	    1.40200, -0.71414, 0);
	yuv = yuv_to_rgb_matrix * yuv;
    };
)vx";

static int attrib[] =
{
	GLX_RGBA,
	GLX_RED_SIZE, 8,
	GLX_GREEN_SIZE, 8,
	GLX_BLUE_SIZE, 8,
	GLX_ALPHA_SIZE, 8,
	GLX_DOUBLEBUFFER,
	None
};

// Vertex for texture
static float vertices[GL_VERTICES_SIZE] = {
//  Position      Color             Texcoords
//   x       y      r    g     b     s     t
    -1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // Top-left      0..6
     1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, // Top-right     7..13
     1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // Bottom-right 14..20
    -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f  // Bottom-left  21..27
};

static GLuint elements[] = {
        0, 1, 2,
        2, 3, 0
};

static float brd_color[] = { 0.0f, 0.0f, 0.0f, 0.0f };

static const char *vertex_shader =
R"vx(#version 130
    in vec2 position;
    in vec3 color;
    in vec2 texcoord;
    out vec3 Color;
    out vec2 Texcoord;
    void main()
    {
	Color = color;
	Texcoord = texcoord;
	gl_Position = vec4(position, 0.0, 1.0);
    }
)vx";

static const char *fragment_shader =
R"vx(#version 130
    in vec3 Color;
    in vec2 Texcoord;
    out vec4 outColor;
    uniform sampler2D tex;
    void main()
    {
	outColor = texture(tex, Texcoord) * vec4(Color, 1.0);
    }
)vx";
#endif

const struct gl_commands GLThreadCommand::gl_names[] =
{
	{ "None", NONE },
	{ "Quit", QUIT },
	{ "Disable", DISABLE },
	{ "Display Frame", DISPLAY_VFRAME },
	{ "Release Resources", RELEASE_RESOURCES },
	{ 0, 0 }
};

GLThreadCommand::GLThreadCommand()
{
	command = GLThreadCommand::NONE;
	frame = 0;
	dpy = 0;
	win = 0;
	zoom = 1;
	screen = -1;
}

void GLThreadCommand::dump(int indent, int show_frame)
{
	printf("%*sGLThreadCommand '%s' %p dump:\n", indent, "", name(command), this);
	printf("%*sframe: %p display %p win %#lx screen %d\n", indent, "",
		frame, dpy, win, screen);
	switch(command)
	{
	case DISPLAY_VFRAME:
		printf("%*s[%d,%d] zoom:%.3f win1:(%.1f,%.1f) (%.1f,%.1f)", indent, "",
			width, height, zoom, glwin1.x1, glwin1.y1, glwin1.x2, glwin1.y2);
		printf(" win2:(%.1f,%.1f) (%.1f,%.1f)\n",
			glwin2.x1, glwin2.y1, glwin2.x2, glwin2.y2);
		break;
	}
	if(show_frame && frame)
		frame->dump(indent);
}

const char *GLThreadCommand::name(int command)
{
	for(int i = 0; gl_names[i].name; i++)
		if(gl_names[i].value == command)
			return gl_names[i].name;
	return "Unknown";
}

GLThread::GLThread()
{
	int maj, min;

	next_command = new Condition(0, "GLThread::next_command", 0);
	command_lock = new Mutex("GLThread::command_lock");
	done = 0;
	last_command = 0;
	memset(commands, 0, sizeof(GLThreadCommand*) * GL_MAX_COMMANDS);
#ifdef HAVE_GL
	shaders = 0;
	last_context = 0;
	current_glctx = 0;
	memset(contexts, 0, sizeof(GLXContext) * GL_MAX_CONTEXTS);
#endif
	BC_WindowBase::get_resources()->set_glthread(this);
}

GLThread::~GLThread()
{
	command_lock->lock("GLThread::~GLThread");
#ifdef HAVE_GL
	delete shaders;
#endif
	for(int i = 0; i < GL_MAX_COMMANDS; i++)
		delete commands[i];
	command_lock->unlock();
	delete next_command;
	delete command_lock;
}

int GLThread::get_glx_version(BC_WindowBase *window)
{
	int maj, min;

#ifdef HAVE_GL
	if(glXQueryVersion(window->get_display(), &maj, &min))
		return 100 * maj + min;
#endif
	return 0;
}

GLThreadCommand* GLThread::new_command()
{
	while(last_command >= GL_MAX_COMMANDS)
	{
		command_lock->lock("GLThread::new_command");
		command_lock->unlock();
	}
	if(!commands[last_command])
		commands[last_command] = new GLThreadCommand;
	return commands[last_command++];
}

#ifdef HAVE_GL
int GLThread::initialize(Display *dpy, Window win, int screen)
{
	XVisualInfo *visinfo;
	GLXContext gl_context;
	struct glctx *ctx = 0;

	if(!(current_glctx = have_context(dpy, screen)))
	{
		for(int i = 0; i < GL_MAX_CONTEXTS; i++)
		{
			if(contexts[i].win == win && contexts[i].dpy == 0)
			{
				ctx = &contexts[i];
				break;
			}
		}
		if(!ctx)
		{
			if(last_context >= GL_MAX_CONTEXTS)
				return 1;
			ctx = &contexts[last_context];
			last_context++;
		}
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
		ctx->dpy = dpy;
		ctx->screen = screen;
		ctx->win = win;
		ctx->gl_context = gl_context;
		ctx->visinfo = visinfo;
		current_glctx = ctx;
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
}
#endif

#ifdef HAVE_GL
void GLThread::generate_renderframe()
{
	if(current_glctx)
	{
		// Create Vertex Array Object
		glGenVertexArrays(1, &current_glctx->vertexarray);
		glBindVertexArray(current_glctx->vertexarray);
		// Create a Vertex Buffer Object and copy the vertex data to it
		glGenBuffers(1, &current_glctx->vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, current_glctx->vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, GL_VERTICES_SIZE * sizeof(float),
			current_glctx->vertices, GL_STATIC_DRAW);
		// Create an element array
		glGenBuffers(1, &current_glctx->elemarray);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, current_glctx->elemarray);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements),
			elements, GL_STATIC_DRAW);
		current_glctx->vertexshader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(current_glctx->vertexshader, 1, &vertex_shader, NULL);
		glCompileShader(current_glctx->vertexshader);
		// Create and compile the fragment shader
		current_glctx->fragmentshader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(current_glctx->fragmentshader, 1, &fragment_shader, NULL);
		glCompileShader(current_glctx->fragmentshader);
		// Link the vertex and fragment shader into a shader program
		current_glctx->shaderprogram = glCreateProgram();
		glAttachShader(current_glctx->shaderprogram, current_glctx->vertexshader);
		glAttachShader(current_glctx->shaderprogram, current_glctx->fragmentshader);
		glBindFragDataLocation(current_glctx->shaderprogram, 0, "outColor");
		glLinkProgram(current_glctx->shaderprogram);
		glUseProgram(current_glctx->shaderprogram);
		// Specify the layout of the vertex data
		current_glctx->posattrib = glGetAttribLocation(
			current_glctx->shaderprogram, "position");
		glEnableVertexAttribArray(current_glctx->posattrib);
		glVertexAttribPointer(current_glctx->posattrib, 2, GL_FLOAT, GL_FALSE,
			7 * sizeof(GLfloat), 0);
		current_glctx->colattrib = glGetAttribLocation(
			current_glctx->shaderprogram, "color");
		glEnableVertexAttribArray(current_glctx->colattrib);
		glVertexAttribPointer(current_glctx->colattrib, 3, GL_FLOAT, GL_FALSE,
			7 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
		current_glctx->texattrib = glGetAttribLocation(
			current_glctx->shaderprogram, "texcoord");
		glEnableVertexAttribArray(current_glctx->texattrib);
		glVertexAttribPointer(current_glctx->texattrib, 2, GL_FLOAT, GL_FALSE,
			7 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));
		// Texture
		// 1st texture
		glGenTextures(1, &current_glctx->firsttexture);
		glBindTexture(GL_TEXTURE_2D, current_glctx->firsttexture);
		// the equivalent of (x,y,z) in texture coordinates is called (s,t,r).
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, brd_color);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
}

GLThread::glctx *GLThread::have_context(Display *dpy, int screen)
{
	for(int i = 0; i < last_context; i++)
	{
		if(contexts[i].dpy == dpy && contexts[i].screen == screen)
			return &contexts[i];
	}
	return 0;
}
#endif

void GLThread::quit()
{
	command_lock->lock("GLThread::quit");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::QUIT;
	command_lock->unlock();
	next_command->unlock();
}

#ifdef HAVE_GL
void GLThread::display_vframe(VFrame *frame, BC_WindowBase *window,
	struct gl_window *inwin, struct gl_window *outwin, double zoom)
{
	command_lock->lock("GLThread::display_vframe");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::DISPLAY_VFRAME;
	command->frame = frame;
	command->dpy = window->top_level->display;
	command->win = window->win;
	command->screen = window->top_level->screen;
	command->width = window->get_w();
	command->height = window->get_h();
	command->zoom = zoom;
	command->glwin1 = *inwin;
	command->glwin2 = *outwin;
	command_lock->unlock();
	next_command->unlock();
}

void GLThread::release_resources()
{
	command_lock->lock("GLThread::release_resources");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::RELEASE_RESOURCES;
	command_lock->unlock();
	next_command->unlock();
}

void GLThread::disable_opengl(BC_WindowBase *window)
{
	command_lock->lock("GLThread::release_resources");
	GLThreadCommand *command = new_command();
	command->command = GLThreadCommand::DISABLE;
	command->dpy = window->top_level->display;
	command->win = window->win;
	command->screen = window->top_level->screen;
	command_lock->unlock();
	next_command->unlock();
}
#endif

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
#ifdef HAVE_GL
		case GLThreadCommand::DISPLAY_VFRAME:
			do_display_vframe(command);
			break;

		case GLThreadCommand::RELEASE_RESOURCES:
			do_release_resources(current_glctx);
			break;

		case GLThreadCommand::DISABLE:
			do_disable_opengl(command);
			break;
#endif
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

#ifdef HAVE_GL
void GLThread::do_disable_opengl(GLThreadCommand *command)
{
	if(GLThread::glctx *ctx = have_context(command->dpy, command->screen))
	{
		if(ctx->vertexarray)
			do_release_resources(ctx);
		XFree(ctx->visinfo);
		glXMakeCurrent(ctx->dpy, None, NULL);
		glXDestroyContext(ctx->dpy, ctx->gl_context);
		ctx->dpy = 0;
		ctx->gl_context = 0;
		if(ctx == current_glctx)
			current_glctx = 0;
	}
}

void GLThread::do_display_vframe(GLThreadCommand *command)
{
	if(initialize(command->dpy, command->win, command->screen))
		return;
	if(!current_glctx->vertexarray)
	{
		set_viewport(command);
		generate_renderframe();
	}
	glBindTexture(GL_TEXTURE_2D, current_glctx->firsttexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, command->frame->get_w(),
		command->frame->get_h(),
		0, GL_RGBA, GL_UNSIGNED_SHORT, command->frame->get_data());
	// Clear the screen to black
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(current_glctx->shaderprogram);
	// Draw a rectangle from the 2 triangles using 6 indices
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glXSwapBuffers(command->dpy, command->win);
}
#endif

#ifdef HAVE_GL
GLuint GLThread::create_texture(int num, int width, int height)
{
	struct texture *txp;

	txp = &current_glctx->textures[num];
	glGenTextures(1, &txp->id);
	txp->width = width;
	txp->height = height;
	glBindTexture(GL_TEXTURE_2D, txp->id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
		GL_UNSIGNED_SHORT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	return txp->id;
}

void GLThread::do_release_resources(struct glctx *ctx)
{
	glDeleteTextures(1, &ctx->firsttexture);
	glDeleteProgram(ctx->shaderprogram);
	glDeleteShader(ctx->fragmentshader);
	glDeleteShader(ctx->vertexshader);
	glDeleteBuffers(1, &ctx->elemarray);
	glDeleteBuffers(1, &ctx->vertexbuffer);
	glDeleteVertexArrays(1, &ctx->vertexarray);
	ctx->vertexarray = 0;
}

void GLThread::set_viewport(GLThreadCommand *command)
{
	double aspect = command->frame->get_pixel_aspect();
	int w = round(command->frame->get_w() * aspect);
	int h = command->frame->get_h();
	int x = round(command->glwin2.x1);
	int y = round(command->glwin2.y1) + command->height - h;

	if(!aspect)
		aspect = 1.0;
	memcpy(current_glctx->vertices, vertices, GL_VERTICES_SIZE * sizeof(float));
	if(!EQUIV(command->glwin1.x1, 1.0))
	{
		double s = command->glwin1.x1 / w * aspect;
		current_glctx->vertices[5] += s;
		current_glctx->vertices[12] += s;
		current_glctx->vertices[19] += s;
		current_glctx->vertices[26] += s;
	}
	if(!EQUIV(command->glwin1.y1, 1.0))
	{
		double s = command->glwin1.y1 / h;
		current_glctx->vertices[6] += s;
		current_glctx->vertices[13] += s;
		current_glctx->vertices[20] += s;
		current_glctx->vertices[27] += s;
	}
	if(!EQUIV(command->zoom, 1.0))
	{
		float sz = 2 * command->zoom;

		current_glctx->vertices[14] = current_glctx->vertices[7] =
			current_glctx->vertices[0] + sz;
		current_glctx->vertices[22] = current_glctx->vertices[15] =
			current_glctx->vertices[1] - sz;
	}
	glViewport(x, y, w, h);
}
#endif

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

void GLThread::show_glxcontext(struct glctx *ctx, int indent)
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
	Display *dpy = ctx->dpy;
	XVisualInfo *visinfo = ctx->visinfo;

	glXQueryVersion(dpy, &maj, &min);
	printf("%*sGLX version %d.%d context %p\n", indent, "", maj, min, ctx);
	printf("%*sDirect rendering: %s\n", indent, "",
		glXIsDirect(dpy, ctx->gl_context) ? "Yes" : "No");
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

void GLThread::show_compile_status(GLuint shader, const char *name)
{
	GLint status;
	char msg_buffer[BCTEXTLEN];

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	printf("Compilation of shader '%s' %s\n", name,
		status ? "succeeded" : "failed");
	msg_buffer[0] = 0;
	glGetShaderInfoLog(shader, BCTEXTLEN, NULL, msg_buffer);
	if(msg_buffer[0])
	{
		fputs("Logi:\n", stdout);
		fputs(msg_buffer, stdout);
	}
}

void GLThread::show_link_status(GLuint program, const char *name)
{
	GLint status;
	char msg_buffer[BCTEXTLEN];

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	printf("Linking of program '%s(%d)' %s\n", name, program,
		status ? "succeeded" : "failed");
	if(!status)
	{
		glGetShaderInfoLog(program, BCTEXTLEN, NULL, msg_buffer);
		fputs("Logi:\n", stdout);
		fputs(msg_buffer, stdout);
	}
}

void GLThread::show_program_params(GLuint program, int indent)
{
	GLint param;

	printf("%*sProgram %d params:\n", indent, "", program);
	glGetProgramiv(program, GL_DELETE_STATUS, &param);
	indent += 2;
	printf("%*sDelete status %d\n", indent, "", param);
	glGetProgramiv(program, GL_LINK_STATUS, &param);
	printf("%*sLink status %d\n", indent, "", param);
	glGetProgramiv(program, GL_VALIDATE_STATUS, &param);
	printf("%*sValidate status %d\n", indent, "", param);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &param);
	printf("%*sInfo log length %d\n", indent, "", param);
	glGetProgramiv(program, GL_ATTACHED_SHADERS, &param);
	printf("%*sAttached shaders %d\n", indent, "", param);
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &param);
	printf("%*sActive attributes %d\n", indent, "", param);
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &param);
	printf("%*sActive attribute max length %d\n", indent, "", param);
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &param);
	printf("%*sActive uniforms %d\n", indent, "", param);
	glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &param);
	printf("%*sActive uniform max length %d\n", indent, "", param);
}

void GLThread::show_shaders(GLuint program, int indent)
{
	GLuint shaders[20];
	GLsizei count;

	if(!glIsProgram(program))
	{
		printf("%d is not a program\n", program);
		return;
	}
	glGetAttachedShaders(program, 20, &count, shaders);
	printf("%*sProgram %d has %d shaders:\n", indent, "", program, count);
	indent += 2;
	for(int i = 0; i < count; i++)
		printf("%*sattached shader: %u\n", indent, "",  shaders[i]);
}

void GLThread::show_uniforms(GLuint program, int indent)
{
	GLint n, max, i;

	if(!glIsProgram(program))
	{
		printf("%d is not a program\n", program);
		return;
	}
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &n);
	glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max);
	printf("%*sProgram %d has %d uniforms with max name %d:\n", indent, "",
		program, n, max);

	indent += 2;
	for(int i = 0; i < n; i++)
	{
		GLint size, len;
		GLenum type;
		char name[100];
		glGetActiveUniform(program, i, 100, &len, &size, &type, name);

		printf("%*s%d: '%s' nameLen: %d size: %d type: '%s'\n", indent, "",
			i, name, len, size, glname(type));
	}
}

const char *GLThread::glname(GLenum type)
{
	static char bufr[64];

	switch(type)
	{
	case GL_BLEND_EQUATION_RGB:
		return "Blend equation RGB";
	case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
		return "Vertex attrib array enabled";
	case GL_VERTEX_ATTRIB_ARRAY_SIZE:
		return "Vertex attrib array size";
	case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
		return "Vertex attrib array stride";
	case GL_VERTEX_ATTRIB_ARRAY_TYPE:
		return "Vertex attrib array";
	case GL_CURRENT_VERTEX_ATTRIB:
		return "Current vertex attrib";
	case GL_VERTEX_PROGRAM_POINT_SIZE:
		return "Vertex program point size";
	case GL_VERTEX_ATTRIB_ARRAY_POINTER:
		return "Vertex attrib array pointer";
	case GL_STENCIL_BACK_FUNC:
		return "Stencil back func";
	case GL_STENCIL_BACK_FAIL:
		return "Stencil back fail";
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
		return "Stencil back pass depth fail";
	case GL_STENCIL_BACK_PASS_DEPTH_PASS:
		return "Stencil back pass depth pass";
	case GL_MAX_DRAW_BUFFERS:
		return "Max draw buffers";
	case GL_DRAW_BUFFER0:
		return "Draw buffer 0";
	case GL_DRAW_BUFFER1:
		return "Draw buffer 1";
	case GL_DRAW_BUFFER2:
		return "Draw buffer 2";
	case GL_DRAW_BUFFER3:
		return "Draw buffer 3";
	case GL_DRAW_BUFFER4:
		return "Draw buffer 4";
	case GL_DRAW_BUFFER5:
		return "Draw buffer 5";
	case GL_DRAW_BUFFER6:
		return "Draw buffer 6";
	case GL_DRAW_BUFFER7:
		return "Draw buffer 7";
	case GL_DRAW_BUFFER8:
		return "Draw buffer 8";
	case GL_DRAW_BUFFER9:
		return "Draw buffer 9";
	case GL_DRAW_BUFFER10:
		return "Draw buffer 10";
	case GL_DRAW_BUFFER11:
		return "Draw buffer 11";
	case GL_DRAW_BUFFER12:
		return "Draw buffer 12";
	case GL_DRAW_BUFFER13:
		return "Draw buffer 13";
	case GL_DRAW_BUFFER14:
		return "Draw buffer 15";
	case GL_BLEND_EQUATION_ALPHA:
		return "Blend equation alpha";
	case GL_MAX_VERTEX_ATTRIBS:
		return "Max vertex attribs";
	case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
		return "Vertex attrib array normalized";
	case GL_MAX_TEXTURE_IMAGE_UNITS:
		return "Max texture image units";
	case GL_FRAGMENT_SHADER:
		return "Fragment shader";
	case GL_VERTEX_SHADER:
		return "Vertex shader";
	case GL_MAX_FRAGMENT_UNIFORM_COMPONENTS:
		return "Max fragment uniform components";
	case GL_MAX_VERTEX_UNIFORM_COMPONENTS:
		return "Max vertex uniform components";
	case GL_MAX_VARYING_FLOATS:
		return "Max varying floats";
	case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
		return "Max vertex texture image units";
	case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
		return "Max combined texture image units";
	case GL_SHADER_TYPE:
		return "Shader type";
	case GL_FLOAT_VEC2:
		return "Float vec2";
	case GL_FLOAT_VEC3:
		return "Float vec3";
	case GL_FLOAT_VEC4:
		return "Float vec4";
	case GL_INT_VEC2:
		return "Int vec2";
	case GL_INT_VEC3:
		return "Int vec3";
	case GL_INT_VEC4:
		return "Int vec4";
	case GL_BOOL:
		return "Bool";
	case GL_BOOL_VEC2:
		return "Bool vec2";
	case GL_BOOL_VEC3:
		return "Bool vec3";
	case GL_BOOL_VEC4:
		return "Bool vec4";
	case GL_FLOAT_MAT2:
		return "Float mat2";
	case GL_FLOAT_MAT3:
		return "Float mat3";
	case GL_FLOAT_MAT4:
		return "Float mat4";
	case GL_SAMPLER_1D:
		return "Sampler 1D";
	case GL_SAMPLER_2D:
		return "Sampler 2D";
	case GL_SAMPLER_3D:
		return "Sampler 3D";
	case GL_SAMPLER_CUBE:
		return "Sampler cube";
	case GL_SAMPLER_1D_SHADOW:
		return "Sampler 1D shadow";
	case GL_SAMPLER_2D_SHADOW:
		return "Sampler 2D shadow";
	default:
		sprintf(bufr, "%#x", type);
		return bufr;
	}
}
#endif
