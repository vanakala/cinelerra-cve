#include "asset.h"
#include "channelpicker.h"
#include "cursors.h"
#include "libdv.h"
#include "edl.h"
#include "edlsession.h"
#include "keys.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "record.h"
#include "recordconfig.h"
#include "recordgui.h"
#include "recordtransport.h"
#include "recordmonitor.h"
#include "mainsession.h"
#include "theme.h"
#include "videodevice.inc"
#include "vframe.h"
#include "videodevice.h"


#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

RecordMonitor::RecordMonitor(MWindow *mwindow, Record *record)
 : Thread(1)
{
	this->mwindow = mwindow;
	this->record = record;
	device = 0;
	thread = 0;
}


RecordMonitor::~RecordMonitor()
{
//printf("RecordMonitor::~RecordMonitor 1\n");
	if(thread)
	{
		thread->stop_playback();
		delete thread;
	}
//printf("RecordMonitor::~RecordMonitor 1\n");
	window->set_done(0);
//printf("RecordMonitor::~RecordMonitor 1\n");
	Thread::join();
//printf("RecordMonitor::~RecordMonitor 1\n");
	if(device) 
	{
		device->close_all();
		delete device;
	}
//printf("RecordMonitor::~RecordMonitor 1\n");
	delete window;
//printf("RecordMonitor::~RecordMonitor 2\n");
}

int RecordMonitor::create_objects()
{
//printf("RecordMonitor::create_objects 1\n");
	window = new RecordMonitorGUI(mwindow,
		record, 
		this);
//printf("RecordMonitor::create_objects 2\n");
	window->create_objects();
//printf("RecordMonitor::create_objects 3\n");

	if(record->default_asset->video_data)
	{
// Configure the output for record monitoring
		VideoOutConfig config(PLAYBACK_LOCALHOST, 0);
		device = new VideoDevice;
//printf("RecordMonitor::create_objects 4\n");



// Override default device for X11 drivers
		if(mwindow->edl->session->playback_config[PLAYBACK_LOCALHOST].values[0]->vconfig->driver ==
			PLAYBACK_X11_XV) config.driver = PLAYBACK_X11_XV;
		config.x11_use_fields = 0;


		device->open_output(&config, 
						record->default_asset->frame_rate, 
						record->default_asset->width, 
						record->default_asset->height,
						window->canvas,
						0);
//printf("RecordMonitor::create_objects 5\n");

		thread = new RecordMonitorThread(mwindow, record, this);
//printf("RecordMonitor::create_objects 6\n");
		thread->start_playback();
	}

	Thread::start();
	return 0;
}


void RecordMonitor::run()
{
	window->run_window();
	close_threads();
}

int RecordMonitor::close_threads()
{
	if(window->channel_picker) window->channel_picker->close_threads();
}

int RecordMonitor::update(VFrame *vframe)
{
	return thread->write_frame(vframe);
}

void RecordMonitor::update_channel(char *text)
{
	if(window->channel_picker)
		window->channel_picker->channel_text->update(text);
}

int RecordMonitor::get_mbuttons_height()
{
	return RECBUTTON_HEIGHT;
}

int RecordMonitor::fix_size(int &w, int &h, int width_given, float aspect_ratio)
{
	w = width_given;
	h = (int)((float)width_given / aspect_ratio);
}

float RecordMonitor::get_scale(int w)
{
	if(mwindow->edl->get_aspect_ratio() > 
		(float)record->frame_w / record->frame_h)
	{
		return (float)w / 
			((float)record->frame_h * 
			mwindow->edl->get_aspect_ratio());
	}
	else
	{
		return (float)w / record->frame_w;
	}
}

int RecordMonitor::get_canvas_height()
{
	return window->get_h() - get_mbuttons_height();
}

int RecordMonitor::get_channel_x()
{
//	return 240;
	return 5;
}

int RecordMonitor::get_channel_y()
{
	return 2;
}










RecordMonitorGUI::RecordMonitorGUI(MWindow *mwindow,
	Record *record, 
	RecordMonitor *thread)
 : BC_Window(PROGRAM_NAME ": Video in", 
 			mwindow->session->rmonitor_x,
			mwindow->session->rmonitor_y,
 			mwindow->session->rmonitor_w, 
 			mwindow->session->rmonitor_h, 
			150, 
			50, 
			1, 
			1,
			1)
{
	this->mwindow = mwindow;
	this->thread = thread;
	this->record = record;
	bitmap = 0;
	channel_picker = 0;
	reverse_interlace = 0;
	meters = 0;
	canvas = 0;
	current_operation = MONITOR_NONE;
}

