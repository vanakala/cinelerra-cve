// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcbitmap.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcwindow.h"
#include "clip.h"
#include "colormodels.h"
#include "vframe.h"

#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <X11/extensions/Xvlib.h>


BC_Bitmap::BC_Bitmap(BC_WindowBase *parent_window, 
	int w, 
	int h, 
	int color_model, 
	int use_shm)
{
	disp_w = 0;
	disp_h = 0;
	base_left = 0;
	base_top = 0;
	initialize(parent_window, 
		w, 
		h, 
		color_model, 
		use_shm);
}

BC_Bitmap::~BC_Bitmap()
{
	delete_data();
}

void BC_Bitmap::initialize(BC_WindowBase *parent_window, 
	int w, 
	int h, 
	int color_model, 
	int use_shm)
{
	this->parent_window = parent_window;
	this->top_level = parent_window->top_level;
	this->w = w;
	this->h = h;
	this->color_model = color_model;
	this->use_shm = use_shm ? parent_window->resources.use_shm : 0;
	for(int i = 0; i < BITMAP_RING; i++)
	{
		ximage[i] = 0;
		xv_image[i] = 0;
		data[i] = 0;
		busyflag[i] = 0;
		pixmaps[i] = 0;
	}
	last_pixmap_used = 0;
	last_pixmap = 0;
	current_ringbuffer = 0;
	ring_buffers = 0;
	xv_portid = 0;
	completion_used = 0;
	drain_action = 0;
	reset_completion();

	if(w == 0 || h == 0)
		return;

	allocate_data();
}

int BC_Bitmap::params_match(int w, int h, int color_model, int use_shm)
{
	int result = 0;
	if(this->w == w &&
		this->h == h &&
		this->color_model == color_model)
	{
		if(use_shm == this->use_shm || use_shm == BC_INFINITY)
			result = 1;
	}
	return result;
}


void BC_Bitmap::allocate_data()
{
	top_level->lock_window("BC_Bitmap::allocate_data");
// Shared memory available
	if(use_shm)
	{
		if(top_level->xvideo_port_id >= 0)
		{
			ring_buffers = BITMAP_RING;
			xv_portid = top_level->xvideo_port_id;
			top_level->xvideo_port_id = -1;
// Create the X Image
			xv_image[0] = XvShmCreateImage(top_level->display, 
						xv_portid, 
						BC_DisplayInfo::cmodel_to_fourcc(color_model),
						0,
						w,
						h,
						&shm_info);
			data_size = xv_image[0]->data_size;
// Create the shared memory
			shm_info.shmid = shmget(IPC_PRIVATE,
				xv_image[0]->data_size * ring_buffers + 4,
				IPC_CREAT | 0600);
			if(shm_info.shmid == -1)
			{
				perror("BC_Bitmap::allocate_data shmget");
				abort();
			}
			data[0] = (unsigned char *)shmat(shm_info.shmid, NULL, 0);
// setting ximage->data stops BadValue
			xv_image[0]->data = shm_info.shmaddr = (char*)data[0];
			shm_info.readOnly = 0;

// Get the real parameters
			w = xv_image[0]->width;
			h = xv_image[0]->height;

// Create remaining X Images
			for(int i = 1; i < ring_buffers; i++)
			{
				data[i] = data[0] + xv_image[0]->data_size * i;
				xv_image[i] = XvShmCreateImage(top_level->display, 
							xv_portid, 
							BC_DisplayInfo::cmodel_to_fourcc(color_model),
							(char*)data[i], 
							w,
							h,
							&shm_info);
				xv_image[i]->data = (char*)data[i];
			}

			if(color_model == BC_YUV422)
			{
				bytes_per_line = w * 2;
				bits_per_pixel = 2;
			}
			else
			{
				bytes_per_line = 0;
				bits_per_pixel = 0;
			}
		}
		else
		{
			ring_buffers = BITMAP_RING;
// Create first X Image
			ximage[0] = XShmCreateImage(top_level->display,
				top_level->vis,
				get_default_depth(),
				get_default_depth() == 1 ? XYBitmap : ZPixmap,
				(char*)NULL,
				&shm_info,
				w,
				h);
			data_size = h * ximage[0]->bytes_per_line;
// Create shared memory
			shm_info.shmid = shmget(IPC_PRIVATE, 
				h * ximage[0]->bytes_per_line * ring_buffers + 4,
				IPC_CREAT | 0600);
			if(shm_info.shmid == -1)
			{
				perror("BC_Bitmap::allocate_data shmget");
				abort();
			}
			data[0] = (unsigned char *)shmat(shm_info.shmid, NULL, 0);
			ximage[0]->data = shm_info.shmaddr = (char*)data[0];  // setting ximage->data stops BadValue
			shm_info.readOnly = 0;

// Get the real parameters
			bits_per_pixel = ximage[0]->bits_per_pixel;
			bytes_per_line = ximage[0]->bytes_per_line;

// Create remaining X Images
			for(int i = 1; i < ring_buffers; i++)
			{
				data[i] = data[0] + h * ximage[0]->bytes_per_line * i;
				ximage[i] = XShmCreateImage(top_level->display, 
						top_level->vis,
						get_default_depth(),
						get_default_depth() == 1 ? XYBitmap : ZPixmap, 
						(char*)data[i],
						&shm_info,
						w,
						h);
				ximage[i]->data = (char*)data[i];
			}
		}
		if(!XShmAttach(top_level->display, &shm_info))
		{
			perror("BC_Bitmap::allocate_data XShmAttach");
			abort();
		}

// This causes it to automatically delete when the program exits.
		shmctl(shm_info.shmid, IPC_RMID, 0);
	}
	else
// Use unshared memory.
	{
		ring_buffers = 1;
// Use RGB frame
		ximage[0] = XCreateImage(top_level->display, 
					top_level->vis, 
					get_default_depth(), 
					get_default_depth() == 1 ? XYBitmap : ZPixmap, 
					0, 
					0,
					w, 
					h, 
					8, 
					0);
		data[0] = (unsigned char*)malloc(h * ximage[0]->bytes_per_line + 4);
		ximage[0]->data = (char *)data[0];

// This differs from the depth parameter of the top_level.
		bits_per_pixel = ximage[0]->bits_per_pixel;
		bytes_per_line = ximage[0]->bytes_per_line;
	}
	XSync(top_level->display, False);
	top_level->unlock_window();
}

