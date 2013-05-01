
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

#include "bcdisplayinfo.h"
#include "clip.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>

#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "bcsignals.h"
#include "colormodels.h"

#define TEST_X 100
#define TEST_Y 100

int BC_DisplayInfo::top_border = -1;
int BC_DisplayInfo::left_border = -1;
int BC_DisplayInfo::bottom_border = -1;
int BC_DisplayInfo::right_border = -1;
int BC_DisplayInfo::auto_reposition_x = -1;
int BC_DisplayInfo::auto_reposition_y = -1;

static struct testdata
{
    int w;
    int h;
} test_sizes[] = {
    { 128, 128 },
    { 164, 164 },
    { 196, 196 }
};

#define TESTDATASIZE (sizeof(test_sizes) / sizeof(struct testdata))

struct xv_adapterinfo *BC_DisplayInfo::xv_adapters = 0;
int BC_DisplayInfo::num_adapters = -1;

static struct fourcc_formats
{
	uint32_t fourcc;
	int cmodel;
}
xv_formats[] =
{
	{ FOURCC_YV12, BC_YUV420P },
	{ FOURCC_YUV2, BC_YUV422 }
};

#define XV_NUM_SUPPORTED_CMODELS (sizeof(xv_formats) / sizeof(struct fourcc_formats))

BC_DisplayInfo::BC_DisplayInfo(const char *display_name, int show_error)
{
	init_window(display_name, show_error);
}

BC_DisplayInfo::~BC_DisplayInfo()
{
	XCloseDisplay(display);
}

void BC_DisplayInfo::parse_geometry(char *geom, int *x, int *y, int *width, int *height)
{
	XParseGeometry(geom, x, y, (unsigned int*)width, (unsigned int*)height);
}

/*
 * Try to figure out how many pixels window maneger shifts window
 */
void BC_DisplayInfo::test_window(int &x_out, 
	int &y_out, 
	int &x_out2, 
	int &y_out2)
{
	unsigned long mask = CWEventMask;
	XSetWindowAttributes attr;
	XSizeHints size_hints;

	x_out = 0;
	y_out = 0;
	x_out2 = 0;
	y_out2 = 0;
	attr.event_mask = StructureNotifyMask;

	Window win = XCreateWindow(display, 
			rootwin, 
			TEST_X,
			TEST_Y,
			test_sizes[0].w,
			test_sizes[0].h,
			0, 
			default_depth, 
			InputOutput, 
			vis, 
			mask, 
			&attr);

	memset(&size_hints, 0, sizeof(size_hints));
	size_hints.flags = PPosition | PSize | PMinSize | PMaxSize;
	size_hints.x = TEST_X;
	size_hints.y = TEST_Y;
	size_hints.width = test_sizes[1].w;
	size_hints.height = test_sizes[1].h;
	size_hints.min_width = test_sizes[0].w;
	size_hints.min_height = test_sizes[0].h;
	size_hints.max_width = test_sizes[TESTDATASIZE - 1].w;
	size_hints.max_height = test_sizes[TESTDATASIZE - 1].h;
	XSetNormalHints(display, win, &size_hints);

	XMapWindow(display, win); 
	XFlush(display);
	XSync(display, 0);

	XMoveResizeWindow(display, 
		win, 
		TEST_X,
		TEST_Y,
		test_sizes[1].w,
		test_sizes[1].h);
	XFlush(display);
	XSync(display, 0);

	for(int i = 2; i < TESTDATASIZE; i++)
	{
		XResizeWindow(display,
			win,
			test_sizes[i].w,
			test_sizes[i].h);
		XFlush(display);
		XSync(display, 0);
	}

	int state = 0;
	XEvent event;
	struct testdata shfts[TESTDATASIZE];
	memset(shfts, 0, sizeof(shfts));

	do
	{
		XNextEvent(display, &event);
		if(event.type == ConfigureNotify && event.xany.window == win)
		{
			if(test_sizes[state + 1].w == event.xconfigure.width && test_sizes[state + 1].h == event.xconfigure.height)
				state++;
			shfts[state].w = event.xconfigure.x + event.xconfigure.border_width;
			shfts[state].h = event.xconfigure.y + event.xconfigure.border_width;
		}
	}while(state < TESTDATASIZE - 1);

	XDestroyWindow(display, win);
	XFlush(display);
	XSync(display, 0);
// Create shift
	x_out = shfts[0].w - TEST_X;
	y_out = shfts[0].h - TEST_Y;
// Reposition shift
	x_out2 = shfts[1].w - shfts[0].w;
	y_out2 = shfts[1].h - shfts[0].h;

	CLAMP(x_out, 0, 60);
	CLAMP(y_out, 0, 60);
	CLAMP(x_out2, 0, 60);
	CLAMP(y_out2, 0, 60);
}

