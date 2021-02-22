// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcbitmap.h"
#include "bcclipboard.h"
#include "bcdisplayinfo.h"
#include "bcmenubar.h"
#include "bcpixmap.h"
#include "bcpopup.h"
#include "bcpopupmenu.h"
#include "bcrepeater.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsubwindow.h"
#include "bctimer.h"
#include "bcwindowbase.h"
#include "colormodels.h"
#include "colors.h"
#include "condition.h"
#include "cursors.h"
#include "bchash.h"
#include "fonts.h"
#include "glthread.h"
#include "keys.h"
#include "language.h"
#include "mutex.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <wchar.h>

#include <X11/extensions/Xvlib.h>
#include <X11/extensions/shape.h>


BC_ResizeCall::BC_ResizeCall(int w, int h)
{
	this->w = w;
	this->h = h;
}


BC_Resources BC_WindowBase::resources;

Window XGroupLeader = 0;

BC_WindowBase::BC_WindowBase()
{
	BC_WindowBase::initialize();
}

BC_WindowBase::~BC_WindowBase()
{
#ifdef HAVE_LIBXXF86VM
	if(window_type == VIDMODE_SCALED_WINDOW && vm_switched)
	{
		restore_vm();
	}
#endif
	if(window_type == MAIN_WINDOW)
	{
// Allow processing the pending events
		lock_events("BC_WindowBase::~BC_WindowBase");
		unlock_events();
	}
	resources.create_window_lock->lock("BC_WindowBase::~BC_WindowBase");
	is_deleting = 1;

	hide_tooltip();
	if(window_type != MAIN_WINDOW)
	{
		if(top_level->active_menubar == this) top_level->active_menubar = 0;
		if(top_level->active_popup_menu == this) top_level->active_popup_menu = 0;
		if(top_level->active_subwindow == this) top_level->active_subwindow = 0;
// Remove pointer from parent window to this
		if(parent_window->subwindows)
			parent_window->subwindows->remove(this);
	}


// Delete the subwindows
	if(subwindows)
	{
		while(subwindows->total)
		{
// Subwindow removes its own pointer
			delete subwindows->values[0];
		}
		delete subwindows;
		subwindows = 0;
	}

	delete pixmap;

	XDestroyWindow(top_level->display, win);

	if(bg_pixmap && !shared_bg_pixmap) delete bg_pixmap;
	if(icon_pixmap) delete icon_pixmap;
	if(icon_window) delete icon_window;
	if(temp_bitmap) delete temp_bitmap;
	resources.create_window_lock->unlock();

	if(have_gl_context)
		resources.get_glthread()->delete_window(top_level->display,
			top_level->screen);

	if(window_type == MAIN_WINDOW) 
	{
		XFreeGC(display, gc);

		if(input_context)
			XDestroyIC(input_context);
		if(input_method)
			XCloseIM(input_method);
		flush();
// Can't close display if another thread is waiting for events.
// Synchronous thread must delete display if gl_context exists.
		XCloseDisplay(display);
		clipboard->stop_clipboard();
		delete clipboard;
	}
	else
	{
		flush();
	}

// Must be last reference to display.
// This only works if it's a MAIN_WINDOW since the display deletion for
// a subwindow is not determined by the subwindow.

	if(wide_text != wide_buffer)
		delete [] wide_text;

	delete [] tooltip_wtext;

	resize_history.remove_all_objects();

	UNSET_ALL_LOCKS(this)
}

void BC_WindowBase::initialize()
{
	test_keypress = 0;
	is_deleting = 0;
	window_lock = 0;
	event_lock = 0;
	x = 0; 
	y = 0; 
	w = 0; 
	h = 0;
	bg_color = -1;
	top_level = 0;
	parent_window = 0;
	subwindows = 0;
	xvideo_port_id = -1;
	video_on = 0;
	motion_events = 0;
	resize_events = 0;
	translation_events = 0;
	ctrl_mask = shift_mask = alt_mask = 0;
	cursor_x = cursor_y = button_number = 0;
	button_down = 0;
	button_pressed = 0;
	button_time1 = button_time2 = 0;
	double_click = 0;
	last_motion_win = 0;
	key_pressed = 0;
	active_menubar = 0;
	active_popup_menu = 0;
	active_subwindow = 0;
	pixmap = 0;
	bg_pixmap = 0;
	tooltip_wtext = 0;
	persistant_tooltip = 0;
	tooltip_popup = 0;
	tooltip_done = 0;
	current_font = MEDIUMFONT;
	current_color = BLACK;
	current_cursor = ARROW_CURSOR;
	hourglass_total = 0;
	is_dragging = 0;
	shared_bg_pixmap = 0;
	icon_pixmap = 0;
	icon_window = 0;
	icon_vframe = 0;
	window_type = MAIN_WINDOW;
	translation_count = 0;
	x_correction = y_correction = 0;
	temp_bitmap = 0;
	tooltip_on = 0;
	temp_cursor = 0;
	toggle_value = 0;
	toggle_drag = 0;
	has_focus = 0;
	is_hourglass = 0;
	is_transparent = 0;
#ifdef HAVE_LIBXXF86VM
	vm_switched = 0;
#endif
	largefont_xft = 0;
	mediumfont_xft = 0;
	smallfont_xft = 0;
	bold_largefont_xft = 0;
	bold_mediumfont_xft = 0;
	bold_smallfont_xft = 0;

	input_method = 0;
	input_context = 0;

	cursor_timer = new Timer;
	have_gl_context = 0;
	wide_text = wide_buffer;
	*wide_text = 0;
	completion_event_type = 0;
	reset_completion();
}

#define DEFAULT_EVENT_MASKS EnterWindowMask | \
			LeaveWindowMask | \
			ButtonPressMask | \
			ButtonReleaseMask | \
			PointerMotionMask | \
			FocusChangeMask


void BC_WindowBase::create_window(BC_WindowBase *parent_window,
				const char *title, 
				int x,
				int y,
				int w, 
				int h, 
				int minw, 
				int minh, 
				int allow_resize,
				int private_color, 
				int hide,
				int bg_color,
				const char *display_name,
				int window_type,
				BC_Pixmap *bg_pixmap,
				int group_it,
				int options)
{
	XSetWindowAttributes attr;
	unsigned long mask;
	XSizeHints size_hints;
	int root_w;
	int root_h;
#ifdef HAVE_LIBXXF86VM
	int vm;
#endif

	id = resources.get_id();

	resources.create_window_lock->lock("BC_WindowBase::create_window");

	if(parent_window) top_level = parent_window->top_level;

#ifdef HAVE_LIBXXF86VM
	if(window_type == VIDMODE_SCALED_WINDOW)
		closest_vm(&vm,&w,&h);
#endif

	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	this->bg_color = bg_color;
	this->window_type = window_type;
	this->hidden = hide;
	this->private_color = private_color;
	this->parent_window = parent_window;
	this->bg_pixmap = bg_pixmap;
	this->allow_resize = allow_resize;
	strcpy(this->title, title);
	if(bg_pixmap) shared_bg_pixmap = 1;

	if(parent_window) top_level = parent_window->top_level;

	subwindows = new BC_SubWindowList;

// Mandatory setup
	if(window_type == MAIN_WINDOW)
	{
		top_level = this;
		parent_window = this;
		event_lock = new Mutex("BC_WindowBase::event_lock");

// get the display connection
		display = init_display(display_name);

// Fudge window placement
		root_w = get_root_w(1, 0);
		root_h = get_root_h(0);
		if(this->x + this->w > root_w) this->x = root_w - this->w;
		if(this->y + this->h > root_h) this->y = root_h - this->h;
		if(this->x < 0) this->x = 0;
		if(this->y < 0) this->y = 0;
		screen = DefaultScreen(display);
		rootwin = RootWindow(display, screen);

		vis = DefaultVisual(display, screen);
		default_depth = DefaultDepth(display, screen);

		client_byte_order = (*(const u_int32_t*)"a   ") & 0x00000001;
		server_byte_order = (XImageByteOrder(display) == MSBFirst) ? 0 : 1;

// This must be done before fonts to know if antialiasing is available.
		init_colors();
// get the resources
		if(resources.use_shm < 0) resources.initialize_display(this);
		x_correction = resources.get_left_border();
		y_correction = resources.get_top_border();
		if(this->bg_color == -1)
			this->bg_color = resources.get_bg_color();

		init_fonts();
		init_gc();
		init_cursors();

// Create the window
		mask = CWEventMask | 
				CWBackPixel | 
				CWColormap | 
				CWCursor;

		attr.event_mask = DEFAULT_EVENT_MASKS |
			StructureNotifyMask | 
			KeyPressMask;

		attr.background_pixel = get_color(this->bg_color);
		attr.colormap = cmap;
		attr.cursor = get_cursor_struct(ARROW_CURSOR);

		if(options & WINDOW_SPLASH)
		{
			mask |= CWOverrideRedirect;
			attr.override_redirect = True;
		}

		win = XCreateWindow(display, 
			rootwin, 
			this->x, 
			this->y, 
			this->w, 
			this->h, 
			0, 
			top_level->default_depth, 
			InputOutput, 
			vis, 
			mask, 
			&attr);

		XGetNormalHints(display, win, &size_hints);

		size_hints.flags = PSize | PMinSize | PMaxSize;
		size_hints.width = this->w;
		size_hints.height = this->h;
		size_hints.min_width = allow_resize ? minw : this->w;
		size_hints.max_width = allow_resize ? 32767 : this->w; 
		size_hints.min_height = allow_resize ? minh : this->h;
		size_hints.max_height = allow_resize ? 32767 : this->h; 
		if(x > -BC_INFINITY && x < BC_INFINITY)
		{
			size_hints.flags |= PPosition;
			size_hints.x = this->x;
			size_hints.y = this->y;
		}

		char *txlist[2];
		txlist[0] = this->title;
		txlist[1] = 0;
		XTextProperty titleprop;
		if(options & WINDOW_UTF8)
			Xutf8TextListToTextProperty(display, txlist,  1,
				XUTF8StringStyle, &titleprop);
		else
			XmbTextListToTextProperty(display, txlist, 1,
				XStdICCTextStyle, &titleprop);
		XSetWMProperties(display, win, &titleprop, &titleprop,
			0, 0, &size_hints, 0, 0);
		XFree(titleprop.value);
		get_atoms();

		clipboard = new BC_Clipboard(display_name);
		clipboard->start_clipboard();

		if (group_it)
		{
			Atom ClientLeaderXAtom;
			if (XGroupLeader == 0)
				XGroupLeader = win;
			XClassHint *class_hints = XAllocClassHint(); 
			class_hints->res_name = (char*)"cinelerra";
			class_hints->res_class = (char*)"Cinelerra";
			XSetClassHint(top_level->display, win, class_hints);
			XFree(class_hints);
			ClientLeaderXAtom = XInternAtom(display, "WM_CLIENT_LEADER", True);
			XChangeProperty(display, 
					win, 
					ClientLeaderXAtom, 
					XA_WINDOW, 
					32, 
					PropModeReplace, 
					(unsigned char *)&XGroupLeader, 
					true);
		}
		init_im();
		completion_event_type = XShmGetEventBase(display) + ShmCompletion;
	}

#ifdef HAVE_LIBXXF86VM
	if(window_type == VIDMODE_SCALED_WINDOW && vm != -1)
	{
		scale_vm (vm);
		vm_switched = 1;
	}
#endif

#ifdef HAVE_LIBXXF86VM
	if(window_type == POPUP_WINDOW || window_type == VIDMODE_SCALED_WINDOW)
#else
	if(window_type == POPUP_WINDOW)
#endif
	{
		mask = CWEventMask | 
			CWBackPixel | 
			CWColormap | 
			CWOverrideRedirect | 
			CWSaveUnder | 
			CWCursor;

		attr.event_mask = DEFAULT_EVENT_MASKS |
			KeyPressMask;

		if(this->bg_color == -1)
			this->bg_color = resources.get_bg_color();
		attr.background_pixel = top_level->get_color(this->bg_color);
		attr.colormap = top_level->cmap;
		if(top_level->is_hourglass)
			attr.cursor = top_level->get_cursor_struct(HOURGLASS_CURSOR);
		else
			attr.cursor = top_level->get_cursor_struct(ARROW_CURSOR);
		attr.override_redirect = True;
		attr.save_under = True;

		win = XCreateWindow(top_level->display, 
			top_level->rootwin, 
			this->x, 
			this->y, 
			this->w, 
			this->h, 
			0, 
			top_level->default_depth, 
			InputOutput, 
			top_level->vis, 
			mask, 
			&attr);
	}

	if(window_type == SUB_WINDOW)
	{
		mask = CWBackPixel | 
			CWEventMask | 
			CWCursor;
		attr.event_mask = DEFAULT_EVENT_MASKS;
		attr.background_pixel = top_level->get_color(this->bg_color);
		if(top_level->is_hourglass)
			attr.cursor = top_level->get_cursor_struct(HOURGLASS_CURSOR);
		else
			attr.cursor = top_level->get_cursor_struct(ARROW_CURSOR);
		win = XCreateWindow(top_level->display, 
			parent_window->win, 
			this->x, 
			this->y, 
			this->w, 
			this->h, 
			0, 
			top_level->default_depth, 
			InputOutput, 
			top_level->vis, 
			mask, 
			&attr);
		init_window_shape();
		XMapWindow(top_level->display, win);
	}

// Create pixmap for all windows
	pixmap = new BC_Pixmap(this, this->w, this->h);

	if(window_type == MAIN_WINDOW && !hidden)
		show_window();

	draw_background(0, 0, this->w, this->h);
	flash();

// Set up options for popup window
#ifdef HAVE_LIBXXF86VM
	if(window_type == POPUP_WINDOW || window_type == VIDMODE_SCALED_WINDOW)
#else
	if(window_type == POPUP_WINDOW)
#endif
	{
		init_window_shape();
		if(!hidden) show_window();
	}
	resources.create_window_lock->unlock();
}