void BC_Bitmap::delete_data()
{
	if(data[0])
	{
		top_level->lock_window("BC_Bitmap::delete_data");
		if(use_shm)
		{
			if(completion_used)
			{
				parent_window->unregister_completion(this);
				completion_used = 0;
			}

			if(xv_image[0])
			{
				if(last_pixmap_used) XvStopVideo(top_level->display, xv_portid, last_pixmap);
				XShmDetach(top_level->display, &shm_info);
				XvUngrabPort(top_level->display, xv_portid, CurrentTime);
				for(int i = 0; i < ring_buffers; i++)
				{
					XFree(xv_image[i]);
				}

				shmdt(shm_info.shmaddr);
				shmctl(shm_info.shmid, IPC_RMID, 0);
			}
			else
			{
				XShmDetach(top_level->display, &shm_info);
				for(int i = 0; i < ring_buffers; i++)
					XDestroyImage(ximage[i]);

				shmdt(shm_info.shmaddr);
				shmctl(shm_info.shmid, IPC_RMID, 0);
			}
		}
		else
			XDestroyImage(ximage[0]);

// data is automatically freed by XDestroyImage
		data[0] = 0;
		last_pixmap_used = 0;
		top_level->unlock_window();
	}
}

int BC_Bitmap::get_default_depth()
{
	if(color_model == BC_TRANSPARENCY)
		return 1;
	else
		return top_level->default_depth;
}

void BC_Bitmap::write_drawable(Drawable &pixmap, 
	GC &gc,
	int dest_x,
	int dest_y,
	int source_x,
	int source_y,
	int dest_w,
	int dest_h)
{
	write_drawable(pixmap, 
		gc,
		source_x, 
		source_y, 
		get_w() - source_x,
		get_h() - source_y,
		dest_x, 
		dest_y, 
		dest_w, 
		dest_h);
}