RecordMonitorGUI::~RecordMonitorGUI()
{
	if(bitmap) delete bitmap;
	if(channel_picker) delete channel_picker;
}

int RecordMonitorGUI::create_objects()
{
//printf("RecordMonitorGUI::create_objects 1\n");
	mwindow->theme->get_rmonitor_sizes(record->default_asset->audio_data, 
		record->default_asset->video_data,
		(mwindow->edl->session->vconfig_in->driver == VIDEO4LINUX ||
			mwindow->edl->session->vconfig_in->driver == CAPTURE_BUZ),
		(mwindow->edl->session->vconfig_in->driver == CAPTURE_BUZ));
	mwindow->theme->draw_rmonitor_bg(this);
//printf("RecordMonitorGUI::create_objects 1\n");

// 	record_transport = new RecordTransport(mwindow, 
// 		record, 
// 		this, 
// 		mwindow->theme->rmonitor_tx_x,
// 		mwindow->theme->rmonitor_tx_y);
// 	record_transport->create_objects();
//printf("RecordMonitorGUI::create_objects 1\n");

	if(record->default_asset->video_data)
	{
		canvas = new RecordMonitorCanvas(mwindow, 
			this,
			record, 
			mwindow->theme->rmonitor_canvas_x, 
			mwindow->theme->rmonitor_canvas_y, 
			mwindow->theme->rmonitor_canvas_w, 
			mwindow->theme->rmonitor_canvas_h);
		canvas->create_objects(0);
//printf("RecordMonitorGUI::create_objects 1\n");

		if(mwindow->edl->session->vconfig_in->driver == VIDEO4LINUX ||
			mwindow->edl->session->vconfig_in->driver == CAPTURE_BUZ)
		{
			channel_picker = new ChannelPicker(mwindow,
				record,
				thread,
				record->current_channeldb(),
				mwindow->theme->rmonitor_channel_x, 
				mwindow->theme->rmonitor_channel_y);
			channel_picker->create_objects();
		}
//printf("RecordMonitorGUI::create_objects 1\n");

		if(mwindow->edl->session->vconfig_in->driver == CAPTURE_BUZ)
		{
			add_subwindow(reverse_interlace = new ReverseInterlace(record,
				mwindow->theme->rmonitor_interlace_x, 
				mwindow->theme->rmonitor_interlace_y));
		}
//printf("RecordMonitorGUI::create_objects 1\n");
		
		add_subwindow(monitor_menu = new BC_PopupMenu(0, 
			0, 
			0, 
			"", 
			0));
		monitor_menu->add_item(new RecordMonitorFullsize(mwindow, 
			this));
//printf("RecordMonitorGUI::create_objects 1\n");
	}
//printf("RecordMonitorGUI::create_objects 1\n");

	if(record->default_asset->audio_data)
	{
		meters = new MeterPanel(mwindow, 
			this,
			mwindow->theme->rmonitor_meter_x,
			mwindow->theme->rmonitor_meter_y,
			mwindow->theme->rmonitor_meter_h,
			record->default_asset->channels,
			1);
		meters->create_objects();
	}
//printf("RecordMonitorGUI::create_objects 2\n");
	return 0;
}

int RecordMonitorGUI::button_press()
{
	if(get_buttonpress() == 2)
	{
		return 0;
	}
	else
// Right button
	if(get_button_down() == 3)
	{
		monitor_menu->activate_menu();
		return 1;
	}
	return 0;
}

int RecordMonitorGUI::button_release()
{
	return 0;
}

int RecordMonitorGUI::get_virtual_center()
{
}

int RecordMonitorGUI::cursor_motion()
{
	return 0;
}