Display* BC_WindowBase::init_display(const char *display_name)
{
	Display* display;

	if(display_name && display_name[0] == 0) display_name = NULL;
	if((display = XOpenDisplay(display_name)) == NULL)
	{
		printf("BC_WindowBase::init_display: cannot connect to X server %s\n", 
			display_name);
		if(getenv("DISPLAY") == NULL)
		{
			printf("'DISPLAY' environment variable not set.\n");
			exit(1);
		}
		else
// Try again with default display.
		{
			if((display = XOpenDisplay(0)) == NULL)
			{
				printf("BC_WindowBase::init_display: cannot connect to default X server.\n");
				exit(1);
			}
		}
	}
	return display;
}

int BC_WindowBase::run_window()
{
	done = 0;
	return_value = 0;

// Events may have been sent before run_window so can't initialize them here.

// Start tooltips
	if(window_type == MAIN_WINDOW)
	{
		set_repeat(resources.tooltip_delay);
	}

// Start common events
	while(!done)
	{
		dispatch_event();
	}

	unset_all_repeaters();
	hide_tooltip();
	done = 0;

	return return_value;
}

void BC_WindowBase::get_key_masks(XEvent *event)
{
// ctrl key down
	ctrl_mask = (event->xkey.state & ControlMask) ? 1 : 0;
// shift key down
	shift_mask = (event->xkey.state & ShiftMask) ? 1 : 0;
	alt_mask = (event->xkey.state & Mod1Mask) ? 1 : 0;
}

void BC_WindowBase::dispatch_event()
{
	XEvent event;
	Window tempwin;
	KeySym keysym;
	int result;
	XClientMessageEvent *ptr;
	int temp;
	int cancel_resize, cancel_translation;
// If an event is waiting get it, otherwise
// wait for next event only if there are no compressed events.
	lock_window("BC_WindowBase::dispatch_event1");
	if(XPending(display) ||
			(!motion_events && !resize_events && !translation_events))
	{
		while(!XPending(display))
		{
			unlock_window();
			struct pollfd fds;
			fds.fd = ConnectionNumber(display);
			fds.events = POLLIN | POLLPRI;
			fds.revents = 0;
			int rv = poll(&fds, 1, -1);
			lock_window("BC_WindowBase::dispatch_event2");
		}
		XNextEvent(display, &event);
		unlock_window();
	}
	else
// Handle compressed events
	{
		unlock_window();
		lock_events("BC_WindowBase::dispatch_event - compressed");
		if(resize_events)
			dispatch_resize_event(last_resize_w, last_resize_h);
		else
		if(motion_events)
			dispatch_motion_event();
		else
		if(translation_events)
			dispatch_translation_event();
		unlock_events();

		return;
	}

	switch(event.type)
	{
	case ClientMessage:
// Clear the resize buffer
		if(resize_events)
			dispatch_resize_event(last_resize_w, last_resize_h);
// Clear the motion buffer since this can clear the window
		if(motion_events)
			dispatch_motion_event();

		ptr = (XClientMessageEvent*)&event;


		if(ptr->message_type == ProtoXAtom && ptr->data.l[0] == DelWinXAtom)
			close_event();
		else
		if(ptr->message_type == RepeaterXAtom)
			dispatch_repeat_event(ptr->data.l[0]);
		else
		if(ptr->message_type == SetDoneXAtom)
		{
			return_value = ptr->data.l[0];
			done = 1;
		} 
		else
			recieve_custom_xatoms(ptr);
		break;

	case FocusIn:
		has_focus = 1;
		dispatch_focus_in();
		break;

	case FocusOut:
		has_focus = 0;
		dispatch_focus_out();
		break;

// Maximized
	case MapNotify:
		break;

// Minimized
	case UnmapNotify:
		break;

	case ButtonPress:
		get_key_masks(&event);
		cursor_x = event.xbutton.x;
		cursor_y = event.xbutton.y;
		button_number = event.xbutton.button;
		event_win = event.xany.window;
		if (button_number != 4 && button_number != 5)
			button_down = 1;
		button_pressed = event.xbutton.button;
		button_time1 = button_time2;
		button_time2 = event.xbutton.time;
		drag_x = cursor_x;
		drag_y = cursor_y;
		drag_win = event_win;
		drag_x1 = cursor_x - resources.drag_radius;
		drag_x2 = cursor_x + resources.drag_radius;
		drag_y1 = cursor_y - resources.drag_radius;
		drag_y2 = cursor_y + resources.drag_radius;

		if(button_time2 - button_time1 < resources.double_click)
		{
// Ignore triple clicks
			double_click = 1; 
			button_time2 = button_time1 = 0; 
		}
		else
			double_click = 0;

		dispatch_button_press();
		break;

	case ButtonRelease:
		get_key_masks(&event);
		button_number = event.xbutton.button;
		event_win = event.xany.window;

		if (button_number != 4 && button_number != 5) 
			button_down = 0;

		dispatch_button_release();
		break;

	case Expose:
		event_win = event.xany.window;
		dispatch_expose_event();
		break;

	case MotionNotify:
		lock_events("BC_WindowBase::dispatch_event MotionNotify");
		get_key_masks(&event);
// Dispatch previous motion event if this is a subsequent motion from a different window
		if(motion_events && last_motion_win != event.xany.window)
			dispatch_motion_event();

// Buffer the current motion
		motion_events = 1;
		last_motion_x = event.xmotion.x;
		last_motion_y = event.xmotion.y;
		last_motion_win = event.xany.window;
		unlock_events();
		break;

	case ConfigureNotify:
		if(win == top_level->win)
		{
			last_translate_x = event.xconfigure.x;
			last_translate_y = event.xconfigure.y;
		}
		else
		{
			lock_window("BC_WindowBase::dispatch_event ConfigureNotify");
			XTranslateCoordinates(top_level->display,
				top_level->win,
				top_level->rootwin,
				0,
				0,
				&last_translate_x,
				&last_translate_y,
				&tempwin);
			unlock_window();
		}
		last_resize_w = event.xconfigure.width;
		last_resize_h = event.xconfigure.height;
		cancel_resize = 0;
		cancel_translation = 0;

// Resize history prevents responses to recursive resize requests
		for(int i = 0; i < resize_history.total && !cancel_resize; i++)
		{
			if(resize_history.values[i]->w == last_resize_w &&
				resize_history.values[i]->h == last_resize_h)
			{
				delete resize_history.values[i];
				resize_history.remove_number(i);
				cancel_resize = 1;
			}
		}

		if(last_resize_w == w && last_resize_h == h)
			cancel_resize = 1;

		if(!cancel_resize)
			resize_events = 1;

		if((last_translate_x == x && last_translate_y == y))
			cancel_translation = 1;

		if(!cancel_translation)
			translation_events = 1;

		translation_count++;
		break;

	case KeyPress:
		get_key_masks(&event);
		key_pressed = 0;

		if(XFilterEvent(&event, win)) break;

		wkey_string_length = XwcLookupString(input_context,
			(XKeyEvent*)&event, wkey_string, 4, &keysym, 0);

// block out control keys
		if(keysym > 0xffe0 && keysym < 0xffff) break;

		switch(keysym)
		{
// block out extra keys
		case XK_Alt_L:
		case XK_Alt_R:
		case XK_Shift_L:
		case XK_Shift_R:
		case XK_Control_L:
		case XK_Control_R:
			key_pressed = 0;
			break;

// Translate key codes
		case XK_Return:     key_pressed = RETURN;    break;
		case XK_Up:         key_pressed = UP;        break;
		case XK_Down:       key_pressed = DOWN;      break;
		case XK_Left:       key_pressed = LEFT;      break;
		case XK_Right:      key_pressed = RIGHT;     break;
		case XK_Next:       key_pressed = PGDN;      break;
		case XK_Prior:      key_pressed = PGUP;      break;
		case XK_BackSpace:  key_pressed = BACKSPACE; break;
		case XK_Escape:     key_pressed = ESC;       break;
		case XK_Tab:
			if(shift_down())
				key_pressed = LEFTTAB;
			else
				key_pressed = TAB;
			break;
		case XK_ISO_Left_Tab: key_pressed = LEFTTAB; break;
		case XK_underscore: key_pressed = '_';       break;
		case XK_asciitilde: key_pressed = '~';       break;
		case XK_Delete:     key_pressed = DELETE;    break;
		case XK_Home:       key_pressed = HOME;      break;
		case XK_End:        key_pressed = END;       break;

// number pad
		case XK_KP_Enter:       key_pressed = KPENTER;   break;
		case XK_KP_Add:         key_pressed = KPPLUS;    break;
		case XK_KP_1:
		case XK_KP_End:         key_pressed = KP1;       break;
		case XK_KP_2:
		case XK_KP_Down:        key_pressed = KP2;       break;
		case XK_KP_3:
		case XK_KP_Page_Down:   key_pressed = KP3;       break;
		case XK_KP_4:
		case XK_KP_Left:        key_pressed = KP4;       break;
		case XK_KP_5:
		case XK_KP_Begin:       key_pressed = KP5;       break;
		case XK_KP_6:
		case XK_KP_Right:       key_pressed = KP6;       break;
		case XK_KP_0:
		case XK_KP_Insert:      key_pressed = KPINS;     break;
		case XK_KP_Decimal:
		case XK_KP_Delete:      key_pressed = KPDEL;     break;
		default:
			key_pressed = keysym & 0xff;
			break;
		}
		result = dispatch_keypress_event();
// Handle some default keypresses
		if(!result)
		{
			if(key_pressed == 'w' || key_pressed == 'W')
				close_event();
		}
		break;

	case LeaveNotify:
		event_win = event.xany.window;
		dispatch_cursor_leave();
		break;

	case EnterNotify:
		event_win = event.xany.window;
		cursor_x = event.xcrossing.x;
		cursor_y = event.xcrossing.y;
		dispatch_cursor_enter();
		break;
	default:
		if(completion_event_type && event.type == completion_event_type)
			dispatch_completion(&event);
		break;
	}
}