void BC_Bitmap::write_drawable(Drawable &pixmap, 
		GC &gc,
		int source_x, 
		int source_y, 
		int source_w,
		int source_h,
		int dest_x, 
		int dest_y, 
		int dest_w, 
		int dest_h)
{
	top_level->lock_window("BC_Bitmap::write_drawable");
	if(use_shm)
	{
		if(hardware_scaling())
		{
			if(completion_used)
			{
				unsigned long offs = get_completion_offset();

				if(offs != ULONG_MAX)
					busyflag[offs / data_size] = 0;

				if(drain_action)
				{
					drain_buffer();
					top_level->unlock_window();
					return;
				}

				if(busyflag[current_ringbuffer])
				{
					top_level->unlock_window();
					return;
				}
			}
			else
			{
				parent_window->register_completion(this);
				completion_used = 1;
			}
			busyflag[current_ringbuffer] = 1;
			set_completion_drawable(pixmap);

			XvShmPutImage(top_level->display, 
				xv_portid, 
				pixmap, 
				gc,
				xv_image[current_ringbuffer], 
				source_x + base_left,
				source_y + base_top,
				source_w, 
				source_h, 
				dest_x, 
				dest_y, 
				dest_w, 
				dest_h, 
				True);
// Need to pass these to the XvStopVideo
			last_pixmap = pixmap;
			last_pixmap_used = 1;

		}
		else
		{
			XShmPutImage(top_level->display, 
				pixmap, 
				gc, 
				ximage[current_ringbuffer], 
				source_x + base_left,
				source_y + base_top,
				dest_x, 
				dest_y, 
				dest_w, 
				dest_h, 
				False);
		}
	}
	else
	{
		XPutImage(top_level->display,
			pixmap, 
			gc, 
			ximage[current_ringbuffer], 
			source_x + base_left,
			source_y + base_top,
			dest_x, 
			dest_y, 
			dest_w, 
			dest_h);
	}
	current_ringbuffer++;
	if(current_ringbuffer >= ring_buffers)
		current_ringbuffer = 0;
	top_level->unlock_window();
	XFlush(top_level->display);
}


// the bitmap must be wholly contained in the source during a GetImage
void BC_Bitmap::read_drawable(Drawable &pixmap, int source_x, int source_y)
{
	if(use_shm)
		XShmGetImage(top_level->display, pixmap, ximage[current_ringbuffer], source_x, source_y, 0xffffffff);
	else
		XGetSubImage(top_level->display, pixmap, source_x, source_y, w, h, 0xffffffff, ZPixmap, ximage[current_ringbuffer], 0, 0);
}

// ============================ Decoding VFrames

void BC_Bitmap::read_frame(VFrame *frame, 
	int x1, 
	int y1, 
	int x2, 
	int y2,
	int need_shm,
	int cmodel)
{
	read_frame(frame, 
		0, 0, frame->get_w(), frame->get_h(),
		x1, y1, x2 - x1, y2 - y1,
		need_shm, cmodel);
}

void BC_Bitmap::read_frame(VFrame *frame, 
	int in_x, 
	int in_y, 
	int in_w, 
	int in_h,
	int out_x, 
	int out_y, 
	int out_w, 
	int out_h,
	int need_shm,
	int cmodel)
{
	double aspect;

	if(cmodel < 0)
		cmodel = color_model;
	if(need_shm < 0)
		need_shm = use_shm;

	if(aspect = frame->get_pixel_aspect())
	{
		double hcf = (double)out_h / in_h;
		double wcf = aspect * hcf;

		if(!hardware_scaling())
		{
			base_left = round(in_x * wcf);
			base_top = round(in_y * hcf);
		}
		else
		{
			base_left = in_x;
			base_top = in_y;
		}
		disp_w = round(frame->get_w() * wcf);
		disp_h = round(frame->get_h() * hcf);

		if(disp_w < out_w)
			disp_w = out_w;
		if(disp_h < out_h)
			disp_h = out_h;
	}
	else
	{
		disp_w = out_w;
		disp_h = out_h;
		base_top = base_left = 0;
	}

	if(w < disp_w || h < disp_h ||
		color_model != cmodel ||
		use_shm != need_shm)
	{
		top_level->lock_window("BC_Bitmap::read_frame");
		delete_data();
		initialize(parent_window, disp_w, disp_h, cmodel, need_shm);
		top_level->unlock_window();
	}

	switch(color_model)
	{
// Hardware accelerated bitmap
	case BC_YUV420P:
		if(frame->get_color_model() == color_model)
		{
			memcpy(get_y_plane(), frame->get_y(), w * h);
			memcpy(get_u_plane(), frame->get_u(), w * h / 4);
			memcpy(get_v_plane(), frame->get_v(), w * h / 4);
			break;
		}

	case BC_YUV422P:
		if(frame->get_color_model() == color_model)
		{
			memcpy(get_y_plane(), frame->get_y(), w * h);
			memcpy(get_u_plane(), frame->get_u(), w * h / 2);
			memcpy(get_v_plane(), frame->get_v(), w * h / 2);
			break;
		}

	case BC_YUV422:
		if(frame->get_color_model() == color_model)
		{
			memcpy(get_data(), frame->get_data(), w * h + w * h);
			break;
		}
	case BC_TRANSPARENCY:
		if(frame->get_color_model() == BC_RGBA8888)
		{
			ColorModels::rgba2transparency(out_w, out_h,
				get_data(), frame->get_data(),
				get_bytes_per_line(),
				frame->get_bytes_per_line());
			break;
		}
// Software only
	default:
		ColorModels::transfer_sws(get_data(), frame->get_data(),
			get_y_plane(), get_u_plane(), get_v_plane(),
			frame->get_y(), frame->get_u(), frame->get_v(),
			frame->get_w(), frame->get_h(),
			disp_w, disp_h,
			frame->get_color_model(), color_model,
			frame->get_bytes_per_line(), get_bytes_per_line());
		break;
	}
}