int RecordMonitorGUI::keypress_event()
{
	int result = 0;
	switch(get_keypress())
	{
		case LEFT:
			if(!ctrl_down()) 
			{ 
				record->record_gui->set_translation(--(record->video_x), record->video_y, record->video_zoom);
			}
			else
			{
				record->video_zoom -= 0.1;
				record->record_gui->set_translation(record->video_x, record->video_y, record->video_zoom);
			}
			result = 1;
			break;
		case RIGHT:
			if(!ctrl_down()) 
			{ 
				record->record_gui->set_translation(++(record->video_x), record->video_y, record->video_zoom);
			}
			else
			{
				record->video_zoom += 0.1;
				record->record_gui->set_translation(record->video_x, record->video_y, record->video_zoom);
			}
			result = 1;
			break;
		case UP:
			if(!ctrl_down()) 
			{ 
				record->record_gui->set_translation(record->video_x, --(record->video_y), record->video_zoom);
			}
			else
			{
				record->video_zoom -= 0.1;
				record->record_gui->set_translation(record->video_x, record->video_y, record->video_zoom);
			}
			result = 1;
			break;
		case DOWN:
			if(!ctrl_down()) 
			{ 
				record->record_gui->set_translation(record->video_x, ++(record->video_y), record->video_zoom);
			}
			else
			{
				record->video_zoom += 0.1;
				record->record_gui->set_translation(record->video_x, record->video_y, record->video_zoom);
			}
			result = 1;
			break;
		case 'w':
			close_event();
			break;
		default:
//			result = record_transport->keypress_event();
			break;
	}
	return result;
}


int RecordMonitorGUI::translation_event()
{
//printf("MWindowGUI::translation_event 1 %d %d\n", get_x(), get_y());
	mwindow->session->rmonitor_x = get_x();
	mwindow->session->rmonitor_y = get_y();
	return 0;
}

int RecordMonitorGUI::resize_event(int w, int h)
{
//printf("RecordMonitorGUI::resize_event %d %d\n", w, h);
	mwindow->session->rmonitor_x = get_x();
	mwindow->session->rmonitor_y = get_y();
	mwindow->session->rmonitor_w = w;
	mwindow->session->rmonitor_h = h;

	mwindow->theme->get_rmonitor_sizes(record->default_asset->audio_data, 
		record->default_asset->video_data,
		(mwindow->edl->session->vconfig_in->driver == VIDEO4LINUX ||
			mwindow->edl->session->vconfig_in->driver == CAPTURE_BUZ),
		(mwindow->edl->session->vconfig_in->driver == CAPTURE_BUZ));
	mwindow->theme->draw_rmonitor_bg(this);
	flash();


// 	record_transport->reposition_window(mwindow->theme->rmonitor_tx_x,
// 		mwindow->theme->rmonitor_tx_y);
	if(channel_picker) channel_picker->reposition();
	if(reverse_interlace) reverse_interlace->reposition_window(reverse_interlace->get_x(),
		reverse_interlace->get_y());
	if(record->default_asset->video_data)
	{
		canvas->reposition_window(0,
			mwindow->theme->rmonitor_canvas_x, 
			mwindow->theme->rmonitor_canvas_y, 
			mwindow->theme->rmonitor_canvas_w, 
			mwindow->theme->rmonitor_canvas_h);
	}

	if(record->default_asset->audio_data)
	{
		meters->reposition_window(mwindow->theme->rmonitor_meter_x, 
			mwindow->theme->rmonitor_meter_y, 
			mwindow->theme->rmonitor_meter_h);
	}

	set_title();
	BC_WindowBase::resize_event(w, h);
	flash();
	flush();
	return 1;
}

int RecordMonitorGUI::set_title()
{
return 0;
	char string[1024];
	int scale;

	scale = (int)(thread->get_scale(thread->record->video_window_w) * 100 + 0.5);

	sprintf(string, PROGRAM_NAME ": Video in %d%%", scale);
	BC_Window::set_title(string);
	return 0;
}

int RecordMonitorGUI::close_event()
{
	thread->record->monitor_video = 0;
	thread->record->monitor_audio = 0;
	thread->record->video_window_open = 0;
	if(record->record_gui->monitor_video) record->record_gui->monitor_video->update(0);
	if(record->record_gui->monitor_audio) record->record_gui->monitor_audio->update(0);
	hide_window();
	record->record_gui->flush();
	return 0;
}

int RecordMonitorGUI::create_bitmap()
{
	if(bitmap && 
		(bitmap->get_w() != get_w() || 
			bitmap->get_h() != thread->get_canvas_height()))
	{
		delete bitmap;
		bitmap = 0;
	}

	if(!bitmap)
	{
//		bitmap = canvas->new_bitmap(get_w(), thread->get_canvas_height());
	}
	return 0;
}