void BC_WindowBase::dispatch_expose_event()
{
	for(int i = 0; subwindows && i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_expose_event();
	}

// Propagate to user
	expose_event();
}

void BC_WindowBase::dispatch_resize_event(int w, int h)
{
// Can't store new w and h until the event is handles 
// because bcfilebox depends on the old w and h to
// reposition widgets.
	if(window_type == MAIN_WINDOW)
	{
		resize_events = 0;
		delete pixmap;
		pixmap = new BC_Pixmap(this, w, h);

		clear_box(0, 0, w, h);
	}

// Propagate to subwindows
	for(int i = 0; subwindows && i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_resize_event(w, h);
	}

// Propagate to user
	resize_event(w, h);

	if(window_type == MAIN_WINDOW)
	{
		this->w = w;
		this->h = h;
	}
}

void BC_WindowBase::dispatch_translation_event()
{
	translation_events = 0;
	if(window_type == MAIN_WINDOW)
	{
		prev_x = x;
		prev_y = y;
		x = last_translate_x;
		y = last_translate_y;
// Correct for window manager offsets
		x -= x_correction;
		y -= y_correction;
	}

	for(int i = 0; subwindows && i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_translation_event();
	}

	translation_event();
}

int BC_WindowBase::dispatch_motion_event()
{
	int result = 0;
	unhide_cursor();

	if(top_level == this)
	{
		event_win = last_motion_win;
		motion_events = 0;
// Test for grab
		if(get_button_down() && !active_menubar && !active_popup_menu)
		{
			if(!result)
			{
				cursor_x = last_motion_x;
				cursor_y = last_motion_y;
				result = dispatch_drag_motion();
			}

			if(!result && 
				(last_motion_x < drag_x1 || last_motion_x >= drag_x2 || 
				last_motion_y < drag_y1 || last_motion_y >= drag_y2))
			{
				cursor_x = drag_x;
				cursor_y = drag_y;

				result = dispatch_drag_start();
			}
		}
		cursor_x = last_motion_x;
		cursor_y = last_motion_y;

		if(active_menubar && !result) result = active_menubar->dispatch_motion_event();
		if(active_popup_menu && !result) result = active_popup_menu->dispatch_motion_event();
		if(active_subwindow && !result) result = active_subwindow->dispatch_motion_event();
	}

	for(int i = 0; subwindows && i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_motion_event();
	}

	if(!result) result = cursor_motion_event();    // give to user
	return result;
}

int BC_WindowBase::dispatch_keypress_event()
{
	int result = 0;
	if(top_level == this)
	{
		if(active_subwindow) 
			result = active_subwindow->dispatch_keypress_event();
	}

	for(int i = 0; subwindows && i < subwindows->total && !result; i++)
		result = subwindows->values[i]->dispatch_keypress_event();

	if(!result) result = keypress_event();

	return result;
}

void BC_WindowBase::dispatch_focus_in()
{
	for(int i = 0; subwindows && i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_focus_in();
	}

	focus_in_event();
}

void BC_WindowBase::dispatch_focus_out()
{
	for(int i = 0; subwindows && i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_focus_out();
	}

	focus_out_event();
}

int BC_WindowBase::register_completion(BC_Bitmap *bitmap)
{
	if(top_level != this)
		return top_level->register_completion(bitmap);

	for(int i = 0; i < COMPLETITION_BITMAPS; i++)
	{
		if(completition_bitmap[i] == bitmap)
			return 0;
	}
	for(int i = 0; i < COMPLETITION_BITMAPS; i++)
	{
		if(!completition_bitmap[i])
		{
			completition_bitmap[i] = bitmap;
			return 0;
		}
	}
	// No free slot
	return 1;
}

void BC_WindowBase::unregister_completion(BC_Bitmap *bitmap)
{
	if(top_level != this)
	{
		top_level->unregister_completion(bitmap);
		return;
	}

	for(int i = 0; i < COMPLETITION_BITMAPS; i++)
	{
		if(completition_bitmap[i] == bitmap)
			completition_bitmap[i] = 0;
	}
}

void BC_WindowBase::reset_completion()
{
	for(int i = 0; i < COMPLETITION_BITMAPS; i++)
		completition_bitmap[i] = 0;
}

int BC_WindowBase::dispatch_completion(XEvent *event)
{
	int result = 0;

	if(top_level != this)
		return top_level->dispatch_completion(event);

	lock_window("BC_WindowBase::completion_event");
	for(int i = 0; i < COMPLETITION_BITMAPS; i++)
	{
		if(completition_bitmap[i])
			result += completition_bitmap[i]->completion_event(event);
	}
	unlock_window();
	return result;
}

int BC_WindowBase::get_has_focus()
{
	return top_level->has_focus;
}

int BC_WindowBase::get_deleting()
{
	if(is_deleting) return 1;
	if(parent_window && parent_window->get_deleting()) return 1;
	return 0;
}

int BC_WindowBase::dispatch_button_press()
{
	int result = 0;
	if(top_level == this)
	{
		if(active_menubar) result = active_menubar->dispatch_button_press();
		if(active_popup_menu && !result) result = active_popup_menu->dispatch_button_press();
		if(active_subwindow && !result) result = active_subwindow->dispatch_button_press();
	}

	for(int i = 0; subwindows && i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_button_press();
	}

	if(!result) result = button_press_event();

	return result;
}

int BC_WindowBase::dispatch_button_release()
{
	int result = 0;
	if(top_level == this)
	{
		if(active_menubar) result = active_menubar->dispatch_button_release();
		if(active_popup_menu && !result) result = active_popup_menu->dispatch_button_release();
		if(active_subwindow && !result) result = active_subwindow->dispatch_button_release();
		if(!result && button_number != 4 && button_number != 5)
			result = dispatch_drag_stop();
	}

	for(int i = 0; subwindows && i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_button_release();
	}

	if(!result)
	{
		result = button_release_event();
	}

	return result;
}


void BC_WindowBase::dispatch_repeat_event(int duration)
{
// all repeat event handlers get called and decide based on activity and duration
// whether to respond
	for(int i = 0; subwindows && i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_repeat_event(duration);
	}

	repeat_event(duration);

// Unlock next repeat signal
	if(window_type == MAIN_WINDOW)
	{
		for(int i = 0; i < repeaters.total; i++)
		{
			if(repeaters.values[i]->delay == duration)
			{
				repeaters.values[i]->repeat_lock->unlock();
			}
		}
	}
}

void BC_WindowBase::unhide_cursor()
{
	if(is_transparent)
	{
		is_transparent = 0;
		if(top_level->is_hourglass)
			set_cursor(HOURGLASS_CURSOR, 1);
		else
			set_cursor(current_cursor, 1);
	}
	cursor_timer->update();
}