void BC_DisplayInfo::init_borders()
{
	if(top_border < 0)
	{
		test_window(left_border, 
			top_border, 
			auto_reposition_x, 
			auto_reposition_y);
		right_border = left_border;
		bottom_border = left_border;
	}
}

int BC_DisplayInfo::get_top_border()
{
	init_borders();
	return top_border;
}

int BC_DisplayInfo::get_left_border()
{
	init_borders();
	return left_border;
}

int BC_DisplayInfo::get_right_border()
{
	init_borders();
	return right_border;
}

int BC_DisplayInfo::get_bottom_border()
{
	init_borders();
	return bottom_border;
}

void BC_DisplayInfo::init_window(const char *display_name, int show_error)
{
	if(display_name && display_name[0] == 0) display_name = NULL;

	if((display = XOpenDisplay(display_name)) == NULL)
	{
		if(show_error)
		{
			printf("BC_DisplayInfo::init_window: cannot connect to X server.\n");
			if(getenv("DISPLAY") == NULL)
				printf("'DISPLAY' environment variable not set.\n");
			exit(1);
		}
		return;
	}

	screen = DefaultScreen(display);
	rootwin = RootWindow(display, screen);
	vis = DefaultVisual(display, screen);
	default_depth = DefaultDepth(display, screen);
}

int BC_DisplayInfo::get_root_w()
{
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	return WidthOfScreen(screen_ptr);
}

int BC_DisplayInfo::get_root_h()
{
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	return HeightOfScreen(screen_ptr);
}

int BC_DisplayInfo::get_abs_cursor_x()
{
	int abs_x, abs_y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

	XQueryPointer(display,
		rootwin,
		&temp_win,
		&temp_win,
		&abs_x,
		&abs_y,
		&win_x,
		&win_y,
		&temp_mask);
	return abs_x;
}

int BC_DisplayInfo::get_abs_cursor_y()
{
	int abs_x, abs_y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

	XQueryPointer(display,
		rootwin,
		&temp_win,
		&temp_win,
		&abs_x,
		&abs_y,
		&win_x,
		&win_y,
		&temp_mask);
	return abs_y;
}

