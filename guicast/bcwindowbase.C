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
#include "bcwindowbase.h"
#include "colormodels.h"
#include "colors.h"
#include "condition.h"
#include "cursors.h"
#include "defaults.h"
#include "fonts.h"
#include "keys.h"
#include "language.h"
#include "sizes.h"
#include "vframe.h"

#ifdef HAVE_GL
#include <GL/gl.h>
#endif
#include <string.h>
#include <unistd.h>

#include <X11/extensions/Xvlib.h>
#include <X11/extensions/shape.h>


BC_ResizeCall::BC_ResizeCall(int w, int h)
{
	this->w = w;
	this->h = h;
}


BC_WindowTree::BC_WindowTree(Display* display, 
	BC_WindowTree *parent_tree, 
	Window root_win)
{
	Window temp_win;
	Window *stack;
	int stack_total;

	this->display = display;
	this->win = root_win;
	this->parent_tree = parent_tree;

	XQueryTree(display, 
		root_win, 
		&temp_win, 
		&temp_win, 		&stack, 
		(unsigned int*)&stack_total);

	for(int i = 0; i < stack_total; i++)
	{
		windows.append(new BC_WindowTree(display, this, stack[i]));
	}
	XFree(stack);
}

BC_WindowTree::~BC_WindowTree()
{
	windows.remove_all_objects();
}

BC_WindowTree* BC_WindowTree::get_node(Window win)
{
//printf("BC_WindowTree::get_node 1\n");
	for(int i = 0; i < windows.total; i++)
	{
		BC_WindowTree *result = 0;
//printf("BC_WindowTree::get_node 2\n");
		if(windows.values[i]->win == win)
			return windows.values[i];
		else
		{
//printf("BC_WindowTree::get_node 3\n");
			result = windows.values[i]->get_node(win);
//printf("BC_WindowTree::get_node 4\n");
			if(result) return result;
		}
	}
}


void BC_WindowTree::dump(int indent, Window caller_win)
{
	XWindowAttributes attr;
	XGetWindowAttributes(display,
    		win,
    		&attr);
	for(int i = 0; i < indent; i++)
		printf(" ");
	printf("x=%d y=%d w=%d h=%d win=%x subwindows=%d ", 
		attr.x, attr.y, attr.width, attr.height, win, windows.total);
	if(caller_win == win) printf("**");
	printf("\n");
	for(int i = 0; i < windows.total; i++)
		windows.values[i]->dump(indent + 1, caller_win);
}




Mutex BC_WindowBase::opengl_lock;

BC_Resources BC_WindowBase::resources;
BC_WindowTree* BC_WindowBase::window_tree = 0;

BC_WindowBase::BC_WindowBase()
{
//printf("BC_WindowBase::BC_WindowBase 1\n");
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

//printf("BC_WindowBase::~BC_WindowBase 1\n");
	hide_tooltip();
	if(window_type != MAIN_WINDOW)
	{
		if(top_level->active_menubar == this) top_level->active_menubar = 0;
		if(top_level->active_popup_menu == this) top_level->active_popup_menu = 0;
		if(top_level->active_subwindow == this) top_level->active_subwindow = 0;
		parent_window->subwindows->remove(this);
	}
//printf("BC_WindowBase::~BC_WindowBase 2\n");

	if(subwindows)
	{
		for(int i = 0; i < subwindows->total; i++)
		{
			delete subwindows->values[i];
		}
//printf("BC_WindowBase::~BC_WindowBase 2.5 %p\n", subwindows);
		delete subwindows;
	}
//printf("BC_WindowBase::~BC_WindowBase 3\n");

	XFreePixmap(top_level->display, pixmap);
	XDestroyWindow(top_level->display, win);

//printf("BC_WindowBase::~BC_WindowBase 4\n");
	if(bg_pixmap && !shared_bg_pixmap) delete bg_pixmap;
	if(icon_pixmap) delete icon_pixmap;
	if (temp_bitmap) delete temp_bitmap;
//printf("BC_WindowBase::~BC_WindowBase 5\n");


	if(window_type == MAIN_WINDOW) 
	{
		
		XFreeGC(top_level->display, gc);
#ifdef HAVE_XFT
		if(top_level->largefont_xft) 
			XftFontClose (top_level->display, (XftFont*)top_level->largefont_xft);
		if(top_level->mediumfont_xft) 
			XftFontClose (top_level->display, (XftFont*)top_level->mediumfont_xft);
		if(top_level->smallfont_xft) 
			XftFontClose (top_level->display, (XftFont*)top_level->smallfont_xft);
#endif
		flush();
// Can't close display if another thread is waiting for events
		XCloseDisplay(top_level->display);
		clipboard->stop_clipboard();
		delete clipboard;


	}
//printf("BC_WindowBase::~BC_WindowBase 6\n");

	resize_history.remove_all_objects();
	UNSET_ALL_LOCKS(this)
//printf("BC_WindowBase::~BC_WindowBase 10\n");
}

int BC_WindowBase::initialize()
{
	window_lock = 0;
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
	bg_pixmap = 0;
	tooltip_text[0] = 0;
	persistant_tooltip = 0;
//	next_repeat_id = 0;
	tooltip_popup = 0;
	tooltip_done = 0;
	current_font = MEDIUMFONT;
	current_cursor = ARROW_CURSOR;
	current_color = BLACK;
	is_dragging = 0;
	shared_bg_pixmap = 0;
	icon_pixmap = 0;
	window_type = MAIN_WINDOW;
	translation_count = 0;
	x_correction = y_correction = 0;
	temp_bitmap = 0;
	tooltip_on = 0;
	temp_cursor = 0;
	toggle_value = 0;
	has_focus = 0;
#ifdef HAVE_LIBXXF86VM
    vm_switched = 0;
#endif
	xft_drawable = 0;
	largefont_xft = 0;
	mediumfont_xft = 0;
	smallfont_xft = 0;

	return 0;
}

#define DEFAULT_EVENT_MASKS EnterWindowMask | \
			LeaveWindowMask | \
			ButtonPressMask | \
			ButtonReleaseMask | \
			PointerMotionMask | \
			FocusChangeMask
			