void BC_WindowBase::update_video_cursor()
{
	if(video_on && !is_transparent)
	{
		if(cursor_timer->get_difference() > VIDEO_CURSOR_TIMEOUT && !is_transparent)
		{
			is_transparent = 1;
			set_cursor(TRANSPARENT_CURSOR, 1);
			cursor_timer->update();
		}
	}
	else
	{
		cursor_timer->update();
	}
}


void BC_WindowBase::dispatch_cursor_leave()
{
	unhide_cursor();

	for(int i = 0; subwindows && i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_cursor_leave();
	}

	cursor_leave_event();
}

int BC_WindowBase::dispatch_cursor_enter()
{
	int result = 0;

	unhide_cursor();

	if(active_menubar) result = active_menubar->dispatch_cursor_enter();
	if(!result && active_popup_menu) result = active_popup_menu->dispatch_cursor_enter();
	if(!result && active_subwindow) result = active_subwindow->dispatch_cursor_enter();

	for(int i = 0; !result && subwindows && i < subwindows->total; i++)
	{
		result = subwindows->values[i]->dispatch_cursor_enter();
	}

	if(!result) result = cursor_enter_event();
	return result;
}

void BC_WindowBase::close_event()
{
	set_done(1);
}

int BC_WindowBase::dispatch_drag_start()
{
	int result = 0;

	if(active_menubar) result = active_menubar->dispatch_drag_start();
	if(!result && active_popup_menu) result = active_popup_menu->dispatch_drag_start();
	if(!result && active_subwindow) result = active_subwindow->dispatch_drag_start();

	for(int i = 0; subwindows && i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_drag_start();
	}

	if(!result) result = is_dragging = drag_start_event();
	return result;
}

int BC_WindowBase::dispatch_drag_stop()
{
	int result = 0;

	for(int i = 0; subwindows && i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_drag_stop();
	}

	if(is_dragging && !result) 
	{
		drag_stop_event();
		is_dragging = 0;
		result = 1;
	}

	return result;
}

int BC_WindowBase::dispatch_drag_motion()
{
	int result = 0;

	for(int i = 0; subwindows && i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_drag_motion();
	}

	if(is_dragging && !result)
	{
		drag_motion_event();
		result = 1;
	}

	return result;
}

void BC_WindowBase::show_tooltip(int w, int h)
{
	Window tempwin;

	if(!tooltip_on && resources.tooltips_enabled)
	{
		int x, y, rootw, rooth;

		top_level->hide_tooltip();
		tooltip_on = 1;


		lock_window("BC_WindowBase::show_tooltip");
		XTranslateCoordinates(top_level->display, 
				win, 
				top_level->rootwin, 
				get_w(), 
				get_h(), 
				&x, 
				&y, 
				&tempwin);
		unlock_window();

		if(w < 0)
		{
			resources.get_root_size(&rootw, &rooth);
			w = TOOLTIP_MAX;
			if(rootw - x < TOOLTIP_MAX)
			{
				w = rootw - x;
				if(w < TOOLTIP_MIN)
					w = TOOLTIP_MIN;
			}
			else
				w = TOOLTIP_MAX;
			BC_TextBox::wstringbreaker(MEDIUMFONT, tooltip_wtext,
				w, this);

			w = get_text_width(MEDIUMFONT, tooltip_wtext);
		}

		if(h < 0)
			h = get_text_height(MEDIUMFONT, tooltip_wtext);

		w += TOOLTIP_MARGIN * 2;
		h += TOOLTIP_MARGIN * 2;

		tooltip_popup = new BC_Popup(top_level, 
					x,
					y,
					w, 
					h, 
					resources.tooltip_bg_color);

		draw_tooltip();
		tooltip_popup->set_font(MEDIUMFONT);
		tooltip_popup->flash(1);
	}
}

void BC_WindowBase::hide_tooltip()
{
	for(int i = 0; subwindows && i < subwindows->total; i++)
		subwindows->values[i]->hide_tooltip();

	if(tooltip_on)
	{
		tooltip_on = 0;
		delete tooltip_popup;
		tooltip_popup = 0;
	}
}

void BC_WindowBase::set_tooltip(const char *text, int is_utf8)
{
	char lbuf[BCTEXTLEN];
	int len = strlen(text);

	strncpy(lbuf, text, BCTEXTLEN - 1);
	lbuf[BCTEXTLEN - 1] = 0;

	if(len)
	{
		if(!tooltip_wtext)
			tooltip_wtext = new wchar_t[BCTEXTLEN];
	}
	else
	{
		delete [] tooltip_wtext;
		tooltip_wtext = 0;
		tooltip_length = 0;
		if(tooltip_on)
			hide_tooltip();
		return;
	}

	tooltip_length = BC_Resources::encode(is_utf8 ? "UTF8" : BC_Resources::encoding,
		BC_Resources::wide_encoding, lbuf, (char*)tooltip_wtext,
		BCTEXTLEN * sizeof(wchar_t)) / sizeof(wchar_t);

// Update existing tooltip if it is visible
	if(tooltip_on)
	{
		draw_tooltip();
		tooltip_popup->flash();
	}
}

// signal the event handler to repeat
void BC_WindowBase::set_repeat(int duration)
{
	if(duration <= 0)
		return;
	if(window_type != MAIN_WINDOW)
	{ 
		top_level->set_repeat(duration);
		return;
	}

// test repeater database for duplicates
	for(int i = 0; i < repeaters.total; i++)
	{
// Already exists
		if(repeaters.values[i]->delay == duration)
		{
			repeaters.values[i]->start_repeating();
			return;
		}
	}

	BC_Repeater *repeater = new BC_Repeater(this, duration);
	repeater->initialize();
	repeaters.append(repeater);
	repeater->start_repeating();
	return;
}

void BC_WindowBase::unset_repeat(int duration)
{
	if(window_type != MAIN_WINDOW)
	{
		top_level->unset_repeat(duration);
		return;
	}

	for(int i = 0; i < repeaters.total; i++)
	{
		if(repeaters.values[i]->delay == duration)
			repeaters.values[i]->stop_repeating();
	}
}


void BC_WindowBase::unset_all_repeaters()
{
	for(int i = 0; i < repeaters.total; i++)
	{
		repeaters.values[i]->stop_repeating();
	}
	repeaters.remove_all_objects();
}

void BC_WindowBase::arm_repeat(int duration)
{
	XEvent event;

	event.xclient.type = ClientMessage;
	event.xclient.message_type = RepeaterXAtom;
	event.xclient.display = top_level->display;
	event.xclient.window = top_level->win;
	event.xclient.format = 32;
	event.xclient.data.l[0] = duration;

	XSendEvent(top_level->display, 
		top_level->win, 
		0, 
		0, 
		&event);
	flush();
}

void BC_WindowBase::send_custom_xatom(XClientMessageEvent *event)
{
	event->type = ClientMessage;
	event->display = top_level->display;
	event->window = top_level->win;

	XSendEvent(top_level->display, 
		top_level->win, 
		0, 
		0, 
		(XEvent *)event);
	flush();
}


Atom BC_WindowBase::create_xatom(const char *atom_name)
{
	return XInternAtom(display, atom_name, False);
}

void BC_WindowBase::get_atoms()
{
	SetDoneXAtom =  create_xatom("BC_REPEAT_EVENT");
	RepeaterXAtom = create_xatom("BC_CLOSE_EVENT");
	DelWinXAtom =   create_xatom("WM_DELETE_WINDOW");
	if(ProtoXAtom = create_xatom("WM_PROTOCOLS"))
		XChangeProperty(display, win, ProtoXAtom, XA_ATOM, 32, PropModeReplace, (unsigned char *)&DelWinXAtom, True);
}


void BC_WindowBase::init_cursors()
{
	arrow_cursor = XCreateFontCursor(display, XC_top_left_arrow);
	cross_cursor = XCreateFontCursor(display, XC_crosshair);
	ibeam_cursor = XCreateFontCursor(display, XC_xterm);
	vseparate_cursor = XCreateFontCursor(display, XC_sb_v_double_arrow);
	hseparate_cursor = XCreateFontCursor(display, XC_sb_h_double_arrow);
	move_cursor = XCreateFontCursor(display, XC_fleur);
	left_cursor = XCreateFontCursor(display, XC_sb_left_arrow);
	right_cursor = XCreateFontCursor(display, XC_sb_right_arrow);
	upright_arrow_cursor = XCreateFontCursor(display, XC_arrow);
	upleft_resize_cursor = XCreateFontCursor(display, XC_top_left_corner);
	upright_resize_cursor = XCreateFontCursor(display, XC_top_right_corner);
	downleft_resize_cursor = XCreateFontCursor(display, XC_bottom_left_corner);
	downright_resize_cursor = XCreateFontCursor(display, XC_bottom_right_corner);
	hourglass_cursor = XCreateFontCursor(display, XC_watch);

	char cursor_data[] = { 0,0,0,0, 0,0,0,0 };
	Colormap colormap = DefaultColormap(display, screen);
	Pixmap pixmap_bottom = XCreateBitmapFromData(display, 
		rootwin,
		cursor_data, 
		8,
		8);
	XColor black, dummy;
	XAllocNamedColor(display, colormap, "black", &black, &dummy);
	transparent_cursor = XCreatePixmapCursor(display,
		pixmap_bottom,
		pixmap_bottom,
		&black,
		&black,
		0,
		0);
	XFreePixmap(display, pixmap_bottom);
}

int BC_WindowBase::evaluate_color_model(int client_byte_order, int server_byte_order, int depth)
{
	int color_model;

	switch(depth)
	{
	case 8:
		color_model = BC_RGB8;
		break;
	case 16:
		color_model = (server_byte_order == client_byte_order) ? BC_RGB565 : BC_BGR565;
		break;
	case 24:
		color_model = server_byte_order ? BC_BGR888 : BC_RGB888;
		break;
	case 32:
		color_model = server_byte_order ? BC_BGR8888 : BC_ARGB8888;
		break;
	}
	return color_model;
}