ReverseInterlace::ReverseInterlace(Record *record, int x, int y)
 : BC_CheckBox(x, y, record->reverse_interlace, _("Swap fields"))
{
	this->record = record;
}

ReverseInterlace::~ReverseInterlace()
{
}

int ReverseInterlace::handle_event()
{
	record->reverse_interlace = get_value();
	return 0;
}

RecordMonitorCanvas::RecordMonitorCanvas(MWindow *mwindow, 
	RecordMonitorGUI *window, 
	Record *record, 
	int x, 
	int y, 
	int w, 
	int h)
 : Canvas(window, 
 	x, 
	y, 
	w, 
	h, 
	record->default_asset->width,
	record->default_asset->height,
	0,
	0,
	1)
{
	this->window = window;
	this->mwindow = mwindow;
	this->record = record;
//printf("RecordMonitorCanvas::RecordMonitorCanvas 1 %d\n", mwindow->edl->session->vconfig_in->driver);
//printf("RecordMonitorCanvas::RecordMonitorCanvas 2\n");
}

RecordMonitorCanvas::~RecordMonitorCanvas()
{
}

int RecordMonitorCanvas::get_output_w()
{
	return record->default_asset->width;
}

int RecordMonitorCanvas::get_output_h()
{
	return record->default_asset->height;
}


int RecordMonitorCanvas::button_press_event()
{
	if(Canvas::button_press_event()) return 1;
	
	if(mwindow->edl->session->vconfig_in->driver == SCREENCAPTURE)
	{
		window->current_operation = MONITOR_TRANSLATE;
		window->translate_x_origin = record->video_x;
		window->translate_y_origin = record->video_y;
		window->cursor_x_origin = get_cursor_x();
		window->cursor_y_origin = get_cursor_y();
	}

	return 0;
}

void RecordMonitorCanvas::zoom_resize_window(float percentage)
{
	int canvas_w, canvas_h;
	calculate_sizes(mwindow->edl->get_aspect_ratio(), 
		record->default_asset->width, 
		record->default_asset->height, 
		percentage,
		canvas_w,
		canvas_h);
	int new_w, new_h;
	new_w = canvas_w + (window->get_w() - mwindow->theme->rmonitor_canvas_w);
	new_h = canvas_h + (window->get_h() - mwindow->theme->rmonitor_canvas_h);
	window->resize_window(new_w, new_h);
	window->resize_event(new_w, new_h);
}

int RecordMonitorCanvas::button_release_event()
{
	window->current_operation = MONITOR_NONE;
	return 0;
}

int RecordMonitorCanvas::cursor_motion_event()
{
	if(window->current_operation == MONITOR_TRANSLATE)
	{
		record->set_translation(
			get_cursor_x() - window->cursor_x_origin + window->translate_x_origin,
			get_cursor_y() - window->cursor_y_origin + window->translate_y_origin);
	}

	return 0;
}

int RecordMonitorCanvas::cursor_enter_event()
{
	if(mwindow->edl->session->vconfig_in->driver == SCREENCAPTURE)
		set_cursor(MOVE_CURSOR);
	return 0;
}

void RecordMonitorCanvas::reset_translation()
{
	record->set_translation(0, 0);
}

int RecordMonitorCanvas::keypress_event()
{
	int result = 0;
	switch(canvas->get_keypress())
	{
		case LEFT:
			record->set_translation(--record->video_x, record->video_y);
			break;
		case RIGHT:
			record->set_translation(++record->video_x, record->video_y);
			break;
		case UP:
			record->set_translation(record->video_x, --record->video_y);
			break;
		case DOWN:
			record->set_translation(record->video_x, ++record->video_y);
			break;
	}
	return result;
}


RecordMonitorFullsize::RecordMonitorFullsize(MWindow *mwindow, 
	RecordMonitorGUI *window)
 : BC_MenuItem(_("Zoom 100%"))
{
	this->mwindow = mwindow;
	this->window = window;
}
int RecordMonitorFullsize::handle_event()
{
	return 1;
}








// ================================== slippery playback ============================