int BC_WindowBase::create_window(BC_WindowBase *parent_window,
				char *title, 
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
				char *display_name,
				int window_type,
				BC_Pixmap *bg_pixmap)
{
	XSetWindowAttributes attr;
	unsigned long mask;
	XSizeHints size_hints;
	int root_w;
	int root_h;
#ifdef HAVE_LIBXXF86VM
    int vm;
#endif

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
	strcpy(this->title, _(title));
	if(bg_pixmap) shared_bg_pixmap = 1;

	if(parent_window) top_level = parent_window->top_level;

	subwindows = new BC_SubWindowList;

// Mandatory setup
	if(window_type == MAIN_WINDOW)
	{
		top_level = this;
		parent_window = this;

// This function must be the first Xlib
// function a multi-threaded program calls
		XInitThreads();


// get the display connection
		display = init_display(display_name);

// Fudge window placement
		root_w = get_root_w(1);
		root_h = get_root_h();
		if(this->x + this->w > root_w) this->x = root_w - this->w;
		if(this->y + this->h > root_h) this->y = root_h - this->h;
		if(this->x < 0) this->x = 0;
		if(this->y < 0) this->y = 0;
		screen = DefaultScreen(display);

		rootwin = RootWindow(display, screen);
		vis = DefaultVisual(display, screen);
		default_depth = DefaultDepth(display, screen);
		client_byte_order = (*(u_int32_t*)"a   ") & 0x00000001;
		server_byte_order = (XImageByteOrder(display) == MSBFirst) ? 0 : 1;



// This must be done before fonts to know if antialiasing is available.
		init_colors();
// get the resources
		if(resources.use_shm < 0) resources.initialize_display(this);
		x_correction = get_resources()->get_left_border();
		y_correction = get_resources()->get_top_border();

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

		XSetStandardProperties(display, 
			win, 
			title, 
			title, 
			None, 
			0, 
			0, 
			&size_hints);
		get_atoms();
		
		clipboard = new BC_Clipboard(display_name);
		clipboard->start_clipboard();
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

		attr.event_mask = DEFAULT_EVENT_MASKS;

		if(this->bg_color == -1)
			this->bg_color = resources.get_bg_color();
		attr.background_pixel = top_level->get_color(bg_color);
		attr.colormap = top_level->cmap;
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
		mask = CWBackPixel | CWEventMask;
		attr.event_mask = DEFAULT_EVENT_MASKS;
		attr.background_pixel = top_level->get_color(this->bg_color);
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
	pixmap = XCreatePixmap(top_level->display, 
		win, 
		this->w, 
		this->h, 
		top_level->default_depth);

// Create truetype rendering surface
#ifdef HAVE_XFT
	if(get_resources()->use_xft)
	{
// printf("BC_WindowBase::create_window 1 %p %p %p %p\n", 
// top_level->display,
// pixmap,
// top_level->vis,
// top_level->cmap);
		xft_drawable = XftDrawCreate(top_level->display,
		       pixmap,
		       top_level->vis,
		       top_level->cmap);
// printf("BC_WindowBase::create_window 10 %p %p %p %p %p\n", 
// xft_drawable, 
// top_level->display,
// pixmap,
// top_level->vis,
// top_level->cmap);
	}
#endif

// Set up options for main window
	if(window_type == MAIN_WINDOW)
	{
		if(get_resources()->bg_image && !bg_pixmap && bg_color < 0)
		{
			this->bg_pixmap = new BC_Pixmap(this, 
				get_resources()->bg_image, 
				PIXMAP_OPAQUE);
		}

		if(!hidden) show_window();

	}




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
	return 0;
}

Display* BC_WindowBase::init_display(char *display_name)
{
	Display* display;

	if(display_name && display_name[0] == 0) display_name = NULL;
	if((display = XOpenDisplay(display_name)) == NULL)
	{
  		printf("BC_WindowBase::create_window: cannot connect to X server %s\n", 
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
				printf("BC_WindowBase::create_window: cannot connect to default X server.\n");
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

// Start tooltips
	if(window_type == MAIN_WINDOW)
	{
//		tooltip_id = get_repeat_id();
		set_repeat(get_resources()->tooltip_delay);
	}

	while(!done)
	{
		dispatch_event();
	}
//printf("BC_WindowBase::run_window 3\n");

	unset_all_repeaters();
//printf("BC_WindowBase::run_window 4\n");
	hide_tooltip();

	return return_value;
}

int BC_WindowBase::get_key_masks(XEvent &event)
{
// ctrl key down
	ctrl_mask = (event.xkey.state & ControlMask) ? 1 : 0;
// shift key down
	shift_mask = (event.xkey.state & ShiftMask) ? 1 : 0;
	alt_mask = (event.xkey.state & Mod1Mask) ? 1 : 0;
//printf("BC_WindowBase::get_key_masks %x\n", event.xkey.state);
	return 0;
}







int BC_WindowBase::dispatch_event()
{
	XEvent event;
    Window tempwin;
  	KeySym keysym;
  	char keys_return[2];
	int result;
	XClientMessageEvent *ptr;
	int temp;
	int cancel_resize, cancel_translation;

	key_pressed = 0;

// If an event is waiting get it, otherwise
// wait for next event only if there are no compressed events.
	if(XPending(display) || 
		(!motion_events && !resize_events && !translation_events))
	{
		XNextEvent(display, &event);
// Lock out window deletions
		lock_window("BC_WindowBase::dispatch_event 1");
		get_key_masks(event);
	}
	else
// Handle compressed events
	{
		lock_window("BC_WindowBase::dispatch_event 2");
		if(resize_events)
			dispatch_resize_event(last_resize_w, last_resize_h);
		else
		if(motion_events)
			dispatch_motion_event();
		else
		if(translation_events)
			dispatch_translation_event();

		unlock_window();
		return 0;
	}

	switch(event.type)
	{
		case ClientMessage:
// Clear the resize buffer
			if(resize_events) dispatch_resize_event(last_resize_w, last_resize_h);
// Clear the motion buffer since this can clear the window
			if(motion_events) dispatch_motion_event();

			ptr = (XClientMessageEvent*)&event;


//printf("BC_WindowBase::dispatch_event 3 %d\n", ptr->data.l[0]);
        	if(ptr->message_type == ProtoXAtom && 
				ptr->data.l[0] == DelWinXAtom)
        	{
				close_event();
			}
			else
			if(ptr->message_type == RepeaterXAtom)
			{
				dispatch_repeat_event(ptr->data.l[0]);
// Make sure the repeater still exists.
// 				for(int i = 0; i < repeaters.total; i++)
// 				{
// 					if(repeaters.values[i]->repeat_id == ptr->data.l[0])
// 					{
// 						dispatch_repeat_event_master(ptr->data.l[0]);
// 						break;
// 					}
// 				}
			}
			else
			if(ptr->message_type == SetDoneXAtom)
			{
				done = 1;
			}
			break;

		case FocusIn:
			has_focus = 1;
			dispatch_focus_in();
			break;

		case FocusOut:
			has_focus = 0;
			dispatch_focus_out();
			break;

		case ButtonPress:
			cursor_x = event.xbutton.x;
			cursor_y = event.xbutton.y;
			button_number = event.xbutton.button;
			event_win = event.xany.window;
  			button_down = 1;
			button_pressed = event.xbutton.button;
			button_time1 = button_time2;
			button_time2 = event.xbutton.time;
			drag_x = cursor_x;
			drag_y = cursor_y;
			drag_win = event_win;
			drag_x1 = cursor_x - get_resources()->drag_radius;
			drag_x2 = cursor_x + get_resources()->drag_radius;
			drag_y1 = cursor_y - get_resources()->drag_radius;
			drag_y2 = cursor_y + get_resources()->drag_radius;

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
			button_number = event.xbutton.button;
			event_win = event.xany.window;
  			button_down = 0;

  			dispatch_button_release();
			break;

		case Expose:
			event_win = event.xany.window;
			dispatch_expose_event();
			break;

		case MotionNotify:
// Dispatch previous motion event if this is a subsequent motion from a different window
{
//Timer timer;
//timer.update();

			if(motion_events && last_motion_win != event.xany.window)
				dispatch_motion_event();

// Buffer the current motion
			motion_events = 1;
			last_motion_x = event.xmotion.x;
			last_motion_y = event.xmotion.y;
			last_motion_win = event.xany.window;
//printf("BC_WindowBase::dispatch_event 1 %lld\n", timer.get_difference());
}
			break;

		case ConfigureNotify:
			XTranslateCoordinates(top_level->display, 
				top_level->win, 
				top_level->rootwin, 
				0, 
				0, 
				&last_translate_x, 
				&last_translate_y, 
				&tempwin);
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
			{
				resize_events = 1;
			}

			if((last_translate_x == x && last_translate_y == y))
				cancel_translation = 1;

			if(!cancel_translation)
			{
				translation_events = 1;
			}

			translation_count++;
			break;

		case KeyPress:
  			keys_return[0] = 0;
  			XLookupString((XKeyEvent*)&event, keys_return, 1, &keysym, 0);
//printf("BC_WindowBase::dispatch_event %08x\n", keys_return[0]);
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
					//key_pressed = keys_return[0]; 
					key_pressed = keysym & 0xff;
					break;
			}

//printf("BC_WindowBase::run_window %d %d %x\n", shift_down(), alt_down(), key_pressed);
			result = dispatch_keypress_event();
// Handle some default keypresses
			if(!result)
			{
				if(key_pressed == 'w' ||
					key_pressed == 'W')
				{
					close_event();
				}
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
	}

	unlock_window();
	return 0;
}

int BC_WindowBase::dispatch_expose_event()
{
	int result = 0;
	for(int i = 0; i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_expose_event();
	}

// Propagate to user
	if(!result) expose_event();
	return result;
}

int BC_WindowBase::dispatch_resize_event(int w, int h)
{
	if(window_type == MAIN_WINDOW)
	{
		resize_events = 0;
// Can't store w and h here because bcfilebox depends on the old w and h to
// reposition widgets.
		XFreePixmap(top_level->display, pixmap);
		pixmap = XCreatePixmap(top_level->display, 
			win, 
			w, 
			h, 
			top_level->default_depth);
		clear_box(0, 0, w, h);
	}

// Propagate to subwindows
	for(int i = 0; i < subwindows->total; i++)
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
	return 0;
}

int BC_WindowBase::dispatch_translation_event()
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

	for(int i = 0; i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_translation_event();
	}

//printf("BC_WindowBase::dispatch_translation_event 1 %p\n", this);
	translation_event();
	return 0;
}

int BC_WindowBase::dispatch_motion_event()
{
	int result = 0;

	if(top_level == this)
	{
		event_win = last_motion_win;
		motion_events = 0;

//printf("BC_WindowBase::dispatch_motion_event 1\n");
// Test for grab
		if(get_button_down() && !active_menubar && !active_popup_menu)
		{
//printf("BC_WindowBase::dispatch_motion_event 2\n");
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
// 				if(window_tree)
// 				{
// 					delete window_tree;
// 					window_tree = 0;
// 				}
// 				if(!window_tree)
// 				{
// 					window_tree = new BC_WindowTree(display, 
// 						0, 
// 						rootwin);
// //window_tree->dump(0, win);
// 				}

				result = dispatch_drag_start();
			}
		}
		cursor_x = last_motion_x;
		cursor_y = last_motion_y;

//printf("BC_WindowBase::dispatch_motion_event 3\n");
		if(active_menubar && !result) result = active_menubar->dispatch_motion_event();
		if(active_popup_menu && !result) result = active_popup_menu->dispatch_motion_event();
		if(active_subwindow && !result) result = active_subwindow->dispatch_motion_event();
//printf("BC_WindowBase::dispatch_motion_event 4\n");
	}

//printf("BC_WindowBase::dispatch_motion_event 5\n");
	for(int i = 0; i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_motion_event();
	}
//printf("BC_WindowBase::dispatch_motion_event 6\n");

	if(!result) result = cursor_motion_event();    // give to user
//printf("BC_WindowBase::dispatch_motion_event 7\n");
	return result;
}

int BC_WindowBase::dispatch_keypress_event()
{
	int result = 0;
	if(top_level == this)
	{
		if(active_subwindow) result = active_subwindow->dispatch_keypress_event();
	}

	for(int i = 0; i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_keypress_event();
	}

	if(!result) result = keypress_event();

	return result;
}

int BC_WindowBase::dispatch_focus_in()
{
	for(int i = 0; i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_focus_in();
	}

	focus_in_event();

	return 0;
}

int BC_WindowBase::dispatch_focus_out()
{
	for(int i = 0; i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_focus_out();
	}

	focus_out_event();

	return 0;
}

int BC_WindowBase::get_has_focus()
{
	return top_level->has_focus;
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

	for(int i = 0; i < subwindows->total && !result; i++)
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
//printf("BC_WindowBase::dispatch_button_release 1 %d\n", result);
		if(active_popup_menu && !result) result = active_popup_menu->dispatch_button_release();
//printf("BC_WindowBase::dispatch_button_release 2 %p %d\n", active_subwindow, result);
		if(active_subwindow && !result) result = active_subwindow->dispatch_button_release();
//printf("BC_WindowBase::dispatch_button_release 3 %d\n", result);
		if(!result) result = dispatch_drag_stop();
	}
//printf("BC_WindowBase::dispatch_button_release 4 %d\n", result);

	for(int i = 0; i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_button_release();
	}
//printf("BC_WindowBase::dispatch_button_release 5 %d\n", result);

	if(!result)
	{
		result = button_release_event();
	}
//printf("BC_WindowBase::dispatch_button_release 6 %d\n", result);

	return result;
}