long BC_Bitmap::get_y_offset()
{
	if(xv_image[0])
		return xv_image[current_ringbuffer]->offsets[0];
	else
		return 0;
}

long BC_Bitmap::get_u_offset()
{
	if(xv_image[0] && xv_image[current_ringbuffer]->num_planes > 1)
		return xv_image[current_ringbuffer]->offsets[2];
	else
		return 0;
}

long BC_Bitmap::get_v_offset()
{
	if(xv_image[0] && xv_image[current_ringbuffer]->num_planes > 1)
		return xv_image[current_ringbuffer]->offsets[1];
	else
		return 0;
}

int BC_Bitmap::get_bytes_per_line()
{
	return bytes_per_line;
}

unsigned char* BC_Bitmap::get_data()
{
	return data[current_ringbuffer];
}

unsigned char* BC_Bitmap::get_y_plane()
{
	if(color_model == BC_YUV420P ||
		color_model == BC_YUV422P)
		return data[current_ringbuffer] + xv_image[current_ringbuffer]->offsets[0];
	else
		return 0;
}

unsigned char* BC_Bitmap::get_v_plane()
{
	if(color_model == BC_YUV420P ||
		color_model == BC_YUV422P)
		return data[current_ringbuffer] + xv_image[current_ringbuffer]->offsets[1];
	else
		return 0;
}

unsigned char* BC_Bitmap::get_u_plane()
{
	if(color_model == BC_YUV420P ||
		color_model == BC_YUV422P)
		return data[current_ringbuffer] + xv_image[current_ringbuffer]->offsets[2];
	else
		return 0;
}

int BC_Bitmap::hardware_scaling()
{
	return(xv_image[0] != 0);
}

int BC_Bitmap::get_w()
{
	return w;
}

int BC_Bitmap::get_h()
{
	return h;
}

int BC_Bitmap::get_color_model()
{
	return color_model; 
}

void BC_Bitmap::reset_completion()
{
	for(int i = 0; i < BITMAP_RING; i++)
		completion_offsets[i] = ULONG_MAX;
	completion_read = 0;
	completion_write = 0;
	completion_drawable = 0;
}

unsigned long BC_Bitmap::get_completion_offset()
{
	unsigned long offs;

	if(completion_read >= BITMAP_RING)
		completion_read = 0;
	offs = completion_offsets[completion_read];
	completion_offsets[completion_read] = ULONG_MAX;

	if(offs != ULONG_MAX)
		completion_read++;

	return offs;
}

void BC_Bitmap::set_completion_drawable(Drawable drawable)
{
	for(int i = 0; i < BITMAP_RING; i++)
	{
		if(pixmaps[i] == drawable)
			return;
	}
	pixmaps[completion_drawable] = drawable;

	if(++completion_drawable >= BITMAP_RING)
		completion_drawable = 0;
}

int BC_Bitmap::completion_event(XEvent *event)
{
	XShmCompletionEvent *ev = (XShmCompletionEvent*)event;

	for(int i = 0; i < BITMAP_RING; i++)
	{
		if(pixmaps[i] == ev->drawable)
		{
			if(completion_write >= BITMAP_RING)
				completion_write = 0;
			completion_offsets[completion_write++] = ev->offset;
			return 1;
		}
	}
	return 0;
}

void BC_Bitmap::completion_drain(int action, BC_WindowBase *window)
{
	if(completion_used)
	{
		drain_action = action;
		drain_window = window;
	}
}

void BC_Bitmap::drain_buffer()
{
	for(int i = 0; i < BITMAP_RING; i++)
	{
		if(busyflag[i])
			return;
	}

	switch(drain_action)
	{
	case BITMAP_DRAIN_HIDE:
		XUnmapWindow(top_level->display, drain_window->win);
		drain_window->hidden = 1;
		break;
	case BITMAP_DRAIN_SHOW:
		XMapWindow(top_level->display, drain_window->win);
		drain_window->hidden = 0;
		break;
	}
	drain_action = 0;
	XFlush(top_level->display);
}