RecordMonitorThread::RecordMonitorThread(MWindow *mwindow, 
	Record *record, 
	RecordMonitor *record_monitor)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->record_monitor = record_monitor;
	this->record = record;
	reset_parameters();
}


void RecordMonitorThread::reset_parameters()
{
	input_frame = 0;
	output_frame[0] = 0;
	shared_data = 0;
	jpeg_engine = 0;
	dv_engine = 0;
}


RecordMonitorThread::~RecordMonitorThread()
{
	if(input_frame && !shared_data) delete input_frame;
}

void RecordMonitorThread::init_output_format()
{
	long offset;

//printf("RecordMonitorThread::init_output_format 1\n");
	switch(mwindow->edl->session->vconfig_in->driver)
	{
		case SCREENCAPTURE:
			output_colormodel = record->vdevice->get_best_colormodel(record->default_asset);
			break;
	
	
		case CAPTURE_BUZ:
			jpeg_engine = new RecVideoMJPGThread(record, this);
			jpeg_engine->start_rendering();
			output_colormodel = BC_YUV422P;
			break;

		case CAPTURE_FIREWIRE:
			dv_engine = new RecVideoDVThread(record, this);
			dv_engine->start_rendering();
			output_colormodel = BC_YUV422P;
			break;

		case VIDEO4LINUX:
			output_colormodel = record->vdevice->get_best_colormodel(record->default_asset);
//printf("RecordMonitorThread::init_output_format 2 %d\n", output_colormodel);
			break;
	}
}

int RecordMonitorThread::start_playback()
{
	ready = 1;
	done = 0;
	output_lock.lock();
	Thread::start();
	return 0;
}

int RecordMonitorThread::stop_playback()
{
	done = 1;
	output_lock.unlock();
	Thread::join();
//printf("RecordMonitorThread::stop_playback 1\n");

	switch(mwindow->edl->session->vconfig_in->driver)
	{
		case CAPTURE_BUZ:
//printf("RecordMonitorThread::stop_playback 2\n");
			if(jpeg_engine) 
			{
				jpeg_engine->stop_rendering();
//printf("RecordMonitorThread::stop_playback 2\n");
				delete jpeg_engine;
			}
//printf("RecordMonitorThread::stop_playback 3\n");
			break;

		case CAPTURE_FIREWIRE:
			if(dv_engine)
			{
				dv_engine->stop_rendering();
				delete dv_engine;
			}
			break;
	}
//printf("RecordMonitorThread::stop_playback 4\n");

	return 0;
}

int RecordMonitorThread::write_frame(VFrame *new_frame)
{
	if(ready)
	{
		ready = 0;
		shared_data = (new_frame->get_color_model() != BC_COMPRESSED);
// Need to wait until after Record creates the input device before starting monitor
// because the input device deterimes the output format.
// First time
		if(!output_frame[0]) init_output_format();
		if(!shared_data)
		{
			if(!input_frame) input_frame = new VFrame;
			input_frame->allocate_compressed_data(new_frame->get_compressed_size());
			memcpy(input_frame->get_data(), 
				new_frame->get_data(), 
				new_frame->get_compressed_size());
			input_frame->set_compressed_size(new_frame->get_compressed_size());
			input_frame->set_field2_offset(new_frame->get_field2_offset());
		}
		else
		{
			input_lock.lock();
			input_frame = new_frame;
		}
		output_lock.unlock();
	}
	return 0;
}

int RecordMonitorThread::render_jpeg()
{
//printf("RecordMonitorThread::render_jpeg 1\n");
	jpeg_engine->render_frame(input_frame, input_frame->get_compressed_size());
//printf("RecordMonitorThread::render_jpeg 2\n");
	return 0;
}

int RecordMonitorThread::render_dv()
{
	dv_engine->render_frame(input_frame, input_frame->get_compressed_size());
	return 0;
}

void RecordMonitorThread::render_uncompressed()
{
// printf("RecordMonitorThread::render_uncompressed 1 %p %p %p %p %p %p %p\n", 
//  	output_frame[0],
//  	output_frame[0]->get_y(), 
//  	output_frame[0]->get_u(), 
//  	output_frame[0]->get_v(),
// 	input_frame->get_y(),
// 	input_frame->get_u(),
// 	input_frame->get_v());

	output_frame[0]->copy_from(input_frame);

//printf("RecordMonitorThread::render_uncompressed 2\n");
}