// int BC_WindowBase::dispatch_repeat_event_master(long duration)
// {
// 	int result = 0;
// 	BC_Repeater *repeater;
// 
// // Unlock the repeater if it still exists.
// 	for(int i = 0; i < repeaters.total; i++)
// 	{
// 		if(repeaters.values[i]->repeat_id == repeat_id)
// 		{
// 			repeater = repeaters.values[i];
// 			if(repeater->interrupted)
// 			{
// // Disregard
// 				if(interrupt_now)
// 				{
// // Delete now
// 					repeater->join();
// 					repeaters.remove(repeater);
// 					delete repeater;
// 				}
// 			}
// 			else
// 			{
// // Propogate to subwindows
// 				if(active_menubar) result = active_menubar->dispatch_repeat_event(repeat_id);
// 				if(!result && active_subwindow) result = active_subwindow->dispatch_repeat_event(repeat_id);
// 				if(!result) result = dispatch_repeat_event(repeat_id);
// 				repeater->repeat_mutex.unlock();
// 			}
// 			i = repeaters.total;
// 		}
// 	}
// 
// 	return result;
// }

int BC_WindowBase::dispatch_repeat_event(int64_t duration)
{
// all repeat event handlers get called and decide based on activity and duration
// whether to respond
	for(int i = 0; i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_repeat_event(duration);
	}
	repeat_event(duration);

// Unlock next signal
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

	return 0;
}

int BC_WindowBase::dispatch_cursor_leave()
{
	for(int i = 0; i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_cursor_leave();
	}
	cursor_leave_event();
	return 0;
}

int BC_WindowBase::dispatch_cursor_enter()
{
	int result = 0;

	if(active_menubar) result = active_menubar->dispatch_cursor_enter();
	if(!result && active_popup_menu) result = active_popup_menu->dispatch_cursor_enter();
	if(!result && active_subwindow) result = active_subwindow->dispatch_cursor_enter();

	for(int i = 0; !result && i < subwindows->total; i++)
	{
		result = subwindows->values[i]->dispatch_cursor_enter();
	}

	if(!result) result = cursor_enter_event();
	return result;
}

int BC_WindowBase::cursor_enter_event()
{
	return 0;
}

int BC_WindowBase::cursor_leave_event()
{
	return 0;
}

int BC_WindowBase::close_event()
{
	set_done(1);
	return 1;
}

int BC_WindowBase::dispatch_drag_start()
{
	int result = 0;
	if(active_menubar) result = active_menubar->dispatch_drag_start();
	if(!result && active_popup_menu) result = active_popup_menu->dispatch_drag_start();
	if(!result && active_subwindow) result = active_subwindow->dispatch_drag_start();
	
	for(int i = 0; i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_drag_start();
	}

	if(!result) result = is_dragging = drag_start_event();
	return result;
}