void BC_Bitmap::dump(int indent, int minmax)
{
	printf("%*sBC_Bitmap %p dump:\n", indent, "", this);
	indent += 2;
	printf("%*s[%dx%d] '%s' pixbits %d bytes/line %ld\n", indent, "", w, h,
		ColorModels::name(color_model), bits_per_pixel, bytes_per_line);
	printf("%*sbuffers %d/%d shm %d port %d top %p\n", indent, "",
		current_ringbuffer, ring_buffers, use_shm, xv_portid, top_level);
	for(int i = 0; i < ring_buffers; i++)
		printf("%*s%d: data %p xv_image %p ximage %p\n", indent, "",
		i, data[i], xv_image[i], ximage[i]);
	if(minmax & w > 0 && h > 0)
	{
		int min, max;
		unsigned int avg;
		int anum = 0;
		int lnum = 0;
		int fnum = 0;
		int amin[4], amax[4], aavg[4];
		uint64_t lmin[4], lmax[4], lavg[4];
		float fmin[4], fmax[4], favg[4];
		unsigned char *bptr;

		switch(color_model)
		{
		case BC_YUV420P:
			VFrame::calc_minmax8(get_y_plane(), w * h, avg, min, max);
			printf("%*sy: avg %d min %d max %d\n", indent, "", avg, min, max);
			VFrame::calc_minmax8(get_u_plane(), w * h / 4, avg, min, max);
			printf("%*su: avg %d min %d max %d\n", indent, "", avg, min, max);
			VFrame::calc_minmax8(get_v_plane(), w * h / 4,avg, min, max);
			printf("%*sv: avg %d min %d max %d\n", indent, "", avg, min, max);
			break;
		case BC_YUV422P:
			VFrame::calc_minmax8(get_y_plane(), w * h, avg, min, max);
			printf("%*sy: avg %d min %d max %d\n", indent, "", avg, min, max);
			VFrame::calc_minmax8(get_u_plane(), w * h / 2, avg, min, max);
			printf("%*su: avg %d min %d max %d\n", indent, "", avg, min, max);
			VFrame::calc_minmax8(get_v_plane(), w * h / 2, avg, min, max);
			printf("%*sv: avg %d min %d max %d\n", indent, "", avg, min, max);
		case BC_RGB888:
		case BC_YUV888:
		case BC_BGR888:
			VFrame::calc_minmax8n(get_data(), 3, w, h,
				bytes_per_line, aavg, amin, amax);
			anum = 3;
			break;
		case BC_ARGB8888:
		case BC_ABGR8888:
		case BC_RGBA8888:
		case BC_BGR8888:
		case BC_YUVA8888:
			VFrame::calc_minmax8n(get_data(), 4, w, h,
				bytes_per_line, aavg, amin, amax);
			anum = 4;
			break;
		case BC_YUV161616:
		case BC_RGB161616:
			VFrame::calc_minmax16((uint16_t *)get_data(), 3, w, h,
				bytes_per_line, lavg, lmin, lmax);
			lnum = 3;
			break;
		case BC_YUVA16161616:
		case BC_RGBA16161616:
			VFrame::calc_minmax16((uint16_t *)get_data(), 4, w, h,
				bytes_per_line, lavg, lmin, lmax);
			lnum = 4;
			break;
		case BC_RGB_FLOAT:
			VFrame::calc_minmaxfl((float *)get_data(), 3, w, h,
				bytes_per_line, favg, fmin, fmax);
			fnum = 3;
			break;
		case BC_RGBA_FLOAT:
			VFrame::calc_minmaxfl((float *)get_data(), 4, w, h,
				bytes_per_line, favg, fmin, fmax);
			fnum = 4;
			break;
		case BC_TRANSPARENCY:
			// Transparency is 1 bit / pixel
			for(int j = 0; j < h; j++)
			{
				bptr = get_data() + j * get_bytes_per_line();
				for(int i = 0; i < get_bytes_per_line(); i++)
					printf(" %#02x", *bptr++);
				putchar('\n');
			}
			break;
		default:
			VFrame::calc_minmax8(get_data(), h * bytes_per_line,
				avg, min, max);
			printf("%*savg %d min %d max %d\n", indent, "", avg, min, max);
			break;
		}
		for(int i = 0; i < anum; i++)
			printf("%*sl:%d avg %d min %d max %d\n", indent, "", i,
				aavg[i], amin[i], amax[i]);
		for(int i = 0; i < lnum; i++)
			printf("%*sl:%d avg %" PRId64" min %" PRId64 " max %" PRId64"\n",
				indent, "", i, lavg[i], lmin[i], lmax[i]);
		for(int i = 0; i < fnum; i++)
			printf("%*sl:%d avg %.3f min %.3f max %.3f\n", indent, "",
			i, favg[i], fmin[i], fmax[i]);
	}
}