int BC_DisplayInfo::test_xvext()
{
	int port, ret;
	XvAdaptorInfo *ai;
	unsigned int adpt;
	XvAttribute *at;
	int attrs, fmts;
	unsigned int encs;
	struct xv_adapterinfo *curadapt, *ca, *cb;
	XSetWindowAttributes attr;
	XvImageFormatValues *fo;
	XSizeHints size_hints;
	XvEncodingInfo *ei;
	int n;

	if(BC_DisplayInfo::num_adapters >= 0)
		return BC_DisplayInfo::num_adapters > 0;
	BC_DisplayInfo::num_adapters = 0;

	if((ret = XvQueryExtension(display, &xv_version, &xv_release, &xv_req_base,
			&xv_event_base, &xv_error_base)) != Success)
	{
		if(ret == XvBadAlloc)
			printf("BC_DisplayInfo::test_xvext: XvBadAlloc error\n");
		return 0;
	}

	if(XvQueryAdaptors(display, DefaultRootWindow(display), &adpt, &ai) != Success)
		return 0;

	BC_DisplayInfo::xv_adapters = new xv_adapterinfo[adpt];
	BC_DisplayInfo::num_adapters = adpt;
	memset(BC_DisplayInfo::xv_adapters, 0, sizeof(struct xv_adapterinfo) * adpt);

	n = 0;
	for(int i = 0; i < adpt; i++)
	{
		if((ai[i].type & XvImageMask) == 0)
			continue;

		curadapt = &BC_DisplayInfo::xv_adapters[n++];
		curadapt->adapter_name = strdup(ai[i].name);
		curadapt->type = ai[i].type;
		curadapt->base_port = ai[i].base_id;
		curadapt->num_ports = ai[i].num_ports;
		curadapt->attributes[XV_ATTRIBUTE_BRIGHTNESS].name = "XV_BRIGHTNESS";
		curadapt->attributes[XV_ATTRIBUTE_CONTRAST].name = "XV_CONTRAST";
		curadapt->attributes[XV_ATTRIBUTE_AUTOPAINT_COLORKEY].name = "XV_AUTOPAINT_COLORKEY";
		curadapt->attributes[XV_ATTRIBUTE_COLORKEY].name = "XV_COLORKEY";
	}
	XvFreeAdaptorInfo(ai);

	if(BC_DisplayInfo::xv_adapters[0].adapter_name == 0)
		return 0;

	for(curadapt = BC_DisplayInfo::xv_adapters; curadapt < &BC_DisplayInfo::xv_adapters[adpt]; curadapt++)
	{
		if(!curadapt->adapter_name)
			break;

		port = curadapt->base_port;

		if(XvQueryEncodings(display, port, &encs, &ei) == Success)
		{
			curadapt->width = ei[0].width;
			curadapt->height = ei[0].height;
			XvFreeEncodingInfo(ei);
		}
		else
		{
			curadapt->type = 0;
			continue;
		}

		if(fo = XvListImageFormats(display, port, &fmts))
		{
			for(int i = 0; i < fmts; i++)
			{
				for(int k = 0; k < XV_NUM_SUPPORTED_CMODELS; k++)
					if(fo[i].id == xv_formats[k].fourcc)
					{
						curadapt->cmodels[curadapt->num_cmodels++] = xv_formats[k].cmodel;
						break;
					}
				if(curadapt->num_cmodels >= XV_MAX_CMODELS)
					break;
			}
			XFree(fo);
		}
		if(curadapt->num_cmodels == 0)
		{
			curadapt->type = 0;
			continue;
		}

		if(at = XvQueryPortAttributes(display, port, &attrs))
		{
			for(int i = 0; i < attrs; i++)
			{
				for(int j = 0; j < NUM_XV_ATTRIBUTES; j++)
				{
					if(strcmp(at[i].name, curadapt->attributes[j].name) == 0)
					{
						curadapt->attributes[j].flags = at[i].flags;
						curadapt->attributes[j].min = at[i].min_value;
						curadapt->attributes[j].max = at[i].max_value;
						curadapt->attributes[j].xatom =
							XInternAtom(display, curadapt->attributes[j].name, False);
						if(curadapt->attributes[j].xatom != None
							&& (curadapt->attributes[j].flags & XvGettable))
						XvGetPortAttribute(display, port,
							curadapt->attributes[j].xatom, &curadapt->attributes[j].value);
					}
				}
			}
			XFree(at);
		}
	}
	n = 0;
	for(curadapt = BC_DisplayInfo::xv_adapters; curadapt < &BC_DisplayInfo::xv_adapters[adpt]; curadapt++)
	{
		if(!curadapt->adapter_name)
			break;

		if(curadapt->type)
		{
			n++;
			continue;
		}
		free(curadapt->adapter_name);
		cb = curadapt;
		for(ca = curadapt + 1; ca < &BC_DisplayInfo::xv_adapters[adpt]; ca++)
			*cb++ = *ca;
		curadapt--;
	}
	if(n == 0)
		return 0;
	return 1;
}

uint32_t BC_DisplayInfo::cmodel_to_fourcc(int cmodel)
{
	for(int i = 0; i < XV_NUM_SUPPORTED_CMODELS; i++)
		if(xv_formats[i].cmodel == cmodel)
			return xv_formats[i].fourcc;
}

void BC_DisplayInfo::dump_xvext()
{
	struct xv_adapterinfo *adp;

	if(BC_DisplayInfo::num_adapters < 0)
	{
		printf("BC_DisplayInfo::Xvext dump: data is uninitalized\n");
		return;
	}
	printf("BC_DisplayInfo::Xvext dump: number of adapters %d\n", BC_DisplayInfo::num_adapters);

	for(int i = 0; i < BC_DisplayInfo::num_adapters; i++)
	{
		adp = BC_DisplayInfo::xv_adapters;
		printf("Adapter #%d: '%s'\n", i, adp->adapter_name);
		printf("    %d ports starting with %d, size %dx%d\n",
			adp->num_ports, adp->base_port, adp->width, adp->height);
		printf("    Supports %d colormodels:", adp->num_cmodels);
		for(int j = 0; j < adp->num_cmodels; j++)
			printf(" %s", cmodel_name(adp->cmodels[j]));
		printf("\n    Attributes:");
		for(int j = 0; j < NUM_XV_ATTRIBUTES; j++)
			if(adp->attributes[j].flags)
				printf(" %s(%c%c:%d)", adp->attributes[j].name,
					adp->attributes[j].flags & XvGettable ? 'r' : '-',
					adp->attributes[j].flags & XvSettable ? 'w' : '-',
					adp->attributes[j].value);
		putchar('\n');
	}
}