int BC_WindowBase::dispatch_drag_stop()
{
	int result = 0;

	for(int i = 0; i < subwindows->total && !result; i++)
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
	for(int i = 0; i < subwindows->total && !result; i++)
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





int BC_WindowBase::show_tooltip(int w, int h)
{
	Window tempwin;

	if(!tooltip_on && get_resources()->tooltips_enabled)
	{
		int i, j, x, y;
		top_level->hide_tooltip();

		tooltip_on = 1;
		if(w < 0)
			w = get_text_width(MEDIUMFONT, tooltip_text);

		if(h < 0)
			h = get_text_height(MEDIUMFONT, tooltip_text);

		w += TOOLTIP_MARGIN * 2;
		h += TOOLTIP_MARGIN * 2;

		XTranslateCoordinates(top_level->display, 
				win, 
				top_level->rootwin, 
				get_w(), 
				get_h(), 
				&x, 
				&y, 
				&tempwin);
		tooltip_popup = new BC_Popup(top_level, 
					x,
					y,
					w, 
					h, 
					get_resources()->tooltip_bg_color);

		draw_tooltip();
		tooltip_popup->set_font(MEDIUMFONT);
		tooltip_popup->flash();
	}
	return 0;
}

int BC_WindowBase::hide_tooltip()
{
	if(subwindows)
		for(int i = 0; i < subwindows->total; i++)
		{
			subwindows->values[i]->hide_tooltip();
		}

	if(tooltip_on)
	{
		tooltip_on = 0;
		delete tooltip_popup;
		tooltip_popup = 0;
	}
	return 0;
}

int BC_WindowBase::set_tooltip(char *text)
{
	strcpy(this->tooltip_text, text);
// Update existing tooltip if it is visible
	if(tooltip_on)
	{
		draw_tooltip();
		tooltip_popup->flash();
	}
	return 0;
}

// signal the event handler to repeat
int BC_WindowBase::set_repeat(int64_t duration)
{
	if(duration <= 0)
	{
		printf("BC_WindowBase::set_repeat duration=%d\n", duration);
		return 0;
	}
	if(window_type != MAIN_WINDOW) return top_level->set_repeat(duration);

// test repeater database for duplicates
	for(int i = 0; i < repeaters.total; i++)
	{
// Already exists
		if(repeaters.values[i]->delay == duration)
		{
			repeaters.values[i]->start_repeating();
			return 0;
		}
	}

	BC_Repeater *repeater = new BC_Repeater(this, duration);
	repeater->initialize();
	repeaters.append(repeater);
    repeater->start_repeating();
	return 0;
}

int BC_WindowBase::unset_repeat(int64_t duration)
{
	if(window_type != MAIN_WINDOW) return top_level->unset_repeat(duration);

	BC_Repeater *repeater = 0;
	for(int i = 0; i < repeaters.total; i++)
	{
		if(repeaters.values[i]->delay == duration)
		{
			repeaters.values[i]->stop_repeating();
		}
	}
	return 0;
}


int BC_WindowBase::unset_all_repeaters()
{
//printf("BC_WindowBase::unset_all_repeaters 1\n");
	for(int i = 0; i < repeaters.total; i++)
	{
		repeaters.values[i]->stop_repeating();
	}
//printf("BC_WindowBase::unset_all_repeaters 1\n");
	repeaters.remove_all_objects();
//printf("BC_WindowBase::unset_all_repeaters 2\n");
	return 0;
}

// long BC_WindowBase::get_repeat_id()
// {
// 	return top_level->next_repeat_id++;
// }

int BC_WindowBase::arm_repeat(int64_t duration)
{
	XEvent event;
	XClientMessageEvent *ptr = (XClientMessageEvent*)&event;
	ptr->type = ClientMessage;
	ptr->message_type = RepeaterXAtom;
	ptr->format = 32;
	ptr->data.l[0] = duration;

	XSendEvent(display, win, 0, 0, &event);
	flush();
	return 0;
}

int BC_WindowBase::get_atoms()
{
	SetDoneXAtom =  XInternAtom(display, "BC_REPEAT_EVENT", False);
	RepeaterXAtom = XInternAtom(display, "BC_CLOSE_EVENT", False);
	DelWinXAtom =   XInternAtom(display, "WM_DELETE_WINDOW", False);
	if(ProtoXAtom = XInternAtom(display, "WM_PROTOCOLS", False))
		XChangeProperty(display, win, ProtoXAtom, XA_ATOM, 32, PropModeReplace, (unsigned char *)&DelWinXAtom, True);
	return 0;
}

void BC_WindowBase::init_cursors()
{
	arrow_cursor = XCreateFontCursor(display, XC_top_left_arrow);
	cross_cursor = XCreateFontCursor(display, XC_crosshair);
	ibeam_cursor = XCreateFontCursor(display, XC_xterm);
	vseparate_cursor = XCreateFontCursor(display, XC_sb_v_double_arrow);
	hseparate_cursor = XCreateFontCursor(display, XC_sb_h_double_arrow);
	move_cursor = XCreateFontCursor(display, XC_fleur);
	left_cursor = XCreateFontCursor(display, XC_left_side);
	right_cursor = XCreateFontCursor(display, XC_right_side);
	upright_arrow_cursor = XCreateFontCursor(display, XC_arrow);
	upleft_resize_cursor = XCreateFontCursor(display, XC_top_left_corner);
	upright_resize_cursor = XCreateFontCursor(display, XC_top_right_corner);
	downleft_resize_cursor = XCreateFontCursor(display, XC_bottom_left_corner);
	downright_resize_cursor = XCreateFontCursor(display, XC_bottom_right_corner);
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
			color_model = server_byte_order ? BC_BGR8888 : BC_RGBA8888;
			break;
	}
	return color_model;
}

int BC_WindowBase::init_colors()
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
			get_resources()->use_xft = 0;
			break;

		default:
 			cmap = DefaultColormap(display, screen);
			break;
	}
	return 0;
}

int BC_WindowBase::create_private_colors()
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
	return 0;
}


int BC_WindowBase::create_color(int color)
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
	return 0;
}

int BC_WindowBase::create_shared_colors()
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
	return 0;
}

int BC_WindowBase::allocate_color_table()
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
	return 0;
}

int BC_WindowBase::init_window_shape()
{
	if(bg_pixmap && bg_pixmap->use_alpha()) 
	{
		XShapeCombineMask(top_level->display,
    		this->win,
    		ShapeBounding,
    		0,
    		0,
    		bg_pixmap->get_alpha(),
    		ShapeSet);
	}
	return 0;
}


int BC_WindowBase::init_gc()
{
	unsigned long gcmask;
	gcmask = GCFont | GCGraphicsExposures;

	XGCValues gcvalues;
	gcvalues.font = mediumfont->fid;        // set the font
	gcvalues.graphics_exposures = 0;        // prevent expose events for every redraw
	gc = XCreateGC(display, rootwin, gcmask, &gcvalues);
	return 0;
}

