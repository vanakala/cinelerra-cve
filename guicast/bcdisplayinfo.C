// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
#define TEST_SIZE 128

int BC_DisplayInfo::top_border = -1;
int BC_DisplayInfo::left_border = -1;
int BC_DisplayInfo::bottom_border = -1;
int BC_DisplayInfo::right_border = -1;
int BC_DisplayInfo::auto_reposition_x = -1;
int BC_DisplayInfo::auto_reposition_y = -1;

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
// This function must be the first Xlib
// function a multi-threaded program calls
	if(!XInitThreads())
	{
		printf("Failed to init X threads\n");
		exit(1);
	}
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
 * Try to figure out how many pixels window manager shifts window
 */
void BC_DisplayInfo::test_window(int &x_out, 
	int &y_out, 
	int &x_out2, 
	int &y_out2)
{
	unsigned long mask = CWEventMask;
	XSetWindowAttributes attr;
	XSizeHints size_hints;
	char *txlist[2];
	XTextProperty titleprop;

	x_out = 0;
	y_out = 0;
	x_out2 = 0;
	y_out2 = 0;
	attr.event_mask = StructureNotifyMask;

	Window win = XCreateWindow(display, 
			rootwin, 
			TEST_X,
			TEST_Y,
			TEST_SIZE,
			TEST_SIZE,
			0, 
			default_depth, 
			InputOutput, 
			vis, 
			mask, 
			&attr);
	size_hints.flags = PPosition | PSize;
	size_hints.x = TEST_X;
	size_hints.y = TEST_Y;
	size_hints.width = TEST_SIZE;
	size_hints.height = TEST_SIZE;
	// Set the name of the window
	// Makes possible to create special config for the window
	txlist[0] = (char *)"guicast_test";
	txlist[1] = 0;
	XmbTextListToTextProperty(display, txlist, 1,
		XStdICCTextStyle, &titleprop);
	XSetWMProperties(display, win, &titleprop, &titleprop,
		0, 0, &size_hints, 0, 0);
	XFree(titleprop.value);

	XMapWindow(display, win); 
	XFlush(display);
	XSync(display, 0);
	// Wait until WM reacts
	usleep(20000);
	XDestroyWindow(display, win);
	XFlush(display);
	XSync(display, 0);

	int xm = -1, ym = -1;
	XEvent event;

	for(;;)
	{
		XNextEvent(display, &event);
		if(event.type == ConfigureNotify && event.xconfigure.window == win)
		{
			if(xm < event.xconfigure.x)
				xm = event.xconfigure.x;
			if(ym < event.xconfigure.y)
				ym = event.xconfigure.y;
		}
		if(event.type == DestroyNotify && event.xdestroywindow.window == win)
			break;
	}
// Create shift
	if(xm >= 0)
	{
		x_out = xm - TEST_X;
		y_out = ym - TEST_Y;
	}
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

void BC_DisplayInfo::get_root_size(int *width, int *height)
{
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);

	*width = WidthOfScreen(screen_ptr);
	*height = HeightOfScreen(screen_ptr);
}

