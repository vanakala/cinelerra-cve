
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

#include "bcbitmap.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcwindow.h"
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
	initialize(parent_window, 
		w, 
		h, 
		color_model, 
		use_shm ? parent_window->get_resources()->use_shm : 0);
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
	this->use_shm = use_shm ? parent_window->get_resources()->use_shm : 0;
	for(int i = 0; i < BITMAP_RING; i++)
	{
		ximage[i] = 0;
		xv_image[i] = 0;
		data[i] = 0;
	}
	last_pixmap_used = 0;
	last_pixmap = 0;
	current_ringbuffer = 0;
	ring_buffers = 0;
	xv_portid = 0;
	base_left = 0;
	base_top = 0;

	if(w == 0 || h == 0)
		return;

// Set ring buffers based on total memory used.
// The program icon must use multiple buffers but larger bitmaps may not fit
// in memory.
	int pixelsize = ColorModels::calculate_pixelsize(color_model);
	int buffer_size = w * h * pixelsize;

	if(buffer_size < 0x40000)
		ring_buffers = BITMAP_RING;
	else
		ring_buffers = 1;

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
// Create the X Image
			xv_image[0] = XvShmCreateImage(top_level->display, 
						xv_portid, 
						BC_DisplayInfo::cmodel_to_fourcc(color_model),
						0,
						w,
						h,
						&shm_info);
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
			if(xv_image[0])
			{
				if(last_pixmap_used) XvStopVideo(top_level->display, xv_portid, last_pixmap);
				XShmDetach(top_level->display, &shm_info);
				XvUngrabPort(top_level->display, xv_portid, CurrentTime);
				top_level->xvideo_port_id = -1;
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
	int dest_h,
	int dont_wait)
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
		dest_h, 
		dont_wait);
}

