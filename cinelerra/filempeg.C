#include "assets.h"
#include "bitspopup.h"
#include "byteorder.h"
#include "clip.h"
#include "file.h"
#include "edit.h"
#include "filempeg.h"
#include "guicast.h"
#include "mwindow.inc"
#include "pluginserver.h"
#include "threadfork.h"
#include "vframe.h"
#include "videodevice.inc"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MPEG_VIDEO_ENCODER "mpeg2enc.plugin"
#define MPEG2_AUDIO_ENCODER "toolame.plugin"
#define MPEG3_AUDIO_ENCODER "lame.plugin"

FileMPEG::FileMPEG(Asset *asset, File *file, ArrayList<PluginServer*> *plugindb)
 : FileBase(asset, file)
{
	reset_parameters();
// May also be VMPEG or AMPEG if write status.
	if(asset->format == FILE_UNKNOWN) asset->format = FILE_MPEG;
	asset->byte_order = 0;
	this->plugindb = plugindb;
}

FileMPEG::~FileMPEG()
{
	close_file();
}

void FileMPEG::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int audio_options,
	int video_options)
{
	if(audio_options && asset->format == FILE_AMPEG)
	{
		MPEGConfigAudio *window = new MPEGConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
	else
	if(video_options && asset->format == FILE_VMPEG)
	{
		MPEGConfigVideo *window = new MPEGConfigVideo(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}

int FileMPEG::check_sig(Asset *asset)
{
	return mpeg3_check_sig(asset->path);
}

int FileMPEG::reset_parameters_derived()
{
	fd = 0;
	video_out = 0;
	audio_out = 0;
	prev_track = 0;
	temp_frame = 0;
	audio_temp = 0;
	audio_allocation = 0;
//	last_sample = -1;
}


// Just create the Quicktime objects since this routine is also called
// for reopening.
int FileMPEG::open_file(int rd, int wr)
{
	int result = 0;
	this->rd = rd;
	this->wr = wr;
//printf("FileMPEG::open_file: %d %d %d\n", rd, wr, asset->format);

	if(rd)
	{
		if(!(fd = mpeg3_open(asset->path)))
		{
			printf("FileMPEG::open_file %s\n", asset->path);
			result = 1;
		}
		else
		{
			mpeg3_set_cpus(fd, file->cpus);
			mpeg3_set_mmx(fd, 0);

			asset->audio_data = mpeg3_has_audio(fd);
			if(asset->audio_data)
			{
				asset->channels = 0;
				for(int i = 0; i < mpeg3_total_astreams(fd); i++)
				{
					asset->channels += mpeg3_audio_channels(fd, i);
				}
				if(!asset->sample_rate)
					asset->sample_rate = mpeg3_sample_rate(fd, 0);
				asset->audio_length = mpeg3_audio_samples(fd, 0); 
	//printf("FileMPEG::open_file 1 %d\n", asset->audio_length);
			}

			asset->video_data = mpeg3_has_video(fd);
			if(asset->video_data)
			{
				asset->layers = mpeg3_total_vstreams(fd);
				asset->width = mpeg3_video_width(fd, 0);
				asset->height = mpeg3_video_height(fd, 0);
				asset->video_length = mpeg3_video_frames(fd, 0);
				asset->vmpeg_cmodel = (mpeg3_colormodel(fd, 0) == MPEG3_YUV422P) ? 1 : 0;
				if(!asset->frame_rate)
					asset->frame_rate = mpeg3_frame_rate(fd, 0);
//printf("FileMPEG::open_file 2 %d\n", asset->video_length);
			}
		}
	}
	
//printf("FileMPEG::open_file 2 %d\n", asset->video_length);
	
	
	if(wr && asset->format == FILE_VMPEG)
	{
		char command_line[BCTEXTLEN];
		char bitrate_string[BCTEXTLEN];
		char quant_string[BCTEXTLEN];
		char iframe_string[BCTEXTLEN];
		char encoder_string[BCTEXTLEN];

		encoder_string[0] = 0;
		sprintf(bitrate_string, " -b %d", asset->vmpeg_bitrate);
		sprintf(quant_string, " -q %d", asset->vmpeg_quantization);
		sprintf(iframe_string, " -n %d", asset->vmpeg_iframe_distance);

// Look up executable plugin
		for(int i = 0; i < plugindb->total; i++)
		{
			if(strstr(plugindb->values[i]->title, MPEG_VIDEO_ENCODER))
				strcpy(encoder_string, plugindb->values[i]->title);
		}

		if(!strlen(encoder_string))
		{
			printf("FileMPEG::open_file: couldn't find %s plugin.\n", MPEG_VIDEO_ENCODER);
			result = 1;
		}
		
		if(!result)
		{
			sprintf(command_line, "%s%s%s%s%s%s%s%s%s%s - %s",
				encoder_string, 
				asset->vmpeg_derivative == 1 ? " -1" : "",
				asset->vmpeg_cmodel == 1 ? " -422" : "",
				asset->vmpeg_fix_bitrate ? bitrate_string : "",
				!asset->vmpeg_fix_bitrate ? quant_string : "",
				iframe_string,
				asset->vmpeg_progressive ? " -p" : "",
				asset->vmpeg_denoise ? " -d" : "",
				file->cpus <= 1 ? " -u" : "",
				asset->vmpeg_seq_codes ? " -g" : "",
				asset->path);

			video_out = new ThreadFork;
			video_out->start_command(command_line, 1);
		}

		if(!video_out) 
		{
			result = 1;
		}
		else
		{
//printf("FileMPEG::open_file 4\n");
			fprintf(video_out->get_stdin(), "%d\n", asset->width);
			fprintf(video_out->get_stdin(), "%d\n", asset->height);
			fprintf(video_out->get_stdin(), "%.16e\n", asset->frame_rate);
//printf("FileMPEG::open_file 4\n");
//sleep(5);
		}
	}
	
	
	if(wr && asset->format == FILE_AMPEG)
	{
		char command_line[BCTEXTLEN];
		char encoder_string[BCTEXTLEN];
		char argument_string[BCTEXTLEN];

//printf("FileMPEG::open_file 1 %d\n", asset->ampeg_derivative);
		encoder_string[0] = 0;

		if(asset->ampeg_derivative == 2)
		{
// Look up executable plugin
			for(int i = 0; i < plugindb->total; i++)
			{
				if(strstr(plugindb->values[i]->title, MPEG2_AUDIO_ENCODER))
					strcpy(encoder_string, plugindb->values[i]->title);
			}

			sprintf(argument_string, " %s -s %f -b %d - %s", 
				(asset->channels >= 2) ? "-m j" : "-m m", 
				(float)asset->sample_rate / 1000,
				asset->ampeg_bitrate,
				asset->path);
		}
		else
		if(asset->ampeg_derivative == 3)
		{
// Look up executable plugin
			for(int i = 0; i < plugindb->total; i++)
			{
// toolame is inclusive of lame
				if(strstr(plugindb->values[i]->title, MPEG3_AUDIO_ENCODER) &&
					!strstr(plugindb->values[i]->title, MPEG2_AUDIO_ENCODER))
					strcpy(encoder_string, plugindb->values[i]->title);
			}

			sprintf(argument_string, " -h %s -s %f -b %d - %s",
				(asset->channels >= 2) ? "-m j" : "-m m",
				(float)asset->sample_rate / 1000,
				asset->ampeg_bitrate,
				asset->path);
		}
		else
		{
			printf("FileMPEG::open_file: ampeg_derivative=%d\n", asset->ampeg_derivative);
			result = 1;
		}

// Look up executable plugin
		if(!result)
		{
			if(!strlen(encoder_string))
			{
				printf("FileMPEG::open_file: couldn't find %s plugin.\n", encoder_string);
				result = 1;
			}
		}

		if(!result)
		{
			sprintf(command_line, "%s%s", encoder_string, argument_string);
//printf("FileMPEG::open_file: %s\n", command_line);
			audio_out = new ThreadFork;
			audio_out->start_command(command_line, 1);
		}
	}

	return result;
}

int FileMPEG::close_file()
{
//printf("FileMPEG::close_file_derived 1\n");
	if(fd)
	{
		mpeg3_close(fd);
	}
	
	if(video_out)
	{
// End of sequence signal
		unsigned char data[5];
		data[0] = data[1] = data[2] = data[3] = 0xff;
		fwrite(data, 4, 1, video_out->get_stdin());
		fflush(video_out->get_stdin());
		
		delete video_out;
		video_out = 0;
	}

	if(audio_out)
	{
		fflush(audio_out->get_stdin());
		delete audio_out;
		audio_out = 0;
	}

//printf("FileMPEG::close_file_derived 1\n");
	if(temp_frame) delete temp_frame;

	if(audio_temp) delete audio_temp;

//printf("FileMPEG::close_file_derived 1\n");
	reset_parameters();
	FileBase::close_file();
//printf("FileMPEG::close_file_derived 2\n");
	return 0;
}

int FileMPEG::get_best_colormodel(Asset *asset, int driver)
{
	switch(driver)
	{
		case PLAYBACK_X11:
			return BC_RGB888;
			if(asset->vmpeg_cmodel == 0) return BC_YUV420P;
			if(asset->vmpeg_cmodel == 1) return BC_YUV422P;
			break;
		case PLAYBACK_X11_XV:
			if(asset->vmpeg_cmodel == 0) return BC_YUV420P;
			if(asset->vmpeg_cmodel == 1) return BC_YUV422P;
			break;
		case PLAYBACK_LML:
		case PLAYBACK_BUZ:
			return BC_YUV422P;
			break;
		case PLAYBACK_FIREWIRE:
			return BC_YUV422P;
			break;
		case VIDEO4LINUX:
		case VIDEO4LINUX2:
			if(asset->vmpeg_cmodel == 0) return BC_YUV420P;
			if(asset->vmpeg_cmodel == 1) return BC_YUV422P;
			break;
		case CAPTURE_BUZ:
		case CAPTURE_LML:
			return BC_YUV422;
			break;
		case CAPTURE_FIREWIRE:
			return BC_YUV422P;
			break;
	}
}

int FileMPEG::colormodel_supported(int colormodel)
{
	return colormodel;
}


int FileMPEG::can_copy_from(Edit *edit, long position)
{
	if(!fd) return 0;
	return 0;
}

int FileMPEG::set_audio_position(long sample)
{
#if 0
	if(!fd) return 1;
	
	int channel, stream;
	to_streamchannel(file->current_channel, stream, channel);

//printf("FileMPEG::set_audio_position %d %d %d\n", sample, mpeg3_get_sample(fd, stream), last_sample);
	if(sample != mpeg3_get_sample(fd, stream) &&
		sample != last_sample)
	{
		if(sample >= 0 && sample < asset->audio_length)
		{
//printf("FileMPEG::set_audio_position seeking stream %d\n", sample);
			return mpeg3_set_sample(fd, sample, stream);
		}
		else
			return 1;
	}
#endif
	return 0;
}

int FileMPEG::set_video_position(long x)
{
	if(!fd) return 1;
	if(x >= 0 && x < asset->video_length)
		return mpeg3_set_frame(fd, x, file->current_layer);
	else
		return 1;
}


int FileMPEG::write_samples(double **buffer, long len)
{
	int result = 0;

	if(audio_out)
	{
// Convert to int16
		int channels = MIN(asset->channels, 2);
		long audio_size = len * channels * 2;
		if(audio_allocation < audio_size)
		{
			if(audio_temp) delete [] audio_temp;
			audio_temp = new unsigned char[audio_size];
			audio_allocation = audio_size;
		}

		if(asset->ampeg_derivative == 2)
		{
			for(int i = 0; i < channels; i++)
			{
				int16_t *output = ((int16_t*)audio_temp) + i;
				double *input = buffer[i];
				for(int j = 0; j < len; j++)
				{
					int sample = (int)(*input * 0x7fff);
					*output = (int16_t)(CLIP(sample, -0x8000, 0x7fff));
					output += channels;
					input++;
				}
			}
		}
		else
		{
			for(int i = 0; i < channels; i++)
			{
				unsigned char *output = audio_temp + i * 2;
				double *input = buffer[i];
				for(int j = 0; j < len; j++)
				{
					int sample = (int)(*input * 0x7fff);
					CLAMP(sample, -0x8000, 0x7fff);
					output[0] = (sample & 0xff00) >> 8;
					output[1] = sample & 0xff;
					output += channels * 2;
					input++;
				}
			}
		}

		result = !fwrite(audio_temp, 
			audio_size, 
			1, 
			audio_out->get_stdin());
	}

	return result;
}

int FileMPEG::write_frames(VFrame ***frames, int len)
{
	int result = 0;

	if(video_out)
	{
		int temp_w = (int)((asset->width + 15) / 16) * 16;
		int temp_h;
		int output_cmodel = 
			(asset->vmpeg_cmodel == 0) ? BC_YUV420P : BC_YUV422P;
		
		
// Height depends on progressiveness
		if(asset->vmpeg_progressive || asset->vmpeg_derivative == 1)
			temp_h = (int)((asset->height + 15) / 16) * 16;
		else
			temp_h = (int)((asset->height + 31) / 32) * 32;

//printf("FileMPEG::write_frames 1 %d %d\n", temp_w, temp_h);
		
// Only 1 layer is supported in MPEG output
		for(int i = 0; i < 1; i++)
		{
			for(int j = 0; j < len; j++)
			{
				VFrame *frame = frames[i][j];
				
				
				
				if(frame->get_w() == temp_w &&
					frame->get_h() == temp_h &&
					frame->get_color_model() == output_cmodel)
				{
					result = !fwrite(frame->get_data(), 
						frame->get_data_size(), 
						1, 
						video_out->get_stdin());
				}
				else
				{
//printf("FileMPEG::write_frames 2\n");
					if(temp_frame&&
						(temp_frame->get_w() != temp_w ||
						temp_frame->get_h() != temp_h ||
						temp_frame->get_color_model() || output_cmodel))
					{
						delete temp_frame;
						temp_frame = 0;
					}
//printf("FileMPEG::write_frames 3\n");


					if(!temp_frame)
					{
						temp_frame = new VFrame(0, 
							temp_w, 
							temp_h, 
							output_cmodel);
					}

					cmodel_transfer(temp_frame->get_rows(), 
						frame->get_rows(),
						temp_frame->get_y(),
						temp_frame->get_u(),
						temp_frame->get_v(),
						frame->get_y(),
						frame->get_u(),
						frame->get_v(),
						0,
						0,
						asset->width,
						asset->height,
						0,
						0,
						asset->width,
						asset->height,
						frame->get_color_model(), 
						temp_frame->get_color_model(),
						0, 
						frame->get_w(),
						temp_w);

//printf("FileMPEG::write_frames %d %d\n", frame->get_color_model(), temp_frame->get_color_model());

// printf("FileMPEG::write_frames %p %d %d %d %d\n", temp_frame->get_data(), 
// 						temp_frame->get_data_size(),
// 						temp_frame->get_color_model(),
// 						temp_frame->get_w(),
// 						temp_frame->get_h());

// Not end of sequence signal
					unsigned char data[5];
					data[0] = data[1] = data[2] = data[3] = 0x0;
					fwrite(data, 4, 1, video_out->get_stdin());
					result = !fwrite(temp_frame->get_data(), 
						temp_frame->get_data_size(), 
						1, 
						video_out->get_stdin());
//printf("FileMPEG::write_frames 5\n");
				}
			}
		}
	}



	return result;
}

int FileMPEG::read_frame(VFrame *frame)
{
	if(!fd) return 1;
	int result = 0;
	int src_cmodel;
	
	if(mpeg3_colormodel(fd, 0) == MPEG3_YUV420P)
		src_cmodel = BC_YUV420P;
	else
	if(mpeg3_colormodel(fd, 0) == MPEG3_YUV422P)
		src_cmodel = BC_YUV422P;

//printf("FileMPEG::read_frame 1 %p %d\n", frame, frame->get_color_model());
	switch(frame->get_color_model())
	{
		case MPEG3_RGB565:
		case MPEG3_BGR888:
		case MPEG3_BGRA8888:
		case MPEG3_RGB888:
		case MPEG3_RGBA8888:
		case MPEG3_RGBA16161616:
			mpeg3_read_frame(fd, 
					frame->get_rows(), /* Array of pointers to the start of each output row */
					0,                    /* Location in input frame to take picture */
					0, 
					asset->width, 
					asset->height, 
					asset->width,                   /* Dimensions of output_rows */
					asset->height, 
					frame->get_color_model(),             /* One of the color model #defines */
					file->current_layer);
			break;

// Use Temp
		default:
// Read these directly
			if(frame->get_color_model() == src_cmodel)
			{
				mpeg3_read_yuvframe(fd,
					(char*)frame->get_y(),
					(char*)frame->get_u(),
					(char*)frame->get_v(),
					0,
					0,
					asset->width,
					asset->height,
					file->current_layer);
			}
			else
// Process through temp frame
			{
				char *y, *u, *v;
				mpeg3_read_yuvframe_ptr(fd,
					&y,
					&u,
					&v,
					file->current_layer);
				cmodel_transfer(frame->get_rows(), 
					0,
					frame->get_y(),
					frame->get_u(),
					frame->get_v(),
					(unsigned char*)y,
					(unsigned char*)u,
					(unsigned char*)v,
					0,
					0,
					asset->width,
					asset->height,
					0,
					0,
					asset->width,
					asset->height,
					src_cmodel, 
					frame->get_color_model(),
					0, 
					asset->width,
					frame->get_w());
			}
//printf("FileMPEG::read_frame 2\n");
			break;
	}
//for(int i = 0; i < frame->get_w() * 3 * 20; i++) 
//	((u_int16_t*)frame->get_rows()[0])[i] = 0xffff;
//printf("FileMPEG::read_frame 3\n");
	return result;
}

void FileMPEG::to_streamchannel(int channel, int &stream_out, int &channel_out)
{
	for(stream_out = 0, channel_out = file->current_channel; 
		stream_out < mpeg3_total_astreams(fd) && 
			channel_out >= mpeg3_audio_channels(fd, stream_out);
		channel_out -= mpeg3_audio_channels(fd, stream_out), stream_out++)
	;
}

int FileMPEG::read_samples(double *buffer, long len)
{
	if(!fd) return 0;

// This is directed to a FileMPEGBuffer
	float *temp_float = new float[len];
// Translate pure channel to a stream and a channel in the mpeg stream
	int stream, channel;
	to_streamchannel(file->current_channel, stream, channel);
	
	
	
//printf("FileMPEG::read_samples 1 current_sample=%ld len=%ld channel=%d\n", file->current_sample, len, channel);

	mpeg3_set_sample(fd, 
		file->current_sample,
		stream);
	mpeg3_read_audio(fd, 
		temp_float,      /* Pointer to pre-allocated buffer of floats */
		0,      /* Pointer to pre-allocated buffer of int16's */
		channel,          /* Channel to decode */
		len,         /* Number of samples to decode */
		stream);          /* Stream containing the channel */


//	last_sample = file->current_sample;
	for(int i = 0; i < len; i++) buffer[i] = temp_float[i];

	delete [] temp_float;
	return 0;
}

char* FileMPEG::strtocompression(char *string)
{
	return "";
}

char* FileMPEG::compressiontostr(char *string)
{
	return "";
}










MPEGConfigAudio::MPEGConfigAudio(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Audio Compression",
 	parent_window->get_abs_cursor_x(),
 	parent_window->get_abs_cursor_y(),
	310,
	120,
	-1,
	-1,
	0,
	0,
	1)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

MPEGConfigAudio::~MPEGConfigAudio()
{
}

int MPEGConfigAudio::create_objects()
{
	int x = 10, y = 10;
	int x1 = 150;
	MPEGLayer *layer;

	add_tool(new BC_Title(x, y, "Layer:"));
	add_tool(layer = new MPEGLayer(x1, y, this));
	layer->create_objects();

	y += 30;
	add_tool(new BC_Title(x, y, "Kbits per second:"));
	add_tool(bitrate = new MPEGABitrate(x1, y, this));
	bitrate->create_objects();
	
	
	add_subwindow(new BC_OKButton(this));
	show_window();
	flush();
	return 0;
}

int MPEGConfigAudio::close_event()
{
	set_done(0);
	return 1;
}







MPEGLayer::MPEGLayer(int x, int y, MPEGConfigAudio *gui)
 : BC_PopupMenu(x, y, 150, layer_to_string(gui->asset->ampeg_derivative))
{
	this->gui = gui;
}

void MPEGLayer::create_objects()
{
	add_item(new BC_MenuItem(layer_to_string(2)));
	add_item(new BC_MenuItem(layer_to_string(3)));
}

int MPEGLayer::handle_event()
{
	gui->asset->ampeg_derivative = string_to_layer(get_text());
	gui->bitrate->set_layer(gui->asset->ampeg_derivative);
	return 1;
};

int MPEGLayer::string_to_layer(char *string)
{
	if(!strcasecmp(layer_to_string(2), string))
		return 2;
	if(!strcasecmp(layer_to_string(3), string))
		return 3;

	return 2;
}

char* MPEGLayer::layer_to_string(int layer)
{
	switch(layer)
	{
		case 2:
			return "II";
			break;
		
		case 3:
			return "III";
			break;
			
		default:
			return "II";
			break;
	}
}







MPEGABitrate::MPEGABitrate(int x, int y, MPEGConfigAudio *gui)
 : BC_PopupMenu(x, 
 	y, 
	150, 
 	bitrate_to_string(gui->string, gui->asset->ampeg_bitrate))
{
	this->gui = gui;
}

void MPEGABitrate::create_objects()
{
	set_layer(gui->asset->ampeg_derivative);
}

void MPEGABitrate::set_layer(int layer)
{
	while(total_items())
	{
		remove_item(0);
	}

	if(layer == 2)
	{
		add_item(new BC_MenuItem("160"));
		add_item(new BC_MenuItem("192"));
		add_item(new BC_MenuItem("224"));
		add_item(new BC_MenuItem("256"));
		add_item(new BC_MenuItem("320"));
		add_item(new BC_MenuItem("384"));
	}
	else
	{
		add_item(new BC_MenuItem("8"));
		add_item(new BC_MenuItem("16"));
		add_item(new BC_MenuItem("24"));
		add_item(new BC_MenuItem("32"));
		add_item(new BC_MenuItem("40"));
		add_item(new BC_MenuItem("48"));
		add_item(new BC_MenuItem("56"));
		add_item(new BC_MenuItem("64"));
		add_item(new BC_MenuItem("80"));
		add_item(new BC_MenuItem("96"));
		add_item(new BC_MenuItem("112"));
		add_item(new BC_MenuItem("128"));
		add_item(new BC_MenuItem("144"));
		add_item(new BC_MenuItem("160"));
		add_item(new BC_MenuItem("192"));
		add_item(new BC_MenuItem("224"));
		add_item(new BC_MenuItem("256"));
		add_item(new BC_MenuItem("320"));
	}
}

int MPEGABitrate::handle_event()
{
	gui->asset->ampeg_bitrate = string_to_bitrate(get_text());
	return 1;
};

int MPEGABitrate::string_to_bitrate(char *string)
{
	return atol(string);
}


char* MPEGABitrate::bitrate_to_string(char *string, int bitrate)
{
	sprintf(string, "%d", bitrate);
	return string;
}









MPEGConfigVideo::MPEGConfigVideo(BC_WindowBase *parent_window, 
	Asset *asset)
 : BC_Window(PROGRAM_NAME ": Video Compression",
 	parent_window->get_abs_cursor_x(),
 	parent_window->get_abs_cursor_y(),
	500,
	300,
	-1,
	-1,
	0,
	0,
	1)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

MPEGConfigVideo::~MPEGConfigVideo()
{
}

int MPEGConfigVideo::create_objects()
{
	int x = 10, y = 10;
	int x1 = x + 150;
	int x2 = x + 300;
	MPEGDerivative *derivative;
	MPEGColorModel *cmodel;

	add_subwindow(new BC_Title(x, y + 5, "Derivative:"));
	add_subwindow(derivative = new MPEGDerivative(x1, y, this));
	derivative->create_objects();
	y += 30;

	add_subwindow(new BC_Title(x, y + 5, "Bitrate:"));
	add_subwindow(new MPEGBitrate(x1, y, this));
	add_subwindow(fixed_bitrate = new MPEGFixedBitrate(x2, y, this));
	y += 30;

	add_subwindow(new BC_Title(x, y, "Quantization:"));
	MPEGQuant *quant = new MPEGQuant(x1, y, this);
	quant->create_objects();
	add_subwindow(fixed_quant = new MPEGFixedQuant(x2, y, this));
	y += 30;

	add_subwindow(new BC_Title(x, y, "I frame distance:"));
	MPEGIFrameDistance *iframe_distance = 
		new MPEGIFrameDistance(x1, y, this);
	iframe_distance->create_objects();
	y += 30;

	add_subwindow(new BC_Title(x, y, "Color model:"));
	add_subwindow(cmodel = new MPEGColorModel(x1, y, this));
	cmodel->create_objects();
	y += 30;
	
//	add_subwindow(new BC_Title(x, y, "P frame distance:"));
//	y += 30;


	add_subwindow(new BC_CheckBox(x, y, &asset->vmpeg_progressive, "Progressive frames"));
	y += 30;
	add_subwindow(new BC_CheckBox(x, y, &asset->vmpeg_denoise, "Denoise"));
	y += 30;
	add_subwindow(new BC_CheckBox(x, y, &asset->vmpeg_seq_codes, "Sequence start codes in every GOP"));

	add_subwindow(new BC_OKButton(this));
	show_window();
	flush();
	return 0;
}

int MPEGConfigVideo::close_event()
{
	set_done(0);
	return 1;
}



MPEGDerivative::MPEGDerivative(int x, int y, MPEGConfigVideo *gui)
 : BC_PopupMenu(x, y, 150, derivative_to_string(gui->asset->vmpeg_derivative))
{
	this->gui = gui;
}

void MPEGDerivative::create_objects()
{
	add_item(new BC_MenuItem(derivative_to_string(1)));
	add_item(new BC_MenuItem(derivative_to_string(2)));
}

int MPEGDerivative::handle_event()
{
	gui->asset->vmpeg_derivative = string_to_derivative(get_text());
	return 1;
};

int MPEGDerivative::string_to_derivative(char *string)
{
	if(!strcasecmp(derivative_to_string(1), string))
		return 1;
	if(!strcasecmp(derivative_to_string(2), string))
		return 2;

	return 1;
}

char* MPEGDerivative::derivative_to_string(int derivative)
{
	switch(derivative)
	{
		case 1:
			return "MPEG-1";
			break;
		
		case 2:
			return "MPEG-2";
			break;
			
		default:
			return "MPEG-1";
			break;
	}
}




MPEGBitrate::MPEGBitrate(int x, int y, MPEGConfigVideo *gui)
 : BC_TextBox(x, y, 100, 1, gui->asset->vmpeg_bitrate)
{
	this->gui = gui;
}


int MPEGBitrate::handle_event()
{
	gui->asset->vmpeg_bitrate = atol(get_text());
	return 1;
};





MPEGQuant::MPEGQuant(int x, int y, MPEGConfigVideo *gui)
 : BC_TumbleTextBox(gui, 
 	(long)gui->asset->vmpeg_quantization, 
	(long)1,
	(long)100,
	x, 
	y,
	100)
{
	this->gui = gui;
}

int MPEGQuant::handle_event()
{
	gui->asset->vmpeg_quantization = atol(get_text());
	return 1;
};

MPEGFixedBitrate::MPEGFixedBitrate(int x, int y, MPEGConfigVideo *gui)
 : BC_Radial(x, y, gui->asset->vmpeg_fix_bitrate, "Fixed bitrate")
{
	this->gui = gui;
}

int MPEGFixedBitrate::handle_event()
{
	update(1);
	gui->asset->vmpeg_fix_bitrate = 1;
	gui->fixed_quant->update(0);
	return 1;
};

MPEGFixedQuant::MPEGFixedQuant(int x, int y, MPEGConfigVideo *gui)
 : BC_Radial(x, y, !gui->asset->vmpeg_fix_bitrate, "Fixed quantization")
{
	this->gui = gui;
}

int MPEGFixedQuant::handle_event()
{
	update(1);
	gui->asset->vmpeg_fix_bitrate = 0;
	gui->fixed_bitrate->update(0);
	return 1;
};

MPEGIFrameDistance::MPEGIFrameDistance(int x, int y, MPEGConfigVideo *gui)
 : BC_TumbleTextBox(gui, 
 	(long)gui->asset->vmpeg_iframe_distance, 
	(long)1,
	(long)100,
	x, 
	y,
	50)
{
	this->gui = gui;
}

int MPEGIFrameDistance::handle_event()
{
	gui->asset->vmpeg_iframe_distance = atol(get_text());
	return 1;
}


MPEGColorModel::MPEGColorModel(int x, int y, MPEGConfigVideo *gui)
 : BC_PopupMenu(x, y, 150, cmodel_to_string(gui->asset->vmpeg_cmodel))
{
	this->gui = gui;
}

void MPEGColorModel::create_objects()
{
	add_item(new BC_MenuItem(cmodel_to_string(0)));
	add_item(new BC_MenuItem(cmodel_to_string(1)));
}

int MPEGColorModel::handle_event()
{
	gui->asset->vmpeg_cmodel = string_to_cmodel(get_text());
	return 1;
};

int MPEGColorModel::string_to_cmodel(char *string)
{
	if(!strcasecmp(cmodel_to_string(0), string))
		return 0;
	if(!strcasecmp(cmodel_to_string(1), string))
		return 1;

	return 1;
}

char* MPEGColorModel::cmodel_to_string(int cmodel)
{
	switch(cmodel)
	{
		case 0:
			return "YUV 4:2:0";
			break;
		
		case 1:
			return "YUV 4:2:2";
			break;
			
		default:
			return "YUV 4:2:0";
			break;
	}
}