void BC_WindowBase::init_colors()
{
	total_colors = 0;
	current_color_value = current_color_pixel = 0;

// Get the real depth
	char *data = 0;
	XImage *ximage;
	ximage = XCreateImage(top_level->display, 
					top_level->vis, 
					top_level->default_depth, 
					ZPixmap, 
					0, 
					data, 
					16, 
					16, 
					8, 
					0);
	bits_per_pixel = ximage->bits_per_pixel;
	XDestroyImage(ximage);

	color_model = evaluate_color_model(client_byte_order, 
		server_byte_order, 
		bits_per_pixel);
// Get the color model
	switch(color_model)
	{
	case BC_RGB8:
		if(private_color)
		{
			cmap = XCreateColormap(display, rootwin, vis, AllocNone);
			create_private_colors();
		}
		else
		{
			cmap = DefaultColormap(display, screen);
			create_shared_colors();
		}

		allocate_color_table();
// No antialiasing
		break;

	default:
		cmap = DefaultColormap(display, screen);
		break;
	}
}

void BC_WindowBase::create_private_colors()
{
	int color;
	total_colors = 256;

	for(int i = 0; i < 255; i++)
	{
		color = (i & 0xc0) << 16;
		color += (i & 0x38) << 10;
		color += (i & 0x7) << 5;
		color_table[i][0] = color;
	}
	create_shared_colors();        // overwrite the necessary colors on the table
}


void BC_WindowBase::create_color(int color)
{
	if(total_colors == 256)
	{
// replace the closest match with an exact match
		color_table[get_color_rgb8(color)][0] = color;
	}
	else
	{
// add the color to the table
		color_table[total_colors][0] = color;
		total_colors++;
	}
}

void BC_WindowBase::create_shared_colors()
{
	create_color(BLACK);
	create_color(WHITE);

	create_color(LTGREY);
	create_color(MEGREY);
	create_color(MDGREY);
	create_color(DKGREY);

	create_color(LTCYAN);
	create_color(MECYAN);
	create_color(MDCYAN);
	create_color(DKCYAN);

	create_color(LTGREEN);
	create_color(GREEN);
	create_color(DKGREEN);

	create_color(LTPINK);
	create_color(PINK);
	create_color(RED);

	create_color(LTBLUE);
	create_color(BLUE);
	create_color(DKBLUE);

	create_color(LTYELLOW);
	create_color(MEYELLOW);
	create_color(MDYELLOW);
	create_color(DKYELLOW);

	create_color(LTPURPLE);
	create_color(MEPURPLE);
	create_color(MDPURPLE);
	create_color(DKPURPLE);

	create_color(FGGREY);
	create_color(MNBLUE);
	create_color(ORANGE);
	create_color(FTGREY);
}

void BC_WindowBase::allocate_color_table()
{
	int red, green, blue, color;
	int result;
	XColor col;

	for(int i = 0; i < total_colors; i++)
	{
		color = color_table[i][0];
		red = (color & 0xFF0000) >> 16;
		green = (color & 0x00FF00) >> 8;
		blue = color & 0xFF;

		col.flags = DoRed | DoGreen | DoBlue;
		col.red   = red<<8   | red;
		col.green = green<<8 | green;
		col.blue  = blue<<8  | blue;

		XAllocColor(display, cmap, &col);
		color_table[i][1] = col.pixel;
	}

	XInstallColormap(display, cmap);
}

void BC_WindowBase::init_window_shape()
{
	if(bg_pixmap && bg_pixmap->get_alpha())
	{
		XShapeCombineMask(top_level->display,
			this->win,
			ShapeBounding,
			0,
			0,
			bg_pixmap->get_alpha(),
			ShapeSet);
	}
}

void BC_WindowBase::init_gc()
{
	unsigned long gcmask;

	gcmask = GCGraphicsExposures;

	XGCValues gcvalues;

	gcvalues.graphics_exposures = 0;        // prevent expose events for every redraw
	gc = XCreateGC(display, rootwin, gcmask, &gcvalues);
}

void BC_WindowBase::init_fonts()
{
	largefont_xft = XftFontOpenName(display, screen,
		resources.large_font_xft);

	mediumfont_xft = XftFontOpenName(display, screen,
		resources.medium_font_xft);

	smallfont_xft = XftFontOpenName(display, screen,
		resources.small_font_xft);

	bold_largefont_xft = XftFontOpenName(display, screen,
		resources.large_b_font_xft);

	bold_mediumfont_xft = XftFontOpenName(display, screen,
		resources.medium_b_font_xft);

	bold_smallfont_xft = XftFontOpenName(display, screen,
		resources.small_b_font_xft);

// Extension failed to locate fonts
	if(!largefont_xft || !mediumfont_xft || !smallfont_xft ||
		!largefont_xft || !mediumfont_xft || !smallfont_xft)
	{
		printf("BC_WindowBase::init_fonts: no xft fonts found\n"
			"    %s=%p %s=%p %s=%p\n"
			"    %s=%p %s=%p %s=%p\n",
			resources.large_font_xft,
			largefont_xft,
			resources.medium_font_xft,
			mediumfont_xft,
			resources.small_font_xft,
			smallfont_xft,
			resources.large_b_font_xft,
			bold_largefont_xft,
			resources.medium_b_font_xft,
			bold_mediumfont_xft,
			resources.small_b_font_xft,
			bold_smallfont_xft);
			exit(1);
	}
}

void BC_WindowBase::init_im()
{
	XIMStyles *xim_styles;
	XIMStyle xim_style;

	if(!(input_method = XOpenIM(display, NULL, NULL, NULL)))
	{
		printf("BC_WindowBase::init_im: Could not open input method.\n");
		exit(1);
	}
	if(XGetIMValues(input_method, XNQueryInputStyle, &xim_styles, NULL) ||
			xim_styles == NULL)
	{
		printf("BC_WindowBase::init_im: Input method doesn't support any styles.\n");
		XCloseIM(input_method);
		exit(1);
	}

	xim_style = 0;
	for(int z = 0;  z < xim_styles->count_styles;  z++)
	{
		if(xim_styles->supported_styles[z] == (XIMPreeditNothing | XIMStatusNothing))
		{
			xim_style = xim_styles->supported_styles[z];
			break;
		}
	}
	XFree(xim_styles);

	if(xim_style == 0)
	{
		printf("BC_WindowBase::init_im: Input method doesn't support the style we need.\n");
		XCloseIM(input_method);
		exit(1);
	}

	input_context = XCreateIC(input_method, XNInputStyle, xim_style,
		XNClientWindow, win, XNFocusWindow, win, NULL);

	if(!input_context)
	{
		printf("BC_WindowBase::init_im: Failed to create input context.\n");
		XCloseIM(input_method);
		exit(1);
	}
}

int BC_WindowBase::get_color(int color) 
{
// return pixel of color
// use this only for drawing subwindows not for bitmaps
	int i, test, difference, result;

	switch(color_model)
	{
	case BC_RGB8:
		if(private_color)
		{
			return get_color_rgb8(color);
		}
		else
		{
// test last color looked up
			if(current_color_value == color) return current_color_pixel;

// look up in table
			current_color_value = color;
			for(i = 0; i < total_colors; i++)
			{
				if(color_table[i][0] == color)
				{
					current_color_pixel = color_table[i][1];
					return current_color_pixel;
				}
			}

// find nearest match
			difference = 0xFFFFFF;

			for(i = 0, result = 0; i < total_colors; i++)
			{
				test = abs((int)(color_table[i][0] - color));

				if(test < difference) 
				{
					current_color_pixel = color_table[i][1]; 
					difference = test;
				}
			}

			return current_color_pixel;
		}
		break;

	case BC_RGB565:
		return get_color_rgb16(color);
		break;

	case BC_BGR565:
		return get_color_bgr16(color);
		break;

	case BC_RGB888:
	case BC_BGR888:
		if(client_byte_order == server_byte_order)
			return color;
		else
			get_color_bgr24(color);
		break;

	default:
		return color;
	}
	return 0;
}

int BC_WindowBase::get_color_rgb8(int color)
{
	int pixel;

	pixel = (color & 0xc00000) >> 16;
	pixel += (color & 0xe000) >> 10;
	pixel += (color & 0xe0) >> 5;
	return pixel;
}

int BC_WindowBase::get_color_rgb16(int color)
{
	int result;

	result = (color & 0xf80000) >> 8;
	result += (color & 0xfc00) >> 5;
	result += (color & 0xf8) >> 3;
	return result;
}

int BC_WindowBase::get_color_bgr16(int color)
{
	int result;

	result = (color & 0xf80000) >> 19;
	result += (color & 0xfc00) >> 5;
	result += (color & 0xf8) << 8;
	return result;
}

int BC_WindowBase::get_color_bgr24(int color)
{
	int result;

	result = (color & 0xff) << 16;
	result += (color & 0xff00);
	result += (color & 0xff0000) >> 16;
	return result;
}

void BC_WindowBase::start_video()
{
	cursor_timer->update();
	video_on = 1;
}

void BC_WindowBase::stop_video()
{
	video_on = 0;
	unhide_cursor();
}

int BC_WindowBase::get_color()
{
	return current_color;
}

void BC_WindowBase::set_color(int color)
{
	current_color = color;
}

void BC_WindowBase::set_current_color(int color)
{
	if(color == -1)
		color = current_color;
	else
		current_color = color;
	XSetForeground(top_level->display, 
		top_level->gc, 
		get_color(color));
}

void BC_WindowBase::set_opaque()
{
	XSetFunction(top_level->display, top_level->gc, GXcopy);
}

void BC_WindowBase::set_inverse() 
{
	XSetFunction(top_level->display, top_level->gc, GXxor);
}

Cursor BC_WindowBase::get_cursor_struct(int cursor)
{
	switch(cursor)
	{
	case ARROW_CURSOR:
		return top_level->arrow_cursor;
	case CROSS_CURSOR:
		return top_level->cross_cursor;
	case IBEAM_CURSOR:
		return top_level->ibeam_cursor;
	case VSEPARATE_CURSOR:
		return top_level->vseparate_cursor;
	case HSEPARATE_CURSOR:
		return top_level->hseparate_cursor;
	case MOVE_CURSOR:
		return top_level->move_cursor;
	case LEFT_CURSOR:
		return top_level->left_cursor;
	case RIGHT_CURSOR:
		return top_level->right_cursor;
	case UPRIGHT_ARROW_CURSOR:
		return top_level->upright_arrow_cursor;
	case UPLEFT_RESIZE:
		return top_level->upleft_resize_cursor;
	case UPRIGHT_RESIZE:
		return top_level->upright_resize_cursor;
	case DOWNLEFT_RESIZE:
		return top_level->downleft_resize_cursor;
	case DOWNRIGHT_RESIZE:
		return top_level->downright_resize_cursor;
	case HOURGLASS_CURSOR:
		return top_level->hourglass_cursor;
	case TRANSPARENT_CURSOR:
		return top_level->transparent_cursor;
	}
	return 0;
}