void BC_DisplayInfo::get_abs_cursor(int *abs_x, int *abs_y)
{
	int win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

	XQueryPointer(display,
		rootwin,
		&temp_win,
		&temp_win,
		abs_x,
		abs_y,
		&win_x,
		&win_y,
		&temp_mask);
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
		printf("BC_DisplayInfo::Xvext dump: data is uninitialized\n");
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
			printf(" %s", ColorModels::name(adp->cmodels[j]));
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

void BC_DisplayInfo::dump_event(XEvent *ev)
{
	const char *mode, *det, *evtn;

	switch(ev->type)
	{
	case KeyPress:
		evtn = "KeyPress";
		goto keyevt;

	case KeyRelease:
		evtn = "KeyRelease";
keyevt:
	{
		XKeyEvent *kevp = (XKeyEvent*)ev;

		print_event_common(ev, evtn);
		printf(" root %#lx sub %#lx\n", kevp->root, kevp->subwindow);
		printf(" time: %ld (%d,%d) ((%d,%d)) state %u key %#x same %c\n",
			kevp->time, kevp->x, kevp->y, kevp->x_root, kevp->y_root,
			kevp->state, kevp->keycode, kevp->same_screen? 'T':'f');
		break;
	}

	case ButtonPress:
		evtn = "ButtonPress";
		goto butevt;

	case ButtonRelease:
		evtn = "ButtonRelease";
butevt:
	{
		XButtonEvent *bevp = (XButtonEvent*)ev;

		print_event_common(ev, evtn);
		printf(" root %#lx sub %#lx\n", bevp->root, bevp->subwindow);
		printf(" time: %ld (%d,%d) ((%d,%d)) state %u but %d same %c\n",
			bevp->time, bevp->x, bevp->y, bevp->x_root, bevp->y_root,
			bevp->state, bevp->button, bevp->same_screen? 'T':'f');
		break;
	}

	case MotionNotify:
	{
		XMotionEvent *mevp = (XMotionEvent*)ev;

		print_event_common(ev, "MotionNotify");
		printf(" root %#lx sub %#lx\n", mevp->root, mevp->subwindow);
		printf(" time: %ld (%d,%d) ((%d,%d)) state %u hint %d same %c\n",
			mevp->time, mevp->x, mevp->y, mevp->x_root, mevp->y_root,
			mevp->state, mevp->is_hint, mevp->same_screen? 'T':'f');
		break;
	}

	case EnterNotify:
		evtn = "EnterNotify";
		goto croevt;

	case LeaveNotify:
		evtn = "LeaveNotify";
croevt:
	{
		XCrossingEvent *cevp = (XCrossingEvent*)ev;

		print_event_common(ev, evtn);
		printf(" root %#lx sub %#lx\n", cevp->root, cevp->subwindow);
		mode = event_mode_str(cevp->mode);
		det = event_detail_str(cevp->detail);
		printf(" time: %ld (%d,%d) ((%d,%d))\n",
			cevp->time, cevp->x, cevp->y, cevp->x_root, cevp->y_root);
		printf("  m: %s d: %s same %c focus %c state %d\n",
			mode, det, cevp->same_screen? 'T':'f',
			cevp->focus? 'T':'f', cevp->state);
		break;
	}

	case FocusIn:
		evtn = "FocusIn";
		goto focevt;

	case FocusOut:
		evtn = "FocusOut";
focevt:
	{
		XFocusChangeEvent *fevp = (XFocusChangeEvent*)ev;

		print_event_common(ev, evtn);
		mode = event_mode_str(fevp->mode);
		det = event_detail_str(fevp->detail);
		printf(" m: %s s: %s\n", mode, det);
		break;
	}

	case KeymapNotify:
	{
		XKeymapEvent *kmevp = (XKeymapEvent*)ev;

		print_event_common(ev, "KeymapNotify");
		for(int i = 0; i < 32; i++)
			printf(" %02x", kmevp->key_vector[i] & 0xff);
		putchar('\n');
		break;
	}

	case Expose:
	{
		XExposeEvent *eevp = (XExposeEvent*)ev;

		print_event_common(ev, "Expose");
		printf(" (%d,%d) [%d,%d] count %d\n", eevp->x, eevp->y,
			eevp->width, eevp->height, eevp->count);
		break;
	}

	case GraphicsExpose:
	{
		XGraphicsExposeEvent *geevp = (XGraphicsExposeEvent*)ev;

		print_event_common(ev, "GraphicsExpose");
		printf(" (%d,%d) [%d,%d] count %d\n", geevp->x, geevp->y,
			geevp->width, geevp->height, geevp->count);
		printf(" major %d minor %d\n", geevp->major_code, geevp->minor_code);
		break;
	}

	case NoExpose:
	{
		XNoExposeEvent *nevp = (XNoExposeEvent*)ev;

		print_event_common(ev, "NoExpose");
		printf(" major %d minor %d\n", nevp->major_code, nevp->minor_code);
		break;
	}

	case VisibilityNotify:
	{
		XVisibilityEvent *vevp = (XVisibilityEvent*)ev;

		print_event_common(ev, "VisibilityNotify");
		printf(" state %d\n", vevp->state);
		break;
	}

	case CreateNotify:
	{
		XCreateWindowEvent *crevp = (XCreateWindowEvent*)ev;

		print_event_common(ev, "CreateNotify");
		printf(" created %#lx (%d,%d) [%d,%d] border %d redct %c\n",
			crevp->window, crevp->x, crevp->y,
			crevp->width, crevp->height, crevp->border_width,
			crevp->override_redirect ? 'T':'f');
		break;
	}

	case DestroyNotify:
	{
		XDestroyWindowEvent *devp = (XDestroyWindowEvent*)ev;

		print_event_common(ev, "DestroyNotify");
		printf(" destroyed %#lx\n", devp->window);
		break;
	}

	case UnmapNotify:
	{
		XUnmapEvent *uevp = (XUnmapEvent*)ev;

		print_event_common(ev, "UnmapNotify");
		printf(" unmapped %#lx from_configure %c\n", uevp->window,
		uevp->from_configure? 'T':'f');
		break;
	}

	case MapNotify:
	{
		XMapEvent *mpevp = (XMapEvent*)ev;

		print_event_common(ev, "MapNotify");
		printf(" mapped %#lx override_redirect %c\n", mpevp->window,
			mpevp->override_redirect? 'T':'f');
		break;
	}

	case MapRequest:
	{
		XMapRequestEvent *mrevp = (XMapRequestEvent*)ev;

		print_event_common(ev, "MapRequest");
		printf(" window %#lx\n", mrevp->window);
		break;
	}

	case ReparentNotify:
	{
		XReparentEvent *rpevp = (XReparentEvent*)ev;

		print_event_common(ev, "ReparentNotify");
		printf(" window %#lx (%d,%d) override_redirect %c\n", rpevp->window,
			rpevp->x, rpevp->y, rpevp->override_redirect? 'T':'f');
		break;
	}

	case ConfigureNotify:
	{
		XConfigureEvent *cfnevp = (XConfigureEvent*)ev;

		print_event_common(ev, "ConfigureNotify");
		printf(" window %#lx (%d,%d) [%d,%d] border_width: %d\n", cfnevp->window,
			cfnevp->x, cfnevp->y, cfnevp->width, cfnevp->height,
			cfnevp->border_width);
		printf(" above %#lx override_redirect %c\n", cfnevp->above,
			cfnevp->override_redirect? 'T':'f');
		break;
	}

	case ConfigureRequest:
	{
		XConfigureRequestEvent *cfrevp = (XConfigureRequestEvent*)ev;

		print_event_common(ev, "ConfigureRequest");
		printf(" window %#lx (%d,%d) [%d,%d] bw: %d\n", cfrevp->window,
			cfrevp->x, cfrevp->y, cfrevp->width, cfrevp->height,
			cfrevp->border_width);
		printf(" above %#lx detail %d vmask %ld\n", cfrevp->above,
			cfrevp->detail, cfrevp->value_mask);
		break;
	}

	case GravityNotify:
	{
		XGravityEvent *gvevp = (XGravityEvent*)ev;

		print_event_common(ev, "GravityNotify");
		printf(" window %#lx (%d,%d)\n", gvevp->window,
			gvevp->x, gvevp->y);
		break;
	}

	case ResizeRequest:
	{
		XResizeRequestEvent *rzevp = (XResizeRequestEvent*)ev;

		print_event_common(ev, "ResizeRequest");
		printf(" [%d,%d]\n", rzevp->width, rzevp->height);
		break;
	}

	case CirculateNotify:
	{
		XCirculateEvent *cievp = (XCirculateEvent*)ev;

		print_event_common(ev, "CirculateNotify");
		printf(" window %#lx place %d\n", cievp->window,
			cievp->place);
		break;
	}

	case CirculateRequest:
	{
		XCirculateRequestEvent *cirevp = (XCirculateRequestEvent*)ev;

		print_event_common(ev, "CirculateRequest");
		printf(" window %#lx place %d\n", cirevp->window,
			cirevp->place);
		break;
	}

	case PropertyNotify:
	{
		XPropertyEvent *prcevp = (XPropertyEvent*)ev;

		print_event_common(ev, "PropertyNotify");
		printf(" atom %s, time %ld, state %d\n",
			atom_name_str(prcevp->display, prcevp->atom),
			prcevp->time, prcevp->state);
		break;
	}

	case SelectionClear:
	{
		XSelectionClearEvent *sclevp = (XSelectionClearEvent*)ev;

		print_event_common(ev, "SelectionClear");
		printf(" selection %s, time %ld\n",
			atom_name_str(sclevp->display, sclevp->selection),
			sclevp->time);
		break;
	}

	case SelectionRequest:
	{
		XSelectionRequestEvent *screvp = (XSelectionRequestEvent*)ev;

		print_event_common(ev, "SelectionRequest");
		printf(" reqwin %#lx selection %s, target %s\n",
			screvp->requestor,
			atom_name_str(screvp->display, screvp->selection),
			atom_name_str(screvp->display, screvp->target));
		printf(" property %s time %ld\n",
			atom_name_str(screvp->display, screvp->property),
			screvp->time);
		break;
	}

	case SelectionNotify:
	{
		XSelectionEvent *scevp = (XSelectionEvent*)ev;

		print_event_common(ev, "SelectionNotify");
		printf(" selection %s, target %s\n",
			atom_name_str(scevp->display, scevp->selection),
			atom_name_str(scevp->display, scevp->target));
		printf(" property %s time %ld\n",
			atom_name_str(scevp->display, scevp->property),
			scevp->time);
		break;
	}

	case ColormapNotify:
	{
		XColormapEvent *cmevp = (XColormapEvent*)ev;

		print_event_common(ev, "ColormapNotify");
		printf(" colormap %#lx, new %c state %d\n",
			cmevp->colormap, cmevp->c_new ? 'T':'f', cmevp->state);
		break;
	}

	case ClientMessage:
	{
		XClientMessageEvent *clmevp = (XClientMessageEvent*)ev;

		print_event_common(ev, "ClientMessage");
		printf(" msg_type %s format %d\n",
		atom_name_str(clmevp->display, clmevp->message_type),
			clmevp->format);
		switch(clmevp->format)
		{
		case 32:
			for(int i = 0; i < 5; i++)
				printf(" %ld", clmevp->data.l[i]);
			break;
		case 16:
			for(int i = 0; i < 10; i++)
				printf(" %hd", clmevp->data.s[i] & 0xffff);
			break;
		case 8:
		default:
			for(int i = 0; i < 20; i++)
				printf(" %02x", clmevp->data.b[i] & 0xff);
			break;
		}
		putchar('\n');
		break;
	}

	case MappingNotify:
	{
		XMappingEvent *mpevp = (XMappingEvent*)ev;

		print_event_common(ev, "MappingNotify");
		printf("request: %s first_keycode %d, count %d\n",
			event_request_str(mpevp->request),
				mpevp->first_keycode, mpevp->count);
		break;
	}

	case GenericEvent:
	{
		XGenericEvent *xgen = (XGenericEvent*)ev;

		printf("Event '%s' dump: ", "GenericEvent");
		printf(" %ld sent %c disp %p extension %d event %d\n", xgen->serial,
			xgen->send_event ? 'T':'f', xgen->display,
			xgen->extension, xgen->evtype);
		break;
	}

	default:
		printf("Unknown event %d\n", ev->type);
		break;
	}
}

const char *BC_DisplayInfo::event_mode_str(int mode)
{
	switch(mode)
	{
	case NotifyNormal:
		return "NotifyNormal";

	case NotifyWhileGrabbed:
		return "NotifyWhileGrabbed";

	case NotifyGrab:
		return "NotifyGrab";

	case NotifyUngrab:
		return "NotifyUngrab";
	}
	return "Unk";
}

const char *BC_DisplayInfo::event_detail_str(int detail)
{
	switch(detail)
	{
	case NotifyAncestor:
		return "NotifyAncestor";

	case NotifyVirtual:
		return "NotifyVirtual";

	case NotifyInferior:
		return "NotifyInferior";

	case NotifyNonlinear:
		return "NotifyNonlinear";

	case NotifyNonlinearVirtual:
		return "NotifyNonlinearVirtual";

	case NotifyPointer:
		return "NotifyPointer";

	case NotifyPointerRoot:
		return "NotifyPointerRoot";

	case NotifyDetailNone:
		return "NotifyDetailNone";
	}
	return "Unk";
}

const char *BC_DisplayInfo::event_request_str(int req)
{
	switch(req)
	{
	case MappingModifier:
		return "MappingModifier";

	case MappingKeyboard:
		return "MappingKeyboard";

	case MappingPointer:
		return "MappingPointer";
	}
	return "Unk";
}

void BC_DisplayInfo::print_event_common(XEvent *ev, const char *name)
{
	XAnyEvent *aevp = (XAnyEvent*)ev;

	printf("Event '%s' dump: ", name);
	printf(" %ld sent %c disp %p win %#lx\n", aevp->serial,
		aevp->send_event ? 'T':'f', aevp->display, aevp->window);
}

const char *BC_DisplayInfo::atom_name_str(Display *dpy, Atom atom)
{
	if(atom == None)
		return "None";
	return XGetAtomName(dpy, atom);
}