int BC_WindowBase::init_fonts()
{
	if((largefont = XLoadQueryFont(display, _(resources.large_font))) == NULL) 
		largefont = XLoadQueryFont(display, "fixed"); 

	if((mediumfont = XLoadQueryFont(display, _(resources.medium_font))) == NULL)
		mediumfont = XLoadQueryFont(display, "fixed"); 

	if((smallfont = XLoadQueryFont(display, _(resources.small_font))) == NULL)
		smallfont = XLoadQueryFont(display, "fixed");

#ifdef HAVE_XFT
	if(get_resources()->use_xft)
	{


//printf("BC_WindowBase::init_fonts 1 %p %p %s\n", display, screen, resources.large_font_xft);

		if(!(largefont_xft = XftFontOpenXlfd(display,
		    screen,
		    resources.large_font_xft)))
		{
			largefont_xft = XftFontOpenXlfd(display,
		    	screen,
		    	"fixed");
		}
//printf("BC_WindowBase::init_fonts 1 %p\n", largefont_xft);
		if(!(largefont_xft = XftFontOpenXlfd(display,
		    screen,
		    resources.large_font_xft)))
		{
			largefont_xft = XftFontOpenXlfd(display,
		    	screen,
		    	"fixed");
		}
//printf("BC_WindowBase::init_fonts 2 %p\n", largefont_xft);


		if(!(mediumfont_xft = XftFontOpenXlfd(display,
		      screen,
		      resources.medium_font_xft)))
		{
			mediumfont_xft = XftFontOpenXlfd(display,
		    	screen,
		    	"fixed");
		}


		if(!(smallfont_xft = XftFontOpenXlfd(display,
		      screen,
		      resources.small_font_xft)))
		{
			  smallfont_xft = XftFontOpenXlfd(display,
		    	  screen,
		    	  "fixed");
		}

//printf("BC_WindowBase::init_fonts 100 %s %p\n", 
//resources.medium_font, 
//mediumfont_xft);

printf("BC_WindowBase::init_fonts: %s=%p %s=%p %s=%p\n",
	resources.large_font_xft,
	largefontset,
	resources.medium_font_xft,
	mediumfontset,
	resources.small_font_xft,
	smallfontset);

// Extension failed to locate fonts
		if(!largefontset || !mediumfontset || !smallfontset)
		{
			printf("BC_WindowBase::init_fonts: no xft fonts found %s=%p %s=%p %s=%p\n",
				resources.large_font_xft,
				largefontset,
				resources.medium_font_xft,
				mediumfontset,
				resources.small_font_xft,
				smallfontset);
			get_resources()->use_xft = 0;
		}
	}
	else
#endif
	if(get_resources()->use_fontset)
	{
		char **m, *d;
		int n;

// FIXME: should check the m,d,n values
		if((largefontset = XCreateFontSet(display, 
			resources.large_fontset,
            &m, 
			&n, 
			&d)) == 0)
            largefontset = XCreateFontSet(display, "fixed,*", &m, &n, &d);
		if((mediumfontset = XCreateFontSet(display, 
			resources.medium_fontset,
            &m, 
			&n, 
			&d)) == 0)
            mediumfontset = XCreateFontSet(display, "fixed,*", &m, &n, &d);
		if((smallfontset = XCreateFontSet(display, 
			resources.small_fontset,
            &m, 
			&n, 
			&d)) == 0)
            smallfontset = XCreateFontSet(display, "fixed,*", &m, &n, &d);

		if(largefontset && mediumfontset && smallfontset)
		{
			curr_fontset = mediumfontset;
			get_resources()->use_fontset = 1;
		}
		else
		{
			curr_fontset = 0;
			get_resources()->use_fontset = 0;
		}
	}

	return 0;
}

int BC_WindowBase::get_color(int64_t color) 
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
			break;	
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

int64_t BC_WindowBase::get_color_rgb16(int color)
{
	int64_t result;
	result = (color & 0xf80000) >> 8;
	result += (color & 0xfc00) >> 5;
	result += (color & 0xf8) >> 3;
	
	return result;
}

int64_t BC_WindowBase::get_color_bgr16(int color)
{
	int64_t result;
	result = (color & 0xf80000) >> 19;
	result += (color & 0xfc00) >> 5;
	result += (color & 0xf8) << 8;

	return result;
}

int64_t BC_WindowBase::get_color_bgr24(int color)
{
	int64_t result;
	result = (color & 0xff) << 16;
	result += (color & 0xff00);
	result += (color & 0xff0000) >> 16;
	return result;
}

int BC_WindowBase::video_is_on()
{
	return video_on;
}

void BC_WindowBase::start_video()
{
	video_on = 1;
	set_color(BLACK);
	draw_box(0, 0, get_w(), get_h());
	flash();
}

void BC_WindowBase::stop_video()
{
	video_on = 0;
}



int64_t BC_WindowBase::get_color()
{
	return top_level->current_color;
}

void BC_WindowBase::set_color(int64_t color)
{
	top_level->current_color = color;
	XSetForeground(top_level->display, 
		top_level->gc, 
		top_level->get_color(color));
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
		case ARROW_CURSOR:  	   return top_level->arrow_cursor;  	    	   break;
		case CROSS_CURSOR:         return top_level->cross_cursor;
		case IBEAM_CURSOR:  	   return top_level->ibeam_cursor;  	    	   break;
		case VSEPARATE_CURSOR:     return top_level->vseparate_cursor;      	   break;
		case HSEPARATE_CURSOR:     return top_level->hseparate_cursor;      	   break;
		case MOVE_CURSOR:  	       return top_level->move_cursor;          	       break;
		case LEFT_CURSOR:   	   return top_level->left_cursor;   	    	   break;
		case RIGHT_CURSOR:         return top_level->right_cursor;  	    	   break;
		case UPRIGHT_ARROW_CURSOR: return top_level->upright_arrow_cursor;  	   break;
		case UPLEFT_RESIZE:        return top_level->upleft_resize_cursor;     	   break;
		case UPRIGHT_RESIZE:       return top_level->upright_resize_cursor;    	   break;
		case DOWNLEFT_RESIZE:      return top_level->downleft_resize_cursor;   	   break;
		case DOWNRIGHT_RESIZE:     return top_level->downright_resize_cursor;  	   break;
	}
	return 0;
}