void BC_WindowBase::set_cursor(int cursor, int override)
{
// don't change cursor if overridden
	if((!top_level->is_hourglass && !is_transparent) || 
		override)
	{
		XDefineCursor(top_level->display, win, get_cursor_struct(cursor));
		flush();
	}

	if(!override) current_cursor = cursor;
}

void BC_WindowBase::set_x_cursor(int cursor)
{
	temp_cursor = XCreateFontCursor(top_level->display, cursor);
	XDefineCursor(top_level->display, win, temp_cursor);
	current_cursor = cursor;
	flush();
}

int BC_WindowBase::get_cursor()
{
	return current_cursor;
}

void BC_WindowBase::start_hourglass()
{
	top_level->start_hourglass_recursive();
	top_level->flush();
}

void BC_WindowBase::stop_hourglass()
{
	top_level->stop_hourglass_recursive();
	top_level->flush();
}

void BC_WindowBase::start_hourglass_recursive()
{
	if(this == top_level)
	{
		hourglass_total++;
		is_hourglass = 1;
	}

	if(!is_transparent)
	{
		set_cursor(HOURGLASS_CURSOR, 1);
		for(int i = 0; subwindows && i < subwindows->total; i++)
		{
			subwindows->values[i]->start_hourglass_recursive();
		}
	}
}

void BC_WindowBase::stop_hourglass_recursive()
{
	if(this == top_level)
	{
		if(hourglass_total == 0) return;
		top_level->hourglass_total--;
	}

	if(!top_level->hourglass_total)
	{
		top_level->is_hourglass = 0;

// Cause set_cursor to perform change
		if(!is_transparent)
			set_cursor(current_cursor, 1);

		for(int i = 0; subwindows && i < subwindows->total; i++)
		{
			subwindows->values[i]->stop_hourglass_recursive();
		}
	}
}

XftFont* BC_WindowBase::get_xft_struct(int font)
{
	switch(font)
	{
	case MEDIUMFONT:
		return (XftFont*)top_level->mediumfont_xft;
	case SMALLFONT:
		return (XftFont*)top_level->smallfont_xft;
	case LARGEFONT:
		return (XftFont*)top_level->largefont_xft;
	case MEDIUMFONT_3D:
		return (XftFont*)top_level->bold_mediumfont_xft;
	case SMALLFONT_3D:
		return (XftFont*)top_level->bold_smallfont_xft;
	case LARGEFONT_3D:
		return (XftFont*)top_level->bold_largefont_xft;
	}

	return 0;
}

void BC_WindowBase::set_font(int font)
{
	top_level->current_font = font;
}

int BC_WindowBase::get_single_text_width(int font, const char *text, int length)
{
	if(get_xft_struct(font))
	{
		XGlyphInfo extents;
		if(resources.locale_utf8)
		{
			XftTextExtentsUtf8(top_level->display,
				get_xft_struct(font),
				(const FcChar8*)text,
				length,
				&extents);
		}
		else
		{
			XftTextExtents8(top_level->display,
				get_xft_struct(font),
				(const FcChar8*)text,
				length,
				&extents);
		}
		return extents.xOff;
	}
	else
	if(font == MEDIUM_7SEGMENT)
		return resources.medium_7segment[0]->get_w() * length;
	return 0;
}

int BC_WindowBase::get_single_text_width(int font, const wchar_t *text, int length)
{
	XGlyphInfo extents;

	XftTextExtents32(top_level->display,
		get_xft_struct(font),
		(const FcChar32*)text,
		length,
		&extents);
	return extents.xOff;
}

int BC_WindowBase::get_text_width(int font, const char *text, int length)
{
	int i, j, w = 0, line_w = 0;
	if(length < 0) length = strlen(text);

	for(i = 0, j = 0; i <= length; i++)
	{
		line_w = 0;
		if(text[i] == '\n')
		{
			line_w = get_single_text_width(font, &text[j], i - j);
			j = i + 1;
		}
		else
		if(text[i] == 0)
		{
			line_w = get_single_text_width(font, &text[j], length - j);
		}
		if(line_w > w) w = line_w;
	}

	if(i > length && w == 0)
	{
		w = get_single_text_width(font, text, length);
	}
	return w;
}

int BC_WindowBase::get_text_width(int font, const wchar_t *text, int length)
{
	int i, j, w = 0, line_w = 0;
	if(length < 0) length = wcslen(text);

	for(i = 0, j = 0; i <= length; i++)
	{
		line_w = 0;
		if(text[i] == '\n')
		{
			line_w = get_single_text_width(font, &text[j], i - j);
			j = i + 1;
		}
		else
		if(text[i] == 0)
		{
			line_w = get_single_text_width(font, &text[j], length - j);
		}
		if(line_w > w) w = line_w;
	}

	if(i > length && w == 0)
	{
		w = get_single_text_width(font, text, length);
	}
	return w;
}

int BC_WindowBase::get_text_ascent(int font)
{
	XftFont *fstruct;

	if(font == MEDIUM_7SEGMENT)
		return resources.medium_7segment[0]->get_h();

	if(fstruct = get_xft_struct(font))
		return fstruct->ascent;
	return 0;
}

int BC_WindowBase::get_text_descent(int font)
{
	XftFont *fstruct;

	if(fstruct = get_xft_struct(font))
		return fstruct->descent;
	return 0;
}

int BC_WindowBase::get_text_height(int font, const wchar_t *text)
{
	XftFont *fstruct;
	int rowh;

	if(fstruct = get_xft_struct(font))
		rowh = fstruct->height;
	else
		rowh = get_text_ascent(font) + get_text_descent(font);

	if(!text)
		return rowh;

	// Add height of lines
	int h = 0, i, length = wcslen(text);

	for(i = 0; i <= length; i++)
	{
		if(text[i] == '\n')
			h++;
		else
		if(text[i] == 0)
			h++;
	}
	return h * rowh;
}

BC_Bitmap* BC_WindowBase::new_bitmap(int w, int h, int color_model)
{
	if(color_model < 0) color_model = top_level->get_color_model();
	return new BC_Bitmap(top_level, w, h, color_model);
}