void BC_Bitmap::rewind_ring()
{
	current_ringbuffer--;
	if(current_ringbuffer < 0) current_ringbuffer = ring_buffers - 1;
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
		int dest_h, 
		int dont_wait)
{
	top_level->lock_window("BC_Bitmap::write_drawable");
	if(use_shm)
	{
		if(dont_wait) XSync(top_level->display, False);

		if(hardware_scaling())
		{
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
				False);
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

// Force the X server into processing all requests.
// This allows the shared memory to be written to again.
		if(!dont_wait) XSync(top_level->display, False);
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
	if(cmodel < 0)
		cmodel = color_model;
	if(need_shm < 0)
		need_shm = use_shm;

	if(frame->get_pixel_aspect())
	{
		double hcf = (double)out_h / in_h;
		double wcf = frame->get_pixel_aspect() * hcf;

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
		ColorModels::transfer_frame(get_data(),
			frame,
			get_y_plane(),
			get_u_plane(),
			get_v_plane(),
			disp_w, disp_h,
			color_model,
			get_bytes_per_line());
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

void BC_Bitmap::rewind_ringbuffer()
{
	current_ringbuffer--;
	if(current_ringbuffer < 0) current_ringbuffer = ring_buffers - 1;
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

char BC_Bitmap::byte_bitswap(char src)
{
	int i;
	char dst;

	dst = 0;
	if (src & 1) dst |= ((unsigned char)1 << 7);
	src = src >> 1;
	if (src & 1) dst |= ((unsigned char)1 << 6);
	src = src >> 1;
	if (src & 1) dst |= ((unsigned char)1 << 5);
	src = src >> 1;
	if (src & 1) dst |= ((unsigned char)1 << 4);
	src = src >> 1;
	if (src & 1) dst |= ((unsigned char)1 << 3);
	src = src >> 1;
	if (src & 1) dst |= ((unsigned char)1 << 2);
	src = src >> 1;
	if (src & 1) dst |= ((unsigned char)1 << 1);
	src = src >> 1;
	if (src & 1) dst |= ((unsigned char)1 << 0);

	return(dst);
}

int BC_Bitmap::get_color_model()
{
	return color_model; 
}

void BC_Bitmap::dump(int minmax)
{
	printf("BC_Bitmap %p dump\n", this);
	printf("    %d x %d %s pixb %d line %ld\n", w, h, ColorModels::name(color_model),
		bits_per_pixel, bytes_per_line);
	printf("    buffers %d/%d shm %d port %d top %p\n",
		current_ringbuffer, ring_buffers, use_shm, xv_portid, top_level);
	for(int i = 0; i < ring_buffers; i++)
		printf("      %d: data %p xv_image %p ximage %p\n",
		i, data[i], xv_image[i], ximage[i]);
	if(minmax)
	{
		int min, max;
		unsigned int avg;
		int anum = 0;
		int lnum = 0;
		int fnum = 0;
		int amin[4], amax[4], aavg[4];
		uint64_t lmin[4], lmax[4], lavg[4];
		float fmin[4], fmax[4], favg[4];

		switch(color_model)
		{
		case BC_YUV420P:
			VFrame::calc_minmax8(get_y_plane(), w * h, avg, min, max);
			printf("    y: avg %d min %d max %d\n", avg, min, max);
			VFrame::calc_minmax8(get_u_plane(), w * h / 4, avg, min, max);
			printf("    u: avg %d min %d max %d\n", avg, min, max);
			VFrame::calc_minmax8(get_v_plane(), w * h / 4,avg, min, max);
			printf("    v: avg %d min %d max %d\n", avg, min, max);
			break;
		case BC_YUV422P:
			VFrame::calc_minmax8(get_y_plane(), w * h, avg, min, max);
			printf("    y: avg %d min %d max %d\n", avg, min, max);
			VFrame::calc_minmax8(get_u_plane(), w * h / 2, avg, min, max);
			printf("    u: avg %d min %d max %d\n", avg, min, max);
			VFrame::calc_minmax8(get_v_plane(), w * h / 2, avg, min, max);
			printf("    v: avg %d min %d max %d\n", avg, min, max);
		case BC_RGB888:
		case BC_YUV888:
		case BC_BGR888:
			VFrame::calc_minmax8n(get_data(), w * h, 3, aavg, amin, amax);
			anum = 3;
			break;
		case BC_ARGB8888:
		case BC_ABGR8888:
		case BC_RGBA8888:
		case BC_BGR8888:
		case BC_YUVA8888:
			VFrame::calc_minmax8n(get_data(), w * h, 4, aavg, amin, amax);
			anum = 4;
			break;
		case BC_YUV161616:
		case BC_RGB161616:
			VFrame::calc_minmax16((uint16_t *)get_data(), w * h, 3, lavg, lmin, lmax);
			lnum = 3;
			break;
		case BC_YUVA16161616:
		case BC_RGBA16161616:
			VFrame::calc_minmax16((uint16_t *)get_data(), w * h, 4, lavg, lmin, lmax);
			lnum = 4;
			break;
		case BC_RGB_FLOAT:
			VFrame::calc_minmaxfl((float *)get_data(), w * h, 3, favg, fmin, fmax);
			fnum = 3;
			break;
		case BC_RGBA_FLOAT:
			VFrame::calc_minmaxfl((float *)get_data(), w * h, 4, favg, fmin, fmax);
			fnum = 4;
			break;
		default:
			VFrame::calc_minmax8(get_data(), h * bytes_per_line,
				avg, min, max);
			printf("    avg %d min %d max %d\n", avg, min, max);
			break;
		}
		for(int i = 0; i < anum; i++)
			printf("    l:%d avg %d min %d max %d\n", i, aavg[i], amin[i], amax[i]);
		for(int i = 0; i < lnum; i++)
			printf("    l:%d avg %" PRId64" min %" PRId64 " max %" PRId64"\n", i, lavg[i], lmin[i], lmax[i]);
		for(int i = 0; i < fnum; i++)
			printf("    l:%d avg %.3f min %.3f max %.3f\n", i, favg[i], fmin[i], fmax[i]);
	}
}