void RecordMonitorThread::show_output_frame()
{
	record_monitor->device->write_buffer(output_frame, record->edl);
}

void RecordMonitorThread::unlock_input()
{
	if(shared_data) input_lock.unlock();
}

int RecordMonitorThread::render_frame()
{
	switch(mwindow->edl->session->vconfig_in->driver)
	{
		case CAPTURE_BUZ:
			render_jpeg();
			break;

		case CAPTURE_FIREWIRE:
			render_dv();
			break;

		default:
			render_uncompressed();
			break;
	}

	return 0;
}

void RecordMonitorThread::new_output_frame()
{
	long offset;
//printf("RecordMonitorThread::new_output_frame %d %p %p\n", output_colormodel, record_monitor, record_monitor->device);
	record_monitor->device->new_output_buffers(output_frame, output_colormodel);
//printf("RecordMonitorThread::new_output_frame 2\n");
}

void RecordMonitorThread::run()
{
//printf("RecordMonitorThread::run 1 %d\n", getpid());
	while(!done)
	{
// Wait for next frame
		output_lock.lock();
		if(done)
		{
			unlock_input();
			return;
		}
//printf("RecordMonitorThread::run 1\n");
		new_output_frame();
//printf("RecordMonitorThread::run 2\n");
		render_frame();
//printf("RecordMonitorThread::run 3\n");
		show_output_frame();
//printf("RecordMonitorThread::run 4\n");
		unlock_input();
// Get next frame
		ready = 1;
	}
}



RecVideoMJPGThread::RecVideoMJPGThread(Record *record, RecordMonitorThread *thread)
{
	this->record = record;
	this->thread = thread;
	mjpeg = 0;
}

RecVideoMJPGThread::~RecVideoMJPGThread()
{
}

int RecVideoMJPGThread::start_rendering()
{
	mjpeg = mjpeg_new(record->default_asset->width, 
		record->default_asset->height, 
		2);
//printf("RecVideoMJPGThread::start_rendering 1 %p\n", mjpeg);
	return 0;
}

int RecVideoMJPGThread::stop_rendering()
{
//printf("RecVideoMJPGThread::stop_rendering 1 %p\n", mjpeg);
	if(mjpeg) mjpeg_delete(mjpeg);
//printf("RecVideoMJPGThread::stop_rendering 2\n");
	return 0;
}

int RecVideoMJPGThread::render_frame(VFrame *frame, long size)
{
// printf("RecVideoMJPGThread::render_frame %d %02x%02x %02x%02x\n", frame->get_field2_offset(), 
// 	frame->get_data()[0], 
// 	frame->get_data()[1], 
// 	frame->get_data()[frame->get_field2_offset()], 
// 	frame->get_data()[frame->get_field2_offset() + 1]);
//frame->set_field2_offset(0);
	mjpeg_decompress(mjpeg, 
		frame->get_data(), 
		frame->get_compressed_size(), 
		frame->get_field2_offset(), 
		thread->output_frame[0]->get_rows(), 
		thread->output_frame[0]->get_y(), 
		thread->output_frame[0]->get_u(), 
		thread->output_frame[0]->get_v(),
		thread->output_frame[0]->get_color_model(),
		record->mwindow->preferences->processors);
	return 0;
}




RecVideoDVThread::RecVideoDVThread(Record *record, RecordMonitorThread *thread)
{
	this->record = record;
	this->thread = thread;
	dv = 0;
}

RecVideoDVThread::~RecVideoDVThread()
{
}


int RecVideoDVThread::start_rendering()
{
	((dv_t*)dv) = dv_new();
	return 0;
}

int RecVideoDVThread::stop_rendering()
{
	if(dv) dv_delete(((dv_t*)dv));
	return 0;
}

int RecVideoDVThread::render_frame(VFrame *frame, long size)
{
	unsigned char *yuv_planes[3];
	yuv_planes[0] = thread->output_frame[0]->get_y();
	yuv_planes[1] = thread->output_frame[0]->get_u();
	yuv_planes[2] = thread->output_frame[0]->get_v();
	dv_read_video(((dv_t*)dv), 
		yuv_planes, 
		frame->get_data(), 
		frame->get_compressed_size(),
		thread->output_frame[0]->get_color_model());
	return 0;
}