void BC_WindowBase::set_cursor(int cursor)
{
	XDefineCursor(top_level->display, win, get_cursor_struct(cursor));
	current_cursor = cursor;
	flush();
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







XFontStruct* BC_WindowBase::get_font_struct(int font)
{
// Clear out unrelated flags
	if(font & BOLDFACE) font ^= BOLDFACE;
	
	switch(font)
	{
		case MEDIUMFONT: return top_level->mediumfont; break;
		case SMALLFONT:  return top_level->smallfont;  break;
		case LARGEFONT:  return top_level->largefont;  break;
	}
	return 0;
}

XFontSet BC_WindowBase::get_fontset(int font)
{
	XFontSet fs = 0;

	if(get_resources()->use_fontset)
	{
		switch(font)
		{
			case SMALLFONT:  fs = top_level->smallfontset; break;
			case LARGEFONT:  fs = top_level->largefontset; break;
			case MEDIUMFONT: fs = top_level->mediumfontset; break;
		}
	}

	return fs;
}

#ifdef HAVE_XFT
XftFont* BC_WindowBase::get_xft_struct(int font)
{
// Clear out unrelated flags
	if(font & BOLDFACE) font ^= BOLDFACE;

	switch(font)
	{
		case MEDIUMFONT:   return (XftFont*)top_level->mediumfont_xft; break;
		case SMALLFONT:    return (XftFont*)top_level->smallfont_xft;  break;
		case LARGEFONT:    return (XftFont*)top_level->largefont_xft;  break;
	}

	return 0;
}
#endif











void BC_WindowBase::set_font(int font)
{
	top_level->current_font = font;


#ifdef HAVE_XFT
	if(get_resources()->use_xft)
	{
		;
	}
	else
#endif
	if(get_resources()->use_fontset)
	{
		set_fontset(font);
	}

	if(get_font_struct(font))
	{
		XSetFont(top_level->display, top_level->gc, get_font_struct(font)->fid);
	}

	return;
}

void BC_WindowBase::set_fontset(int font)
{
	XFontSet fs = 0;

	if(get_resources()->use_fontset)
	{
		switch(font)
		{
			case SMALLFONT:  fs = top_level->smallfontset; break;
			case LARGEFONT:  fs = top_level->largefontset; break;
			case MEDIUMFONT: fs = top_level->mediumfontset; break;
		}
	}

	curr_fontset = fs;
}


XFontSet BC_WindowBase::get_curr_fontset(void)
{
	if(get_resources()->use_fontset)
		return curr_fontset;
	return 0;
}

int BC_WindowBase::get_single_text_width(int font, char *text, int length)
{
#ifdef HAVE_XFT
	if(get_resources()->use_xft && get_xft_struct(font))
	{
		XGlyphInfo extents;
		XftTextExtents8(top_level->display,
			get_xft_struct(font),
			(FcChar8*)text, 
			length,
			&extents);
		return extents.xOff;
	}
	else
#endif
	if(get_resources()->use_fontset && top_level->get_fontset(font))
		return XmbTextEscapement(top_level->get_fontset(font), text, length);
	else
	if(get_font_struct(font)) 
		return XTextWidth(get_font_struct(font), text, length);
	else
	{
		int w = 0;
		switch(font)
		{
			case MEDIUM_7SEGMENT:
				return get_resources()->medium_7segment[0]->get_w() * length;
				break;

			default:
				return 0;
		}
		return w;
	}
}

int BC_WindowBase::get_text_width(int font, char *text, int length)
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

int BC_WindowBase::get_text_ascent(int font)
{
#ifdef HAVE_XFT
	if(get_resources()->use_xft && get_xft_struct(font))
	{
		XGlyphInfo extents;
		XftTextExtents8(top_level->display,
			get_xft_struct(font),
			(FcChar8*)"O", 
			1,
			&extents);
		return extents.y;
	}
	else
#endif
	if(get_resources()->use_fontset && top_level->get_fontset(font))
	{
        XFontSetExtents *extents;

        extents = XExtentsOfFontSet(top_level->get_fontset(font));
        return -extents->max_logical_extent.y;
	}
	else
	if(get_font_struct(font))
		return top_level->get_font_struct(font)->ascent;
	else
	switch(font)
	{
		case MEDIUM_7SEGMENT:
			return get_resources()->medium_7segment[0]->get_h();
			break;

		default:
			return 0;
	}
}

int BC_WindowBase::get_text_descent(int font)
{
#ifdef HAVE_XFT
	if(get_resources()->use_xft && get_xft_struct(font))
	{
		XGlyphInfo extents;
		XftTextExtents8(top_level->display,
			get_xft_struct(font),
			(FcChar8*)"j", 
			1,
			&extents);
		return extents.height - extents.y;
	}
	else
#endif
    if(get_resources()->use_fontset && top_level->get_fontset(font))
    {
        XFontSetExtents *extents;

        extents = XExtentsOfFontSet(top_level->get_fontset(font));
        return (extents->max_logical_extent.height
                        + extents->max_logical_extent.y);
    }
    else
	if(get_font_struct(font))
		return top_level->get_font_struct(font)->descent;
	else
	switch(font)
	{
		default:
			return 0;
	}
}

int BC_WindowBase::get_text_height(int font, char *text)
{
	if(!text) return get_text_ascent(font) + get_text_descent(font);

// Add height of lines
	int h = 0, i, length = strlen(text);
	for(i = 0; i <= length; i++)
	{
		if(text[i] == '\n')
			h++;
		else
		if(text[i] == 0)
			h++;
	}
	return h * (get_text_ascent(font) + get_text_descent(font));
}

BC_Bitmap* BC_WindowBase::new_bitmap(int w, int h, int color_model)
{
	if(color_model < 0) color_model = top_level->get_color_model();
	return new BC_Bitmap(top_level, w, h, color_model);
}

int BC_WindowBase::accel_available(int color_model)
{
	if(window_type != MAIN_WINDOW) 
		return top_level->accel_available(color_model);

	int result = 0;

	switch(color_model)
	{
		case BC_YUV420P:
			result = grab_port_id(this, color_model);
			if(result >= 0)
			{
				xvideo_port_id = result;
				result = 1;
			}
			else
				result = 0;
			break;

		case BC_YUV422P:
			result = 0;
			break;

		case BC_YUV422:
//printf("BC_WindowBase::accel_available 1\n");
			result = grab_port_id(this, color_model);
//printf("BC_WindowBase::accel_available 2 %d\n", result);
			if(result >= 0)
			{
				xvideo_port_id = result;
				result = 1;
			}
			else
				result = 0;
//printf("BC_WindowBase::accel_available 3 %d\n", xvideo_port_id);
			break;

		default:
			result = 0;
			break;
	}
//printf("BC_WindowBase::accel_available %d %d\n", color_model, result);
	return result;
}


int BC_WindowBase::grab_port_id(BC_WindowBase *window, int color_model)
{
	int numFormats, i, j, k;
	unsigned int ver, rev, numAdapt, reqBase, eventBase, errorBase;
	int port_id = -1;
    XvAdaptorInfo *info;
    XvImageFormatValues *formats;
	int x_color_model;

	if(!get_resources()->use_xvideo) return -1;

// Translate from color_model to X color model
	x_color_model = cmodel_bc_to_x(color_model);

// Only local server is fast enough.
	if(!resources.use_shm) return -1;

// XV extension is available
    if(Success != XvQueryExtension(window->display, 
				  &ver, 
				  &rev, 
				  &reqBase, 
				  &eventBase, 
				  &errorBase))
    {
		return -1;
    }

// XV adaptors are available
	XvQueryAdaptors(window->display, 
		DefaultRootWindow(window->display), 
		&numAdapt, 
		&info);

	if(!numAdapt)
	{
		return -1;
	}

// Get adaptor with desired color model
    for(i = 0; i < numAdapt && port_id == -1; i++)
    {
/* adaptor supports XvImages */
		if(info[i].type & XvImageMask) 
		{  
	    	formats = XvListImageFormats(window->display, 
							info[i].base_id, 
							&numFormats);
// for(j = 0; j < numFormats; j++)
// 	printf("%08x\n", formats[j].id);

	    	for(j = 0; j < numFormats; j++) 
	    	{
/* this adaptor supports the desired format */
				if(formats[j].id == x_color_model)
				{
/* Try to grab a port */
					for(k = 0; k < info[i].num_ports; k++)
					{
/* Got a port */
						if(Success == XvGrabPort(top_level->display, 
							info[i].base_id + k, 
							CurrentTime))
						{
//printf("BC_WindowBase::grab_port_id %llx\n", info[i].base_id);
							port_id = info[i].base_id + k;
							break;
						}
					}
				}
			}
	    	if(formats) XFree(formats);
		}
	}

    XvFreeAdaptorInfo(info);

	return port_id;
}


int BC_WindowBase::show_window() 
{ 
	XMapWindow(top_level->display, win); 
	XFlush(top_level->display);
//	XSync(top_level->display, 0);
	hidden = 0; 
	return 0;
}

int BC_WindowBase::hide_window() 
{ 
	XUnmapWindow(display, win); 
	XFlush(top_level->display);
	hidden = 1; 
	return 0;
}

BC_MenuBar* BC_WindowBase::add_menubar(BC_MenuBar *menu_bar)
{
	subwindows->append((BC_SubWindow*)menu_bar);

	menu_bar->parent_window = this;
	menu_bar->top_level = this->top_level;
	menu_bar->initialize();
	return menu_bar;
}

BC_WindowBase* BC_WindowBase::add_subwindow(BC_WindowBase *subwindow)
{
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

int BC_WindowBase::flash(int x, int y, int w, int h)
{
	set_opaque();
	XSetWindowBackgroundPixmap(top_level->display, win, pixmap);
	if(x >= 0)
	{
		XClearArea(top_level->display, win, x, y, w, h, 0);
	}
	else
	{
		XClearWindow(top_level->display, win);
	}
	return 0;
}

void BC_WindowBase::flush()
{
	XFlush(top_level->display);
}

void BC_WindowBase::sync_display()
{
	XSync(top_level->display, False);
}

int BC_WindowBase::get_window_lock()
{
	return top_level->window_lock;
}

int BC_WindowBase::lock_window(char *location) 
{
	SET_LOCK(this, title, location);
	XLockDisplay(top_level->display); 
	top_level->window_lock = 1;
	return 0;
}

int BC_WindowBase::unlock_window() 
{
	UNSET_LOCK(this);
	top_level->window_lock = 0;
	XUnlockDisplay(top_level->display); 
	return 0;
}

void BC_WindowBase::set_done(int return_value)
{
	if(window_type != MAIN_WINDOW) 
		top_level->set_done(return_value);
	else
	{
		XEvent event;
		XClientMessageEvent *ptr = (XClientMessageEvent*)&event;

		event.type = ClientMessage;
		ptr->message_type = SetDoneXAtom;
		ptr->format = 32;
		XSendEvent(top_level->display, top_level->win, 0, 0, &event);
		top_level->return_value = return_value;
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

int BC_WindowBase::get_root_w(int ignore_dualhead)
{
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	int result = WidthOfScreen(screen_ptr);
// Wider than 16:9, narrower than dual head
	if(!ignore_dualhead) if((float)result / HeightOfScreen(screen_ptr) > 1.8) result /= 2;
	return result;
}

int BC_WindowBase::get_root_h()
{
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	return HeightOfScreen(screen_ptr);
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

int BC_WindowBase::activate()
{
	return 0;
}

int BC_WindowBase::deactivate()
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
	return 0;
}

int BC_WindowBase::cycle_textboxes(int amount)
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
	
	return 0;
}

int BC_WindowBase::find_next_textbox(BC_WindowBase **first_textbox, BC_WindowBase **next_textbox, int &result)
{
// Search subwindows for textbox
	for(int i = 0; i < subwindows->total && result < 2; i++)
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
	return 0;
}

int BC_WindowBase::find_prev_textbox(BC_WindowBase **last_textbox, BC_WindowBase **prev_textbox, int &result)
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
	for(int i = subwindows->total - 1; i >= 0 && result < 2; i--)
	{
		BC_WindowBase *test_subwindow = subwindows->values[i];
		test_subwindow->find_prev_textbox(last_textbox, prev_textbox, result);
	}
	return 0;
}

BC_Clipboard* BC_WindowBase::get_clipboard()
{
	return top_level->clipboard;
}

int BC_WindowBase::get_relative_cursor_x()
{
	int abs_x, abs_y, x, y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

	XQueryPointer(top_level->display, 
	   top_level->win, 
	   &temp_win, 
	   &temp_win,
       &abs_x, 
	   &abs_y, 
	   &win_x, 
	   &win_y, 
	   &temp_mask);

	XTranslateCoordinates(top_level->display, 
			top_level->rootwin, 
			win, 
			abs_x, 
			abs_y, 
			&x, 
			&y, 
			&temp_win);

	return x;
}

int BC_WindowBase::get_relative_cursor_y()
{
	int abs_x, abs_y, x, y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

	XQueryPointer(top_level->display, 
	   top_level->win, 
	   &temp_win, 
	   &temp_win,
       &abs_x, 
	   &abs_y, 
	   &win_x, 
	   &win_y, 
	   &temp_mask);

	XTranslateCoordinates(top_level->display, 
			top_level->rootwin, 
			win, 
			abs_x, 
			abs_y, 
			&x, 
			&y, 
			&temp_win);

	return y;
}

int BC_WindowBase::get_abs_cursor_x()
{
	int abs_x, abs_y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

	 XQueryPointer(top_level->display, 
	 	top_level->win, 
		&temp_win, 
		&temp_win,
        &abs_x, 
		&abs_y, 
		&win_x, 
		&win_y, 
		&temp_mask);
	return abs_x;
}

int BC_WindowBase::get_abs_cursor_y()
{
	int abs_x, abs_y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

	 XQueryPointer(top_level->display, 
	 	top_level->win, 
		&temp_win, 
		&temp_win,
        &abs_x, 
		&abs_y, 
		&win_x, 
		&win_y, 
		&temp_mask);
	return abs_y;
}

int BC_WindowBase::match_window(Window win) 
{
	if (this->win == win) return 1;
	int result = 0;
	for(int i = 0; i < subwindows->total; i++)
	{
		result = subwindows->values[i]->match_window(win);
		if (result) return result;
	}
	return 0;

}

int BC_WindowBase::get_cursor_over_window()
{
	if(top_level != this) return top_level->get_cursor_over_window();

	int abs_x, abs_y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win1, temp_win2;
//printf("BC_WindowBase::get_cursor_over_window 2\n");

	if (!XQueryPointer(display, 
		win, 
		&temp_win1, 
		&temp_win2,
		&abs_x, 
		&abs_y, 
		&win_x, 
		&win_y, 
		&temp_mask))
	return(0);
//printf("BC_WindowBase::get_cursor_over_window 3 %p\n", window_tree);

	int result = match_window(temp_win2)	;
//	printf("t1: %p, t2: %p, win: %p, ret: %i\n", temp_win1, temp_win2, win, result);
	return(result);
/*------------------------previous attempts ------------
// Get location in window tree
	BC_WindowTree *tree_node = window_tree->get_node(win);

// KDE -> Window is overlapped by a transparent window.
// FVWM -> Window is bordered by different windows.
	BC_WindowTree *parent_node = tree_node->parent_tree;
	for( ; 
		parent_node->parent_tree != window_tree; 
		tree_node = parent_node, parent_node = parent_node->parent_tree)
	{
		;
	}

	int got_it = 0;
	for(int i = 0; i < parent_node->windows.total; i++)
	{
		if(parent_node->windows.values[i] == tree_node)
		{
			got_it = 1;
		}
		else
		if(got_it)
		{
			XWindowAttributes attr;
			XGetWindowAttributes(display,
    				parent_node->windows.values[i]->win,
    				&attr);

// Obstructed by overlapping window.
			if(abs_x >= attr.x && abs_x < attr.x + attr.width &&
				abs_y >= attr.y && abs_y < attr.y + attr.height &&
				!attr.override_redirect)
			{
				return 0;
			}
		}
	}
*/
# if 0
// Test every window after current node in same parent node.
// Test every parent node after current parent node.
// These are the overlapping windows.
	while(parent_node && parent_node != window_tree)
	{
//printf("BC_WindowBase::get_cursor_over_window 4\n");
		if(parent_node
		for(int i = 0; i < parent_node->windows.total; i++)
		{
//printf("BC_WindowBase::get_cursor_over_window 5\n");
			if(parent_node->windows.values[i] == tree_node)
			{
//printf("BC_WindowBase::get_cursor_over_window 6\n");
				got_it = 1;
			}
			else
			if(got_it)
			{
//printf("BC_WindowBase::get_cursor_over_window 7\n");
				XWindowAttributes attr;
				XGetWindowAttributes(display,
    					parent_node->windows.values[i]->win,
    					&attr);

// Obstructed by overlapping window.
				if(abs_x >= attr.x && abs_x < attr.x + attr.width &&
					abs_y >= attr.y && abs_y < attr.y + attr.height &&
					!attr.override_redirect)
				{
					return 0;
				}
			}
//printf("BC_WindowBase::get_cursor_over_window 8\n");
		}
		tree_node = parent_node;
		parent_node = tree_node->parent_tree;
	}
//printf("BC_WindowBase::get_cursor_over_window 9\n");

#endif

// Didn't find obstruction
	return 1;
}

int BC_WindowBase::relative_cursor_x(BC_WindowBase *pov)
{
	int x, y;
	Window tempwin;

	translate_coordinates(top_level->event_win, 
		pov->win,
		top_level->cursor_x,
		top_level->cursor_y,
		&x,
		&y);
	return x;
}

int BC_WindowBase::relative_cursor_y(BC_WindowBase *pov)
{
	int x, y;
	Window tempwin;

	translate_coordinates(top_level->event_win, 
		pov->win,
		top_level->cursor_x,
		top_level->cursor_y,
		&x,
		&y);
	return y;
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

int BC_WindowBase::resize_window(int w, int h)
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
	XFreePixmap(top_level->display, pixmap);
	pixmap = XCreatePixmap(top_level->display, win, w, h, top_level->default_depth);

// Propagate to menubar
	for(int i = 0; i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_resize_event(w, h);
	}

	draw_background(0, 0, w, h);
	if(top_level == this && get_resources()->recursive_resizing)
		resize_history.append(new BC_ResizeCall(w, h));
	return 0;
}

// The only way for resize events to be propagated is by updating the internal w and h
int BC_WindowBase::resize_event(int w, int h)
{
	if(window_type == MAIN_WINDOW)
	{
		this->w = w;
		this->h = h;
	}
	return 0;
}

int BC_WindowBase::reposition_window(int x, int y, int w, int h)
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

//printf("BC_WindowBase::reposition_window %d %d %d\n", translation_count, x_correction, y_correction);

	if(this->w <= 0)
		printf("BC_WindowBase::reposition_window this->w == %d\n", this->w);
	if(this->h <= 0)
		printf("BC_WindowBase::reposition_window this->h == %d\n", this->h);

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
		XFreePixmap(top_level->display, pixmap);
 		pixmap = XCreatePixmap(top_level->display, 
			win, 
			this->w, 
			this->h, 
			top_level->default_depth);

// Propagate to menubar
		for(int i = 0; i < subwindows->total; i++)
		{
			subwindows->values[i]->dispatch_resize_event(this->w, this->h);
		}

//		draw_background(0, 0, w, h);
	}

	return 0;
}

int BC_WindowBase::set_tooltips(int tooltips_enabled)
{
	get_resources()->tooltips_enabled = tooltips_enabled;
	return 0;
}

int BC_WindowBase::raise_window(int do_flush)
{
	XRaiseWindow(top_level->display, win);
	if(do_flush) XFlush(top_level->display);
	return 0;
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

void BC_WindowBase::set_title(char *text)
{
	XSetStandardProperties(top_level->display, top_level->win, text, text, None, 0, 0, 0); 
	strcpy(this->title, _(text));
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

int BC_WindowBase::set_icon(VFrame *data)
{
	if(icon_pixmap) delete icon_pixmap;
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
	wm_hints.flags = IconPixmapHint | IconMaskHint | IconWindowHint;
	wm_hints.icon_pixmap = icon_pixmap->get_pixmap();
	wm_hints.icon_mask = icon_pixmap->get_alpha();
	wm_hints.icon_window = icon_window->win;
// for(int i = 0; i < 1000; i++)
// printf("02x ", icon_pixmap->get_alpha()->get_row_pointers()[0][i]);
// printf("\n");

	XSetWMHints(top_level->display, top_level->win, &wm_hints);
	XSync(top_level->display, 0);
	return 0;
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

int BC_WindowBase::load_defaults(Defaults *defaults)
{
	get_resources()->filebox_mode = defaults->get("FILEBOX_MODE", get_resources()->filebox_mode);
	get_resources()->filebox_w = defaults->get("FILEBOX_W", get_resources()->filebox_w);
	get_resources()->filebox_h = defaults->get("FILEBOX_H", get_resources()->filebox_h);
	defaults->get("FILEBOX_FILTER", get_resources()->filebox_filter);
	return 0;
}

int BC_WindowBase::save_defaults(Defaults *defaults)
{
	defaults->update("FILEBOX_MODE", get_resources()->filebox_mode);
	defaults->update("FILEBOX_W", get_resources()->filebox_w);
//printf("BC_ListBox::cursor_motion_event 1 %lld\n", timer.get_difference());
	defaults->update("FILEBOX_H", get_resources()->filebox_h);
	defaults->update("FILEBOX_FILTER", get_resources()->filebox_filter);
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
//Timer timer;
//timer.update();
	if(src_w == dest_w)
	{
		*dest_x_return = src_x;
		*dest_y_return = src_y;
	}
	else
	{
		XTranslateCoordinates(top_level->display, 
			src_w, 
			dest_w, 
			src_x, 
			src_y, 
			dest_x_return, 
			dest_y_return,
			&tempwin);
//printf("BC_WindowBase::translate_coordinates 1 %lld\n", timer.get_difference());
	}
}







#ifdef HAVE_LIBXXF86VM
void BC_WindowBase::closest_vm(int *vm, int *width, int *height)
{
   int foo,bar;
   *vm = 0;
   if(XF86VidModeQueryExtension(top_level->display,&foo,&bar)) {
	   int vm_count,i;
	   XF86VidModeModeInfo **vm_modelines;
	   XF86VidModeGetAllModeLines(top_level->display,XDefaultScreen(top_level->display),&vm_count,&vm_modelines);
	   for (i = 0; i < vm_count; i++) {
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
	   XF86VidModeGetAllModeLines(top_level->display,XDefaultScreen(top_level->display),&vm_count,&vm_modelines);
	   XF86VidModeGetModeLine(top_level->display,XDefaultScreen(top_level->display),&dotclock,&vml);
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
	   // orig_modeline.private = vml.private;
	   XF86VidModeSwitchToMode(top_level->display,XDefaultScreen(top_level->display),vm_modelines[vm]);
	   XF86VidModeSetViewPort(top_level->display,XDefaultScreen(top_level->display),0,0);
	   XFlush(top_level->display);
   }
}

void BC_WindowBase::restore_vm()
{
   XF86VidModeSwitchToMode(top_level->display,XDefaultScreen(top_level->display),&orig_modeline);
   XFlush(top_level->display);
}


#endif


#ifdef HAVE_GL

extern "C"
{
	GLXContext glXCreateContext(Display *dpy,
                               XVisualInfo *vis,
                               GLXContext shareList,
                               int direct);
	
  	int glXMakeCurrent(Display *dpy,
                       Drawable drawable,
                       GLXContext ctx);

	void glXSwapBuffers(Display *dpy,
                       Drawable drawable);
};



void BC_WindowBase::enable_opengl()
{
	lock_window("BC_WindowBase::enable_opengl");
	opengl_lock.lock();

	XVisualInfo viproto;
	XVisualInfo *visinfo;
	int nvi;

//printf("BC_WindowBase::enable_opengl 1\n");
	viproto.screen = top_level->screen;
	visinfo = XGetVisualInfo(top_level->display,
    	VisualScreenMask,
    	&viproto,
    	&nvi);
//printf("BC_WindowBase::enable_opengl 1 %p\n", visinfo);

	gl_context = glXCreateContext(top_level->display,
		visinfo,
		0,
		1);
//printf("BC_WindowBase::enable_opengl 1\n");

	glXMakeCurrent(top_level->display,
		win,
		gl_context);

	unsigned long valuemask = CWEventMask;
	XSetWindowAttributes attributes;
	attributes.event_mask = DEFAULT_EVENT_MASKS |
			ExposureMask;
	XChangeWindowAttributes(top_level->display, win, valuemask, &attributes);

	opengl_lock.unlock();
	unlock_window();
//printf("BC_WindowBase::enable_opengl 2\n");
}

void BC_WindowBase::disable_opengl()
{
	unsigned long valuemask = CWEventMask;
	XSetWindowAttributes attributes;
	attributes.event_mask = DEFAULT_EVENT_MASKS;
	XChangeWindowAttributes(top_level->display, win, valuemask, &attributes);
}

void BC_WindowBase::lock_opengl()
{
	lock_window("BC_WindowBase::lock_opengl");
	opengl_lock.lock();
	glXMakeCurrent(top_level->display,
		win,
		gl_context);
}

void BC_WindowBase::unlock_opengl()
{
	opengl_lock.unlock();
	unlock_window();
}

void BC_WindowBase::flip_opengl()
{
	glXSwapBuffers(top_level->display, win);
}

#endif