int BC_WindowBase::accel_available(int color_model, int w, int h)
{
	int i, j, k;
	struct xv_adapterinfo *adpt;

	if(window_type != MAIN_WINDOW) 
		return top_level->accel_available(color_model, w, h);

	if(!resources.use_xvideo) return 0;

// Only local server is fast enough.
	if(!resources.use_shm) return 0;

	for(i = 0; i < BC_DisplayInfo::num_adapters; i++)
	{
		adpt = &BC_DisplayInfo::xv_adapters[i];

		if(adpt->width < w || adpt->height < h)
			continue;

		for(j = 0; j < adpt->num_cmodels; j++)
		{
			if(adpt->cmodels[j] == color_model)
			{
				for(k = 0; k < adpt->num_ports; k++)
				{
					if(Success == XvGrabPort(top_level->display,
							adpt->base_port + k,
							CurrentTime))
					{
						xvideo_port_id = adpt->base_port + k;
// Set XV_AUTOPAINT_COLORKEY if it is supported
						if(adpt->attributes[XV_ATTRIBUTE_AUTOPAINT_COLORKEY].flags & XvSettable)
						{
							XvSetPortAttribute(top_level->display, xvideo_port_id, 
								adpt->attributes[XV_ATTRIBUTE_AUTOPAINT_COLORKEY].xatom, 1);
						}
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

int BC_WindowBase::accel_cmodels(int *cmodels, int len)
{
	int i, j, k, num, cm;
	struct xv_adapterinfo *adpt;

	if(window_type != MAIN_WINDOW) 
		return top_level->accel_cmodels(cmodels, len);

	if(!resources.use_xvideo) return 0;

// Only local server is fast enough.
	if(!resources.use_shm) return 0;

	num = 0;
	for(i = 0; i < BC_DisplayInfo::num_adapters; i++)
	{
		adpt = &BC_DisplayInfo::xv_adapters[i];

		for(j = 0; j < adpt->num_cmodels; j++)
		{
			cm = adpt->cmodels[j];
			for(k = 0; k < num; k++)
				if(cmodels[k] == cm)
					break;
			if(k >= num)
			{
				cmodels[num++] = cm;
				if(num >= len)
					return num;
			}
		}
	}
	return num;
}

void BC_WindowBase::show_window(int flush)
{
	int completion = 0;

	top_level->lock_window("BC_WindowBase::show_window");
	for(int i = 0; i < COMPLETITION_BITMAPS; i++)
	{
		if(top_level->completition_bitmap[i])
		{
			top_level->completition_bitmap[i]->completion_drain(BITMAP_DRAIN_SHOW, this);
			completion = 1;
		}
	}
	if(!completion)
	{
		XMapWindow(top_level->display, win);
		if(flush)
			XFlush(top_level->display);
		hidden = 0;
	}
	top_level->unlock_window();
}

void BC_WindowBase::hide_window(int flush)
{
	int completion = 0;

	top_level->lock_window("BC_WindowBase::hide_window");
	for(int i = 0; i < COMPLETITION_BITMAPS; i++)
	{
		if(top_level->completition_bitmap[i])
		{
			top_level->completition_bitmap[i]->completion_drain(BITMAP_DRAIN_HIDE, this);
			completion = 1;
		}
	}
	if(!completion)
	{
		XUnmapWindow(top_level->display, win);
		if(flush)
			XFlush(top_level->display);
		hidden = 1;
	}
	top_level->unlock_window();
}

BC_MenuBar* BC_WindowBase::add_menubar(BC_MenuBar *menu_bar)
{
	if(subwindows)
		subwindows->append((BC_SubWindow*)menu_bar);

	menu_bar->parent_window = this;
	menu_bar->top_level = this->top_level;
	menu_bar->initialize();
	return menu_bar;
}

BC_WindowBase* BC_WindowBase::add_subwindow(BC_WindowBase *subwindow)
{
	if(subwindows)
		subwindows->append(subwindow);

	if(subwindow->bg_color == -1) subwindow->bg_color = this->bg_color;

// parent window must be set before the subwindow initialization
	subwindow->parent_window = this;
	subwindow->top_level = this->top_level;

// Execute derived initialization
	subwindow->initialize();
	return subwindow;
}


BC_WindowBase* BC_WindowBase::add_tool(BC_WindowBase *subwindow)
{
	return add_subwindow(subwindow);
}

void BC_WindowBase::flash(int x, int y, int w, int h, int flush)
{
	set_opaque();
	XSetWindowBackgroundPixmap(top_level->display, win, pixmap->opaque_pixmap);
	if(x >= 0)
	{
		XClearArea(top_level->display, win, x, y, w, h, 0);
	}
	else
	{
		XClearWindow(top_level->display, win);
	}

	if(flush)
		this->flush();
}

void BC_WindowBase::flash(int flush)
{
	flash(-1, -1, -1, -1, flush);
}

void BC_WindowBase::flush()
{
	XFlush(top_level->display);
}

void BC_WindowBase::sync_display()
{
	lock_window("BC_WindowBase::sync_display");
	XSync(top_level->display, False);
	unlock_window();
}

int BC_WindowBase::get_window_lock()
{
	return top_level->window_lock;
}

void BC_WindowBase::lock_window(const char *location) 
{
	if(top_level && top_level != this)
	{
		top_level->lock_window(location);
	}
	else
	if(top_level)
	{
		SET_XLOCK(this, title, location);
		XLockDisplay(display);
		SET_LOCK2
		top_level->window_lock++;
	}
	else
	{
		printf("BC_WindowBase::lock_window top_level NULL\n");
	}
}

void BC_WindowBase::unlock_window()
{
	if(top_level && top_level != this)
	{
		top_level->unlock_window();
	}
	else
	if(top_level)
	{
		UNSET_LOCK(this);
		if(--top_level->window_lock < 0)
			top_level->window_lock = 0;
		XUnlockDisplay(display);
	}
	else
	{
		printf("BC_WindowBase::unlock_window top_level NULL\n");
	}
}

void BC_WindowBase::lock_events(const char *location)
{
	if(top_level)
	{
		if(top_level != this)
			top_level->lock_events(location);
		else
			event_lock->lock(location);
	}
	else
		printf("BC_WindowBase::lock_event top_level NULL\n");
}

void BC_WindowBase::unlock_events()
{
	if(top_level)
	{
		if(top_level != this)
			top_level->unlock_events();
		else
			event_lock->unlock();
	}
	else
		printf("BC_WindowBase::unlock_event top_level NULL\n");
}

void BC_WindowBase::set_done(int return_value)
{
	if(window_type != MAIN_WINDOW)
		top_level->set_done(return_value);
	else
	{
		XEvent event;

		event.type = ClientMessage;
		event.xclient.display = display;
		event.xclient.window = win;
		event.xclient.message_type = SetDoneXAtom;
		event.xclient.format = 32;
		event.xclient.data.l[0] = return_value;

		XSendEvent(display,
			win,
			0,
			0,
			&event);
		flush();
	}
}

int BC_WindowBase::get_w()
{
	return w;
}

int BC_WindowBase::get_h()
{
	return h;
}

int BC_WindowBase::get_x()
{
	return x;
}

int BC_WindowBase::get_y()
{
	return y;
}

void BC_WindowBase::get_dimensions(int *width, int *height)
{
	*width = w;
	*height = h;
}

int BC_WindowBase::get_root_w(int ignore_dualhead, int lock_display)
{
	if(lock_display) lock_window("BC_WindowBase::get_root_w");
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	int result = WidthOfScreen(screen_ptr);
// Wider than 16:9, narrower than dual head
	if(!ignore_dualhead) 
		if((float)result / HeightOfScreen(screen_ptr) > 1.8) result /= 2;

	if(lock_display) unlock_window();
	return result;
}

int BC_WindowBase::get_root_h(int lock_display)
{
	if(lock_display) lock_window("BC_WindowBase::get_root_h");
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	int result = HeightOfScreen(screen_ptr);
	if(lock_display) unlock_window();
	return result;
}

// Bottom right corner
int BC_WindowBase::get_x2()
{
	return w + x;
}

int BC_WindowBase::get_y2()
{
	return y + h;
}

int BC_WindowBase::get_video_on()
{
	return video_on;
}

int BC_WindowBase::get_hidden()
{
	return top_level->hidden;
}

int BC_WindowBase::cursor_inside()
{
	return (top_level->cursor_x >= 0 && 
			top_level->cursor_y >= 0 && 
			top_level->cursor_x < w && 
			top_level->cursor_y < h);
}

BC_WindowBase* BC_WindowBase::get_top_level()
{
	return top_level;
}

BC_WindowBase* BC_WindowBase::get_parent()
{
	return parent_window;
}

int BC_WindowBase::get_color_model()
{
	return top_level->color_model;
}

BC_Resources* BC_WindowBase::get_resources()
{
	return &BC_WindowBase::resources;
}

GLThread* BC_WindowBase::get_glthread()
{
	return BC_WindowBase::resources.get_glthread();
}

int BC_WindowBase::get_bg_color()
{
	return bg_color;
}

BC_Pixmap* BC_WindowBase::get_bg_pixmap()
{
	return bg_pixmap;
}

void BC_WindowBase::set_active_subwindow(BC_WindowBase *subwindow)
{
	top_level->active_subwindow = subwindow;
}

void BC_WindowBase::deactivate()
{
	if(window_type == MAIN_WINDOW)
	{
		if(top_level->active_menubar) top_level->active_menubar->deactivate();
		if(top_level->active_popup_menu) top_level->active_popup_menu->deactivate();
		if(top_level->active_subwindow) top_level->active_subwindow->deactivate();

		top_level->active_menubar = 0;
		top_level->active_popup_menu = 0;
		top_level->active_subwindow = 0;
	}
}

void BC_WindowBase::cycle_textboxes(int amount)
{
	int result = 0;
	BC_WindowBase *new_textbox = 0;

	if(amount > 0)
	{
		BC_WindowBase *first_textbox = 0;
		find_next_textbox(&first_textbox, &new_textbox, result);
		if(!new_textbox) new_textbox = first_textbox;
	}
	else
	if(amount < 0)
	{
		BC_WindowBase *last_textbox = 0;
		find_prev_textbox(&last_textbox, &new_textbox, result);
		if(!new_textbox) new_textbox = last_textbox;
	}

	if(new_textbox != active_subwindow)
	{
		deactivate();
		new_textbox->activate();
	}
}

void BC_WindowBase::find_next_textbox(BC_WindowBase **first_textbox, BC_WindowBase **next_textbox, int &result)
{
// Search subwindows for textbox
	for(int i = 0; subwindows && i < subwindows->total && result < 2; i++)
	{
		BC_WindowBase *test_subwindow = subwindows->values[i];
		test_subwindow->find_next_textbox(first_textbox, next_textbox, result);
	}

	if(result < 2)
	{
		if(uses_text())
		{
			if(!*first_textbox) *first_textbox = this;

			if(result < 1)
			{
				if(top_level->active_subwindow == this)
					result++;
			}
			else
			{
				result++;
				*next_textbox = this;
			}
		}
	}
}

void BC_WindowBase::find_prev_textbox(BC_WindowBase **last_textbox, BC_WindowBase **prev_textbox, int &result)
{
	if(result < 2)
	{
		if(uses_text())
		{
			if(!*last_textbox) *last_textbox = this;

			if(result < 1)
			{
				if(top_level->active_subwindow == this)
					result++;
			}
			else
			{
				result++;
				*prev_textbox = this;
			}
		}
	}

// Search subwindows for textbox
	if(subwindows)
	{
		for(int i = subwindows->total - 1; subwindows && i >= 0 && result < 2; i--)
		{
			BC_WindowBase *test_subwindow = subwindows->values[i];
			test_subwindow->find_prev_textbox(last_textbox, prev_textbox, result);
		}
	}
}

BC_Clipboard* BC_WindowBase::get_clipboard()
{
	return top_level->clipboard;
}

void BC_WindowBase::get_relative_cursor_pos(int *rel_x, int *rel_y)
{
	int abs_x, abs_y;
	unsigned int temp_mask;
	Window temp_win;

	XQueryPointer(top_level->display,
			win,
			&temp_win,
			&temp_win,
			&abs_x,
			&abs_y,
			rel_x,
			rel_y,
			&temp_mask);
}

void BC_WindowBase::get_abs_cursor_pos(int *abs_x, int *abs_y)
{
	lock_window("BC_WindowBase::get_abs_cursor");
	BC_Resources::get_abs_cursor(abs_x, abs_y);
	unlock_window();
}

int BC_WindowBase::match_window(Window win)
{
	if(this->win == win)
		return 1;

	for(int i = 0; subwindows && i < subwindows->total; i++)
	{
		if(subwindows->values[i]->match_window(win))
			return 1;
	}
	return 0;
}

int BC_WindowBase::get_cursor_over_window(int *rel_x, int *rel_y)
{
	int abs_x, abs_y;
	unsigned int temp_mask;
	Window temp_win1, temp_win2;

	if(!XQueryPointer(top_level->display,
			win,
			&temp_win1,
			&temp_win2,
			&abs_x,
			&abs_y,
			rel_x,
			rel_y,
			&temp_mask))
		return 0;

	return !temp_win2 || match_window(temp_win2);
}

int BC_WindowBase::get_drag_x()
{
	return top_level->drag_x;
}

int BC_WindowBase::get_drag_y()
{
	return top_level->drag_y;
}

int BC_WindowBase::get_cursor_x()
{
	return top_level->cursor_x;
}

int BC_WindowBase::get_cursor_y()
{
	return top_level->cursor_y;
}

int BC_WindowBase::is_event_win()
{
	return this->win == top_level->event_win;
}

void BC_WindowBase::set_dragging(int value)
{
	is_dragging = value;
}

int BC_WindowBase::get_dragging()
{
	return is_dragging;
}

int BC_WindowBase::get_buttonpress()
{
	return top_level->button_number;
}

int BC_WindowBase::get_button_down()
{
	return top_level->button_down;
}

int BC_WindowBase::alt_down()
{
	return top_level->alt_mask;
}

int BC_WindowBase::shift_down()
{
	return top_level->shift_mask;
}

int BC_WindowBase::ctrl_down()
{
	return top_level->ctrl_mask;
}

wchar_t* BC_WindowBase::get_wkeystring(int *length)
{
	if(length)
		*length = top_level->wkey_string_length;
	return top_level->wkey_string;
}

int BC_WindowBase::get_keypress()
{
	return top_level->key_pressed;
}

int BC_WindowBase::get_double_click()
{
	return top_level->double_click;
}

int BC_WindowBase::get_bgcolor()
{
	return bg_color;
}

void BC_WindowBase::resize_window(int w, int h)
{
	if(window_type == MAIN_WINDOW && !allow_resize)
	{
		XSizeHints size_hints;
		size_hints.flags = PSize | PMinSize | PMaxSize;
		size_hints.width = w;
		size_hints.height = h;
		size_hints.min_width = w;
		size_hints.max_width = w; 
		size_hints.min_height = h;
		size_hints.max_height = h; 
		XSetNormalHints(top_level->display, win, &size_hints);
	}
	XResizeWindow(top_level->display, win, w, h);

	this->w = w;
	this->h = h;
	delete pixmap;
	pixmap = new BC_Pixmap(this, w, h);

// Propagate to menubar
	for(int i = 0; subwindows && i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_resize_event(w, h);
	}

	draw_background(0, 0, w, h);
	if(top_level == this && resources.recursive_resizing)
		resize_history.append(new BC_ResizeCall(w, h));
}

// The only way for resize events to be propagated is by updating the internal w and h
void BC_WindowBase::resize_event(int w, int h)
{
	if(window_type == MAIN_WINDOW)
	{
		this->w = w;
		this->h = h;
	}
}

void BC_WindowBase::reposition_widget(int x, int y, int w, int h) 
{
	reposition_window(x, y, w, h);
}

void BC_WindowBase::reposition_window(int x, int y, int w, int h)
{
	int resize = 0;

// Some tools set their own dimensions before calling this, causing the 
// resize check to skip.
	this->x = x;
	this->y = y;

	if(w > 0 && w != this->w)
	{
		resize = 1;
		this->w = w;
	}

	if(h > 0 && h != this->h)
	{
		resize = 1;
		this->h = h;
	}

	if(translation_count && window_type == MAIN_WINDOW)
	{
// KDE shifts window right and down.
// FVWM leaves window alone and adds border around it.
		XMoveResizeWindow(top_level->display, 
			win, 
			x + BC_DisplayInfo::left_border - BC_DisplayInfo::auto_reposition_x, 
			y + BC_DisplayInfo::top_border - BC_DisplayInfo::auto_reposition_y, 
			this->w,
			this->h);
	}
	else
	{
		XMoveResizeWindow(top_level->display, 
			win, 
			x, 
			y, 
			this->w, 
			this->h);
	}

	if(resize)
	{
		delete pixmap;
		pixmap = new BC_Pixmap(this, this->w, this->h);
		draw_background(0, 0, this->w, this->h);
// Propagate to menubar
		for(int i = 0; subwindows && i < subwindows->total; i++)
		{
			subwindows->values[i]->dispatch_resize_event(this->w, this->h);
		}
	}
}

void BC_WindowBase::set_tooltips(int tooltips_enabled)
{
	resources.tooltips_enabled = tooltips_enabled;
}

void BC_WindowBase::raise_window(int do_flush)
{
	XRaiseWindow(top_level->display, win);
	if(do_flush) XFlush(top_level->display);
}

void BC_WindowBase::set_background(VFrame *bitmap)
{
	if(bg_pixmap && !shared_bg_pixmap) delete bg_pixmap;

	bg_pixmap = new BC_Pixmap(this, 
			bitmap, 
			PIXMAP_OPAQUE);
	shared_bg_pixmap = 0;
	draw_background(0, 0, w, h);
}

void BC_WindowBase::set_title(const char *text)
{
	XTextProperty titleprop;
	char *txlist[2];

	strcpy(this->title, text);
	txlist[0] = this->title;
	txlist[1] = 0;

	XmbTextListToTextProperty(top_level->display, txlist, 1,
		XStdICCTextStyle, &titleprop);
	XSetWMName(top_level->display, top_level->win, &titleprop);
	XSetWMIconName(top_level->display, top_level->win, &titleprop);
	XFree(titleprop.value);

	flush();
}

void BC_WindowBase::set_utf8title(const char *text)
{
	XTextProperty titleprop;
	char *txlist[2];

	strcpy(this->title, text);
	txlist[0] = this->title;
	txlist[1] = 0;

	Xutf8TextListToTextProperty(top_level->display, txlist,  1,
		XUTF8StringStyle, &titleprop);
	XSetWMName(top_level->display, top_level->win, &titleprop);
	XSetWMIconName(top_level->display, top_level->win, &titleprop);
	XFree(titleprop.value);
	flush();
}

char* BC_WindowBase::get_title()
{
	return title;
}

int BC_WindowBase::get_toggle_value()
{
	return toggle_value;
}

int BC_WindowBase::get_toggle_drag()
{
	return toggle_drag;
}

void BC_WindowBase::set_icon(VFrame *data)
{
	top_level->lock_window("BC_WindowBase::set_icon");
	icon_vframe = data;
	if(icon_pixmap) delete icon_pixmap;
	if(icon_window) delete icon_window;
	icon_pixmap = new BC_Pixmap(top_level, 
		data, 
		PIXMAP_ALPHA,
		1);

	icon_window = new BC_Popup(this, 
		(int)BC_INFINITY, 
		(int)BC_INFINITY, 
		icon_pixmap->get_w(), 
		icon_pixmap->get_h(), 
		-1, 
		1, // All windows are hidden initially
		icon_pixmap);

	XWMHints wm_hints;
	wm_hints.flags = WindowGroupHint | IconPixmapHint | IconMaskHint | IconWindowHint;
	wm_hints.icon_pixmap = icon_pixmap->get_pixmap();
	wm_hints.icon_mask = icon_pixmap->get_alpha();
	wm_hints.icon_window = icon_window->win;
	wm_hints.window_group = XGroupLeader;

	XSetWMHints(top_level->display, top_level->win, &wm_hints);
	XSync(top_level->display, 0);
	top_level->unlock_window();
}

int BC_WindowBase::set_w(int w)
{
	this->w = w;
	return 0;
}

int BC_WindowBase::set_h(int h)
{
	this->h = h;
	return 0;
}

// For some reason XTranslateCoordinates can take a long time to return.
// We work around this by only calling it when the event windows are different.
void BC_WindowBase::translate_coordinates(Window src_w, 
		Window dest_w,
		int src_x,
		int src_y,
		int *dest_x_return,
		int *dest_y_return)
{
	Window tempwin = 0;

	if(src_w == dest_w)
	{
		*dest_x_return = src_x;
		*dest_y_return = src_y;
	}
	else
	{
		lock_window("BC_WindowBase::translate_coordinates");
		XTranslateCoordinates(top_level->display, 
			src_w, 
			dest_w, 
			src_x, 
			src_y, 
			dest_x_return, 
			dest_y_return,
			&tempwin);
		unlock_window();
	}
}

#ifdef HAVE_LIBXXF86VM
void BC_WindowBase::closest_vm(int *vm, int *width, int *height)
{
	int foo, bar;
	*vm = 0;
	if(XF86VidModeQueryExtension(top_level->display,&foo,&bar))
	{
		int vm_count, i;
		XF86VidModeModeInfo **vm_modelines;
		XF86VidModeGetAllModeLines(top_level->display,XDefaultScreen(top_level->display),&vm_count,&vm_modelines);
		for (i = 0; i < vm_count; i++)
		{
			if (vm_modelines[i]->hdisplay < vm_modelines[*vm]->hdisplay && vm_modelines[i]->hdisplay >= *width)
				*vm = i;
		}
		display = top_level->display;
		if (vm_modelines[*vm]->hdisplay == *width)
			*vm = -1;
		else
		{
			*width = vm_modelines[*vm]->hdisplay;
			*height = vm_modelines[*vm]->vdisplay;
		}
	}
}

void BC_WindowBase::scale_vm(int vm)
{
	int foo,bar,dotclock;
	if(XF86VidModeQueryExtension(top_level->display,&foo,&bar))
	{
		int vm_count;
		XF86VidModeModeInfo **vm_modelines;
		XF86VidModeModeLine vml;
		XF86VidModeGetAllModeLines(top_level->display, XDefaultScreen(top_level->display), &vm_count, &vm_modelines);
		XF86VidModeGetModeLine(top_level->display, XDefaultScreen(top_level->display), &dotclock, &vml);
		orig_modeline.dotclock = dotclock;
		orig_modeline.hdisplay = vml.hdisplay;
		orig_modeline.hsyncstart = vml.hsyncstart;
		orig_modeline.hsyncend = vml.hsyncend;
		orig_modeline.htotal = vml.htotal;
		orig_modeline.vdisplay = vml.vdisplay;
		orig_modeline.vsyncstart = vml.vsyncstart;
		orig_modeline.vsyncend = vml.vsyncend;
		orig_modeline.vtotal = vml.vtotal;
		orig_modeline.flags = vml.flags;
		orig_modeline.privsize = vml.privsize;
		XF86VidModeSwitchToMode(top_level->display, XDefaultScreen(top_level->display), vm_modelines[vm]);
		XF86VidModeSetViewPort(top_level->display, XDefaultScreen(top_level->display), 0, 0);
		XFlush(top_level->display);
	}
}

void BC_WindowBase::restore_vm()
{
	XF86VidModeSwitchToMode(top_level->display, XDefaultScreen(top_level->display), &orig_modeline);
	XFlush(top_level->display);
}


#endif


int BC_WindowBase::get_id()
{
	return id;
}

void BC_WindowBase::set_protowatch()
{
	BC_Signals::watchXproto(top_level->display);
}
